#pragma once

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <array>

// =========================================================
// 1. Fixed-Point Math & Constants
// =========================================================

using dsp_sample_t = int32_t;
using dsp_accum_t = int64_t; // 67-bit accum in hardware, 64-bit is sufficient for C++ sim

#define Q31_MAX 2147483647
#define Q31_MIN -2147483648
#define FIXED_ONE 2147483648.0f

// Saturation Logic: The "sound" of E-mu filters relies on this specific clipping.
// IMPORTANT: Uses branchless saturation to avoid CPU pipeline stalls.
inline dsp_sample_t saturate(dsp_accum_t x) {
    return static_cast<dsp_sample_t>(
        std::min(std::max(x, static_cast<dsp_accum_t>(Q31_MIN)),
                 static_cast<dsp_accum_t>(Q31_MAX))
    );
}

// Haunted Saturation: Controlled mantissa corruption for vintage digital artifacts
inline dsp_sample_t haunted_saturate(dsp_accum_t x, float bitDepth = 24.0f) {
    dsp_sample_t clean = saturate(x);

    if (bitDepth < 31.0f) {
        int bitsToKeep = static_cast<int>(bitDepth);
        bitsToKeep = std::max(8, std::min(31, bitsToKeep)); // Safe bounds: 8-31 bits
        int shift = 31 - bitsToKeep;
        dsp_sample_t mask = ~((1 << shift) - 1);
        return clean & mask; // Truncate lower bits
    }

    return clean;
}

inline dsp_sample_t float_to_fixed(float f) {
    f = std::min(std::max(f, -1.0f), 1.0f);
    return static_cast<dsp_sample_t>(f * FIXED_ONE);
}

inline float fixed_to_float(dsp_sample_t i) {
    return (float)i / FIXED_ONE;
}

// =========================================================
// 2. Data Structures: The Z-Plane Topology
// =========================================================

struct ZPlaneSection {
    float k1;   // Frequency (0.0 - 1.0 mapping to Logarithmic Octaves)
    float k2;   // Resonance (0.0 - 1.0 mapping to Linear dB)
    float gain; // Gain compensation (0.0 - 1.0)
    int type;   // 0=LP, 1=HP, 2=BP, 3=Peak, 4=Shelf
};

struct ZPlaneFrame {
    static const int SECTION_COUNT = 7;
    ZPlaneSection sections[SECTION_COUNT];
};

struct ZPlaneCube {
    ZPlaneFrame corners[8];
    // Corner bits: 0bXYZ where X = Morph, Y = Freq, Z = Transform
};

struct BiquadCoeffs {
    dsp_sample_t b1; // Feedback 1
    dsp_sample_t b2; // Feedback 2
    dsp_sample_t a0, a1, a2; // Feedforward
};

// =========================================================
// 3. The ARMAdillo Engine (Decoder & Interpolator)
// =========================================================

class ARMAdilloEngine {
public:
    static ZPlaneFrame interpolate_cube(const ZPlaneCube& cube, float x, float y, float z);
    static BiquadCoeffs decode_section(const ZPlaneSection& params, float sampleRate = 48000.0f);

private:
    static float lerp(float a, float b, float t) { return a + t * (b - a); }
};

// =========================================================
// 4. The DSP Pipeline (H-Chip Emulation)
// =========================================================

class EmuHChipFilter {
private:
    struct SectionState {
        dsp_sample_t x1, x2;
        dsp_sample_t y1, y2;
    };

    SectionState states[ZPlaneFrame::SECTION_COUNT];
    float sampleRate;

public:
    EmuHChipFilter(float initialSampleRate = 48000.0f)
        : sampleRate(initialSampleRate) {
        reset();
    }

    void reset();
    void setSampleRate(float newSampleRate);
    float getSampleRate() const;

    // Biquad with haunted saturation in the feedback loop.
    inline dsp_sample_t process_biquad(dsp_sample_t x, SectionState& s, const BiquadCoeffs& c) {
        dsp_accum_t acc = 0;

        acc += (dsp_accum_t)x * c.a0;
        acc += (dsp_accum_t)s.x1 * c.a1;
        acc += (dsp_accum_t)s.x2 * c.a2;

        acc += (dsp_accum_t)s.y1 * c.b1;
        acc += (dsp_accum_t)s.y2 * c.b2;

        acc >>= 31;

        dsp_sample_t y = haunted_saturate(acc);

        s.x2 = s.x1;
        s.x1 = x;
        s.y2 = s.y1;
        s.y1 = y;

        return y;
    }

    void process_block(const ZPlaneCube& cube,
                       float mod_x, float mod_y, float mod_z,
                       const std::vector<float>& input,
                       std::vector<float>& output);

    void process_block(const ZPlaneCube& cube,
                       float mod_x, float mod_y, float mod_z,
                       const float* input,
                       float* output,
                       int numSamples);
};
