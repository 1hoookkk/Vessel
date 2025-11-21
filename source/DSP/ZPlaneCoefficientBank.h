#pragma once

#include <array>
#include <cstdint>

namespace zplane {

// Maximum number of biquad sections per model
constexpr int kMaxSections = 6; // up to 12 poles in conjugate pairs

// Polar parameterization of a single second-order section (conjugate pair)
struct SectionPolar {
    // Poles (complex conjugate at radius/angle)
    float poleRadius { 0.0f };   // (0, 1], clamped at runtime to kRmax
    float poleAngle  { 0.0f };   // radians

    // Zeros (optional) (0 => unused)
    float zeroRadius { 0.0f };   // [0, 1]
    float zeroAngle  { 0.0f };   // radians

    // Linear gain applied to this section
    float sectionGain { 1.0f };
};

// One Z-plane filter model (cascade of sections)
struct Model {
    std::uint8_t numSections { 0 };                             // 0..kMaxSections
    std::array<SectionPolar, kMaxSections> s { { } };           // valid up to numSections
    float overallGain { 1.0f };                                 // linear overall gain
};

// Read-only coefficient bank (clean-room). Construct off the audio thread.
class ZPlaneCoefficientBank {
public:
    ZPlaneCoefficientBank();

    // Accessors
    const Model& getModel(std::uint16_t modelId) const;
    std::uint16_t modelCount() const noexcept { return modelCount_; }

private:
    std::uint16_t modelCount_ { 0 };
    std::array<Model, 64> models_ { { } }; // MVP: reserve space for up to 64 models
};

} // namespace zplane


