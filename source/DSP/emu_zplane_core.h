/**
 * E-mu Z-Plane Filter Emulation Core (Project ARMAdillo)
 *
 * Theory of Operation:
 * This implementation replicates the E-mu H-Chip/FX8010 filter architecture.
 * It features:
 * 1. 14-Pole Cascade: 7 Biquad sections in series.
 * 2. ARMAdillo Encoding: Interpolation happens in k-space (Pitch/Resonance), not Coefficient-space.
 * 3. Filter Cube: Trilinear interpolation between 8 filter frames.
 * 4. Fixed-Point Arithmetic: 32-bit state/coeff with 64-bit accumulation and hardware-accurate saturation.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

namespace emu {

// =========================================================
// 1. Fixed-Point Math & Constants
// =========================================================

// The FX8010/H-Chip uses 32-bit registers.
// Q31 Format: 1 Sign bit, 0 Integer bits, 31 Fractional bits.
using dsp_sample_t = int32_t;
using dsp_accum_t = int64_t; // 67-bit accum in hardware, 64-bit is sufficient for C++ sim

constexpr dsp_sample_t Q31_MAX = 2147483647;
constexpr dsp_sample_t Q31_MIN = -2147483648;
constexpr float FIXED_ONE = 2147483648.0f;

// Saturation Logic: The "sound" of E-mu filters relies on this specific clipping.
inline dsp_sample_t saturate(dsp_accum_t x) {
    if (x > Q31_MAX) return Q31_MAX;
    if (x < Q31_MIN) return Q31_MIN;
    return static_cast<dsp_sample_t>(x);
}

// Helper: Float to Q31
inline dsp_sample_t float_to_fixed(float f) {
    if (f >= 1.0f) return Q31_MAX;
    if (f <= -1.0f) return Q31_MIN;
    return static_cast<dsp_sample_t>(f * FIXED_ONE);
}

// Helper: Q31 to Float (for visualization/output)
inline float fixed_to_float(dsp_sample_t i) {
    return static_cast<float>(i) / FIXED_ONE;
}

// =========================================================
// 2. Data Structures: The Z-Plane Topology
// =========================================================

// Filter types supported by the H-Chip
enum class FilterType {
    LP = 0,  // Low-pass
    HP = 1,  // High-pass
    BP = 2,  // Band-pass
    Peak = 3,  // Peak/Notch
    Shelf = 4  // Shelf
};

// A single filter section in ARMAdillo parameter space
struct ZPlaneSection {
    float k1;   // Frequency (0.0 - 1.0 mapping to Logarithmic Octaves)
    float k2;   // Resonance (0.0 - 1.0 mapping to Linear dB)
    float gain; // Gain compensation (0.0 - 1.0)
    FilterType type;   // Filter type

    ZPlaneSection() : k1(0.5f), k2(0.0f), gain(1.0f), type(FilterType::LP) {}
    ZPlaneSection(float k1_, float k2_, float gain_, FilterType type_)
        : k1(k1_), k2(k2_), gain(gain_), type(type_) {}
};

// A Frame contains the parameters for all 7 cascade sections
struct ZPlaneFrame {
    static constexpr int SECTION_COUNT = 7;
    std::array<ZPlaneSection, SECTION_COUNT> sections;

    ZPlaneFrame() = default;
};

// The Cube contains the 8 corner frames used for morphing
struct ZPlaneCube {
    std::array<ZPlaneFrame, 8> corners;
    // Corner Indices:
    // 0: 000 (Min X, Min Y, Min Z)
    // 1: 100 (Max X, Min Y, Min Z)
    // 2: 010
    // 3: 110
    // 4: 001
    // 5: 101
    // 6: 011
    // 7: 111

    ZPlaneCube() = default;
};

// Direct Form I Biquad Coefficients (Decoded)
struct BiquadCoeffs {
    dsp_sample_t b1; // Feedback 1 (inverted sign convention typical of Rossum patents)
    dsp_sample_t b2; // Feedback 2
    dsp_sample_t a0, a1, a2; // Feedforward

    BiquadCoeffs() : b1(0), b2(0), a0(float_to_fixed(1.0f)), a1(0), a2(0) {}
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

        SectionState() : x1(0), x2(0), y1(0), y2(0) {}
    };

    std::array<SectionState, ZPlaneFrame::SECTION_COUNT> states;
    float sampleRate;

public:
    EmuHChipFilter();

    void reset();
    void prepare(float sampleRate);

    // The Authentic Fixed-Point Biquad Operation
    inline dsp_sample_t process_biquad(dsp_sample_t x, SectionState& s, const BiquadCoeffs& c);

    // Process a single sample through the 7-section cascade
    float processSample(const ZPlaneCube& cube, float mod_x, float mod_y, float mod_z, float input);

    // Process a block of samples with interpolation
    void process_block(const ZPlaneCube& cube,
                       float mod_x, float mod_y, float mod_z,
                       const float* input,
                       float* output,
                       int numSamples);
};

} // namespace emu
