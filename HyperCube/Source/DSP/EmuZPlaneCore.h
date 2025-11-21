#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>

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
    static ZPlaneFrame interpolate_cube(const ZPlaneCube& cube, float x, float y, float z);

    // The "Secret Sauce": Decoding k-space to Biquad Coefficients
    // IMPORTANT: Sample rate parameter ensures correct frequency mapping
    static BiquadCoeffs decode_section(const ZPlaneSection& params, float sampleRate = 48000.0f);

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

    void reset();

    // Update sample rate (called from prepareToPlay in JUCE plugin)
    void setSampleRate(float newSampleRate);

    float getSampleRate() const;

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
                       std::vector<float>& output);
};