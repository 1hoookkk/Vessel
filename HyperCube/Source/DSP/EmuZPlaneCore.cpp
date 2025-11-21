#include "EmuZPlaneCore.h"

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
    // b2 determines the pole radius (R^2).
    // E-mu logic: As k2 increases, R approaches 1.0.
    // Approx: b2 = 1.0 - 2^(-k2 * scale).
    // Using a simplified model where k2 (0.0-1.0) maps to R (0.0 to 0.99)
    float R = 0.0f + (0.98f * params.k2);
    float b2_real = -(R * R); // Standard biquad convention: b2 is negative for stability

    // 2. Decode Frequency (k1)
    // k1 maps to Theta.
    // Mapping: Theta = Pi * (BaseFreq / Nyquist) * 2^(k1 * OctaveRange)
    // Simplified for demo: Map 0.0-1.0 to 20Hz-20kHz
    float freqHz = 20.0f * std::pow(1000.0f, params.k1);
    float theta = 2.0f * 3.14159f * freqHz / sampleRate;  // Use parameterized sample rate

    // 3. Calculate Feedback 1 (b1)
    // b1 = 2 * R * cos(theta)
    float b1_real = 2.0f * R * std::cos(theta);

    // 4. Feedforward (a0, a1, a2)
    // For a Z-Plane "Resonator", often a0=1-R, a1=0, a2=-(1-R) or similar for constant peak.
    // Using a simple LowPass topology for demonstration:
    // H(z) = K * (1 + 2z^-1 + z^-2) / (1 - b1z^-1 - b2z^-2)
    float alpha = (1.0f - R); // simple gain comp
    float a0_real = alpha * params.gain;
    float a1_real = 0.0f;
    float a2_real = -alpha * params.gain; // Bandpass-ish

    // Convert to Fixed Point
    c.b1 = float_to_fixed(b1_real);
    c.b2 = float_to_fixed(b2_real);
    c.a0 = float_to_fixed(a0_real);
    c.a1 = float_to_fixed(a1_real);
    c.a2 = float_to_fixed(a2_real);

    return c;
}

// =========================================================
// EmuHChipFilter Implementation
// =========================================================

void EmuHChipFilter::reset() {
    for (int i = 0; i < ZPlaneFrame::SECTION_COUNT; ++i) {
        states[i] = {0, 0, 0, 0};
    }
}

void EmuHChipFilter::setSampleRate(float newSampleRate) {
    sampleRate = newSampleRate;
    // Note: Caller should re-calculate coefficients after this call
}

float EmuHChipFilter::getSampleRate() const {
    return sampleRate;
}

void EmuHChipFilter::process_block(const ZPlaneCube& cube, 
                   float mod_x, float mod_y, float mod_z, 
                   const std::vector<float>& input, 
                   std::vector<float>& output) {
    
    // 1. Calculate ARMAdillo parameters for this block
    // (In real hardware, this might ramp per 32 samples. We do it per block here)
    ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(cube, mod_x, mod_y, mod_z);

    // 2. Decode into coefficients for all 7 sections
    BiquadCoeffs coeffs[ZPlaneFrame::SECTION_COUNT];
    for(int s=0; s<ZPlaneFrame::SECTION_COUNT; ++s) {
        coeffs[s] = ARMAdilloEngine::decode_section(frame.sections[s], sampleRate);
    }

    // 3. Sample Loop
    for (size_t i = 0; i < input.size(); ++i) {
        // Convert Input Float -> Fixed
        dsp_sample_t signal = float_to_fixed(input[i]);

        // Cascade Series Processing
        for (int s = 0; s < ZPlaneFrame::SECTION_COUNT; ++s) {
            signal = process_biquad(signal, states[s], coeffs[s]);
        }

        // Convert Output Fixed -> Float
        output[i] = fixed_to_float(signal);
    }
}
