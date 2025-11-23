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
#define Q31_MIN (-2147483647 - 1)
#define FIXED_ONE 2147483647.0f  // INT32_MAX, not 2^31 (prevents overflow)

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
    // 14-pole filter = 7 biquad sections (Phase 1.1 spec requirement)
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