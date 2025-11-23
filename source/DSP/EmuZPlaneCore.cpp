#include "EmuZPlaneCore.h"
#include <cassert>

// =========================================================
// ARMAdilloEngine Implementation
// =========================================================

ZPlaneFrame ARMAdilloEngine::interpolate_cube(const ZPlaneCube& cube, float x, float y, float z) {
    ZPlaneFrame result;
    
    // Clamp inputs
    x = std::max(0.0f, std::min(1.0f, x));
    y = std::max(0.0f, std::min(1.0f, y));
    z = std::max(0.0f, std::min(1.0f, z));

    for (int i = 0; i < ZPlaneFrame::SECTION_COUNT; ++i) {
        // Extract k1 values for this specific section from all 8 corners
        float c000 = cube.corners[0].sections[i].k1;
        float c100 = cube.corners[1].sections[i].k1;
        float c010 = cube.corners[2].sections[i].k1;
        float c110 = cube.corners[3].sections[i].k1;
        float c001 = cube.corners[4].sections[i].k1;
        float c101 = cube.corners[5].sections[i].k1;
        float c011 = cube.corners[6].sections[i].k1;
        float c111 = cube.corners[7].sections[i].k1;

        // Interpolate X
        float c00 = lerp(c000, c100, x);
        float c10 = lerp(c010, c110, x);
        float c01 = lerp(c001, c101, x);
        float c11 = lerp(c011, c111, x);

        // Interpolate Y
        float c0 = lerp(c00, c10, y);
        float c1 = lerp(c01, c11, y);

        // Interpolate Z
        result.sections[i].k1 = lerp(c0, c1, z);

        // Interpolate k2 (Resonance) - Full trilinear interpolation
        float k2_000 = cube.corners[0].sections[i].k2;
        float k2_100 = cube.corners[1].sections[i].k2;
        float k2_010 = cube.corners[2].sections[i].k2;
        float k2_110 = cube.corners[3].sections[i].k2;
        float k2_001 = cube.corners[4].sections[i].k2;
        float k2_101 = cube.corners[5].sections[i].k2;
        float k2_011 = cube.corners[6].sections[i].k2;
        float k2_111 = cube.corners[7].sections[i].k2;

        float k2_c00 = lerp(k2_000, k2_100, x);
        float k2_c10 = lerp(k2_010, k2_110, x);
        float k2_c01 = lerp(k2_001, k2_101, x);
        float k2_c11 = lerp(k2_011, k2_111, x);
        float k2_c0 = lerp(k2_c00, k2_c10, y);
        float k2_c1 = lerp(k2_c01, k2_c11, y);
        result.sections[i].k2 = lerp(k2_c0, k2_c1, z);

        // Interpolate gain - Full trilinear interpolation
        float g_000 = cube.corners[0].sections[i].gain;
        float g_100 = cube.corners[1].sections[i].gain;
        float g_010 = cube.corners[2].sections[i].gain;
        float g_110 = cube.corners[3].sections[i].gain;
        float g_001 = cube.corners[4].sections[i].gain;
        float g_101 = cube.corners[5].sections[i].gain;
        float g_011 = cube.corners[6].sections[i].gain;
        float g_111 = cube.corners[7].sections[i].gain;

        float g_c00 = lerp(g_000, g_100, x);
        float g_c10 = lerp(g_010, g_110, x);
        float g_c01 = lerp(g_001, g_101, x);
        float g_c11 = lerp(g_011, g_111, x);
        float g_c0 = lerp(g_c00, g_c10, y);
        float g_c1 = lerp(g_c01, g_c11, y);
        result.sections[i].gain = lerp(g_c0, g_c1, z);

        // Type: Use nearest-neighbor (corner 0) - filter topology shouldn't morph
        // In production, all corners should have the same type for a given section
        result.sections[i].type = cube.corners[0].sections[i].type;
    }
    return result;
}

BiquadCoeffs ARMAdilloEngine::decode_section(const ZPlaneSection& params, float sampleRate) {
    BiquadCoeffs c;

    // 1. Decode Resonance (k2)
    // Map k2 through a gentle curve to bias toward stable radii while allowing high-Q peaks.
    // CRITICAL: At extreme k2 (0.98+), apply safety limiter to prevent instability
    float k2_clamped = std::clamp(params.k2, 0.0f, 1.0f);
    float R = std::pow(k2_clamped, 1.35f) * 0.995f;

    // SAFETY: Above k2=0.95, compress the curve to prevent runaway resonance
    if (k2_clamped > 0.95f) {
        float excess = (k2_clamped - 0.95f) / 0.05f; // 0-1 in danger zone
        float compressed = 0.95f + excess * 0.035f;  // Map 0.95-1.0 → 0.95-0.985
        R = std::pow(compressed, 1.35f) * 0.995f;
    }

    R = std::clamp(R, 0.10f, 0.988f); // Hard limit: never exceed 0.988
    // 2. Decode Frequency (k1) -> theta at CURRENT sample rate
    // k1 is sample-rate independent (normalized log scale: 20Hz-20kHz)
    // We convert k1 → Hz → theta using the current sampleRate.
    // This ensures poles track the correct frequency regardless of playback sample rate.
    float freqHz = 20.0f * std::pow(1000.0f, params.k1); // Inverse of log10(f/20)/3
    freqHz = std::clamp(freqHz, 20.0f, sampleRate * 0.49f); // Respect Nyquist
    float theta = 2.0f * 3.14159f * freqHz / sampleRate; // Digital frequency

    // 3. Calculate Feedback 1 (b1)
    float b1_real = 2.0f * R * std::cos(theta);

    // CRITICAL FIX: Prevent coefficient overflow at extreme parameters
    // At high resonance + high frequency, b1 can exceed Q15 range [-1.0, 1.0)
    // Scale all coefficients if b1 would overflow
    float coeff_scale = 1.0f;
    if (std::abs(b1_real) > 0.99f) {
        coeff_scale = 0.99f / std::abs(b1_real);
        b1_real *= coeff_scale;
    }

    // 4. Feedforward: RESONATOR topology (not bandpass!)
    // CRITICAL FIX: E-mu uses resonators that ADD peaks without killing signal
    // Bandpass (a0=-a2) kills fundamentals when cascaded
    // Resonator (a0=1, a2=0) passes signal through while poles create formant peaks
    float cascadeGain = 1.0f / 7.0f; // Divide by section count to prevent clipping
    float b2_real = -(R * R) * coeff_scale; // Apply overflow scaling
    float a0_real = cascadeGain * params.gain * coeff_scale; // Apply overflow scaling
    float a1_real = 0.0f;
    float a2_real = 0.0f; // ZERO - this is the key difference from bandpass

    // Convert to Fixed Point
    c.b1 = float_to_fixed(b1_real);
    c.b2 = float_to_fixed(b2_real);
    c.a0 = float_to_fixed(a0_real);
    c.a1 = float_to_fixed(a1_real);
    c.a2 = float_to_fixed(a2_real);

    return c;
}