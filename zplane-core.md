/**
 * E-mu Z-Plane Filter Emulation Core (Project ARMAdillo)
 * * Theory of Operation:
 * This implementation replicates the E-mu H-Chip/FX8010 filter architecture.
 * It features:
 * 1. 14-Pole Cascade: 7 Biquad sections in series.
 * 2. ARMAdillo Encoding: Interpolation happens in k-space (Pitch/Resonance), not Coefficient-space.
 * 3. Filter Cube: Trilinear interpolation between 8 filter frames.
 * 4. Fixed-Point Arithmetic: 32-bit state/coeff with 64-bit accumulation and hardware-accurate saturation.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <iomanip>

// =========================================================
// 1. Fixed-Point Math & Constants
// =========================================================

// The FX8010/H-Chip uses 32-bit registers.
// Q31 Format: 1 Sign bit, 0 Integer bits, 31 Fractional bits.
using dsp_sample_t = int32_t;
using dsp_accum_t = int64_t; // 67-bit accum in hardware, 64-bit is sufficient for C++ sim

#define Q31_MAX 2147483647
#define Q31_MIN -2147483648
#define FIXED_ONE 2147483648.0f

// Saturation Logic: The "sound" of E-mu filters relies on this specific clipping.
// IMPORTANT: Uses branchless saturation to avoid CPU pipeline stalls.
inline dsp_sample_t saturate(dsp_accum_t x) {
    // Branchless clamp: std::min(std::max(x, min), max)
    return static_cast<dsp_sample_t>(
        std::min(std::max(x, static_cast<dsp_accum_t>(Q31_MIN)),
                 static_cast<dsp_accum_t>(Q31_MAX))
    );
}

// Helper: Float to Q31
// IMPORTANT: Uses branchless saturation to avoid CPU pipeline stalls.
inline dsp_sample_t float_to_fixed(float f) {
    // Clamp input range before conversion
    f = std::min(std::max(f, -1.0f), 1.0f);
    return static_cast<dsp_sample_t>(f * FIXED_ONE);
}

// Helper: Q31 to Float (for visualization/output)
inline float fixed_to_float(dsp_sample_t i) {
    return (float)i / FIXED_ONE;
}

// =========================================================
// 2. Data Structures: The Z-Plane Topology
// =========================================================

// A single filter section in ARMAdillo parameter space
struct ZPlaneSection {
    float k1;   // Frequency (0.0 - 1.0 mapping to Logarithmic Octaves)
    float k2;   // Resonance (0.0 - 1.0 mapping to Linear dB)
    float gain; // Gain compensation (0.0 - 1.0)
    int type;   // 0=LP, 1=HP, 2=BP, 3=Peak, 4=Shelf
};

// A Frame contains the parameters for all 7 cascade sections
struct ZPlaneFrame {
    static const int SECTION_COUNT = 7;
    ZPlaneSection sections[SECTION_COUNT];
};

// The Cube contains the 8 corner frames used for morphing
struct ZPlaneCube {
    ZPlaneFrame corners[8];
    // Corner Indices:
    // 0: 000 (Min X, Min Y, Min Z)
    // 1: 100 (Max X, Min Y, Min Z)
    // 2: 010
    // 3: 110
    // 4: 001
    // 5: 101
    // 6: 011
    // 7: 111
};

// Direct Form I Biquad Coefficients (Decoded)
struct BiquadCoeffs {
    dsp_sample_t b1; // Feedback 1 (inverted sign convention typical of Rossum patents)
    dsp_sample_t b2; // Feedback 2
    dsp_sample_t a0, a1, a2; // Feedforward
};

// =========================================================
// 3. The ARMAdillo Engine (Decoder & Interpolator)
// =========================================================

class ARMAdilloEngine {
public:
    // Trilinear Interpolation of the Filter Cube
    // x (Morph), y (Freq/Track), z (Transform) -> All 0.0 to 1.0
    static ZPlaneFrame interpolate_cube(const ZPlaneCube& cube, float x, float y, float z) {
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

    // The "Secret Sauce": Decoding k-space to Biquad Coefficients
    // IMPORTANT: Sample rate parameter ensures correct frequency mapping
    static BiquadCoeffs decode_section(const ZPlaneSection& params, float sampleRate = 48000.0f) {
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

private:
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
};

// =========================================================
// 4. The DSP Pipeline (H-Chip Emulation)
// =========================================================

class EmuHChipFilter {
private:
    struct SectionState {
        dsp_sample_t x1, x2; // Input history
        dsp_sample_t y1, y2; // Output history
    };

    SectionState states[ZPlaneFrame::SECTION_COUNT];
    float sampleRate;  // Current sample rate for frequency calculations

public:
    EmuHChipFilter(float initialSampleRate = 48000.0f)
        : sampleRate(initialSampleRate) {
        reset();
    }

    void reset() {
        for (int i = 0; i < ZPlaneFrame::SECTION_COUNT; ++i) {
            states[i] = {0, 0, 0, 0};
        }
    }

    // Update sample rate (called from prepareToPlay in JUCE plugin)
    void setSampleRate(float newSampleRate) {
        sampleRate = newSampleRate;
        // Note: Caller should re-calculate coefficients after this call
    }

    float getSampleRate() const {
        return sampleRate;
    }

    // The Authentic Fixed-Point Biquad Operation
    inline dsp_sample_t process_biquad(dsp_sample_t x, SectionState& s, const BiquadCoeffs& c) {
        dsp_accum_t acc = 0;

        // MAC Operations (Multiply-Accumulate)
        // Input feedforward
        acc += (dsp_accum_t)x * c.a0;
        acc += (dsp_accum_t)s.x1 * c.a1;
        acc += (dsp_accum_t)s.x2 * c.a2;

        // Feedback (Note: subtraction or addition depends on coeff sign convention)
        // Standard difference eq: y[n] = x terms + b1*y[n-1] + b2*y[n-2]
        acc += (dsp_accum_t)s.y1 * c.b1;
        acc += (dsp_accum_t)s.y2 * c.b2;

        // Shift Back (Q62 -> Q31)
        // The product of two Q31 numbers is Q62. We shift right by 31 to get back to Q31.
        acc >>= 31;

        // Hardware Saturation (The "E-mu Sound")
        dsp_sample_t y = saturate(acc);

        // Update State
        s.x2 = s.x1;
        s.x1 = x;
        s.y2 = s.y1;
        s.y1 = y;

        return y;
    }

    // Process a block of samples with interpolation
    void process_block(const ZPlaneCube& cube, 
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
};

// =========================================================
// 5. Verification & Demonstration
// =========================================================

int main() {
    std::cout << "Initializing E-mu Z-Plane Architecture Emulation..." << std::endl;

    // 1. Setup the Engine
    EmuHChipFilter filterEngine;
    ZPlaneCube demoCube;

    // 2. Define the Cube (Simplified)
    // Corner 0: Low Frequency, Low Res
    // Corner 1: High Frequency, Low Res
    for (int i = 0; i < 7; ++i) {
        demoCube.corners[0].sections[i] = {0.1f, 0.2f, 1.0f, 0}; // Closed
        demoCube.corners[1].sections[i] = {0.9f, 0.2f, 1.0f, 0}; // Open
        // ... fill other corners ...
    }

    // 3. Generate Test Signal (Impulse for Impulse Response / Sawtooth for morph)
    const int blockSize = 64;
    std::vector<float> input(blockSize, 0.0f);
    std::vector<float> output(blockSize, 0.0f);
    
    // Impulse
    input[0] = 1.0f; 

    // 4. Run Filter: Frame 1 (Closed)
    std::cout << "\n--- Processing Frame A (Closed Filter) ---" << std::endl;
    filterEngine.process_block(demoCube, 0.0f, 0.0f, 0.0f, input, output);
    
    std::cout << std::fixed << std::setprecision(4);
    for(int i=0; i<5; ++i) {
        std::cout << "Sample " << i << ": " << output[i] << std::endl;
    }

    // 5. Run Filter: Frame 2 (Morphing halfway)
    std::cout << "\n--- Processing Morph (50% Open) ---" << std::endl;
    filterEngine.reset(); // Reset state for clean impulse
    filterEngine.process_block(demoCube, 0.5f, 0.0f, 0.0f, input, output);

    for(int i=0; i<5; ++i) {
        std::cout << "Sample " << i << ": " << output[i] << std::endl;
    }

    std::cout << "\nEmulation Complete. Check limit cycles and saturation logic." << std::endl;
    return 0;
}