#include "PresetConverter.h"
#include <array>
#include <utility>
#include <cmath>
#include <algorithm>

namespace {

using PolarPole = std::pair<float, float>; // {radius, theta}
using PolarShape = std::array<PolarPole, ZPlaneFrame::SECTION_COUNT>;

constexpr float kPi = 3.14159265f;
constexpr float kThetaSwing = 0.35f;   // radians of pole rotation for Y-axis extremes
constexpr float kRadiusSwing = 0.08f;  // +/- radius scaling for Z-axis extremes

inline float clampRadius(float r) {
    return std::clamp(r, 0.10f, 0.995f);
}

// Materialize the legacy table into a polar shape.
PolarShape loadPolarShape(int shapeIndex) {
    PolarShape out{};

    if (shapeIndex < 0 || shapeIndex >= AUTHENTIC_EMU_NUM_SHAPES) {
        for (auto& s : out) s = {0.0f, 0.0f};
        out[6] = {0.99f, 0.0f};
        return out;
    }

    const float* shapeData = AUTHENTIC_EMU_SHAPES[shapeIndex];
    for (int i = 0; i < 6; ++i) {
        out[i] = { shapeData[i * 2], shapeData[i * 2 + 1] };
    }

    // 7th section: gentle pass-through helper to keep the cascade stable.
    out[6] = {0.99f, 0.0f};
    return out;
}

} // namespace

ZPlaneCube PresetConverter::convertToCube(int shapeIndexA, int shapeIndexB) {
    ZPlaneCube cube{};

    const PolarShape shapeA = loadPolarShape(shapeIndexA);
    const PolarShape shapeB = loadPolarShape(shapeIndexB);

    for (int corner = 0; corner < 8; ++corner) {
        const int x = (corner & 0b001) ? 1 : 0;
        const int y = (corner & 0b010) ? 1 : 0;
        const int z = (corner & 0b100) ? 1 : 0;

        const PolarShape& base = x == 0 ? shapeA : shapeB;

        for (int s = 0; s < ZPlaneFrame::SECTION_COUNT; ++s) {
            float r = base[s].first;
            float theta = base[s].second;

            // Y-axis: rotate pole angle (symmetric swing around the origin).
            const float thetaDelta = (y ? kThetaSwing : -kThetaSwing);
            theta = std::clamp(theta + thetaDelta, 0.0f, kPi);

            // Z-axis: scale pole radius toward/away from unit circle.
            const float radiusScale = 1.0f + (z ? kRadiusSwing : -kRadiusSwing);
            r = clampRadius(r * radiusScale);

            cube.corners[corner].sections[s] = convertPoleToSection(r, theta);
        }
    }

    return cube;
}

ZPlaneFrame PresetConverter::convertShapeToFrame(int shapeIndex) {
    ZPlaneFrame frame{};
    const PolarShape polar = loadPolarShape(shapeIndex);

    for (int i = 0; i < ZPlaneFrame::SECTION_COUNT; ++i) {
        frame.sections[i] = convertPoleToSection(polar[i].first, polar[i].second);
    }

    return frame;
}

ZPlaneSection PresetConverter::convertPoleToSection(float r, float theta) {
    ZPlaneSection sec{};

    // Theta -> Frequency (k1), using reference sample rate.
    const float sampleRate = (float)AUTHENTIC_EMU_SAMPLE_RATE_REF; // 44100
    float freqHz = theta * sampleRate / (2.0f * kPi);
    freqHz = std::clamp(freqHz, 20.0f, 20000.0f);

    // Logarithmic mapping into 0..1
    sec.k1 = std::log10(freqHz / 20.0f) / 3.0f;
    sec.k1 = std::clamp(sec.k1, 0.0f, 1.0f);

    // Radius -> Resonance (normalize against 0.98 legacy max)
    sec.k2 = std::clamp(r / 0.98f, 0.0f, 1.0f);

    sec.gain = 1.0f;
    sec.type = 0; // LP/resonator topology
    return sec;
}
