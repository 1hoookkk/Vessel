#include "ZPlaneCoefficientBank.h"
#include <cassert>

namespace zplane {

ZPlaneCoefficientBank::ZPlaneCoefficientBank() {
    // Placeholder clean-room examples so wiring compiles; replace with analyzed models.
    modelCount_ = 2;

    models_[0].numSections = 3;
    models_[0].s[0] = { 0.92f, 0.20f, 0.0f, 0.0f, 1.0f };
    models_[0].s[1] = { 0.90f, 0.40f, 0.0f, 0.0f, 1.0f };
    models_[0].s[2] = { 0.88f, 0.60f, 0.0f, 0.0f, 1.0f };
    models_[0].overallGain = 1.0f;

    models_[1].numSections = 6;
    for (int i = 0; i < 6; ++i) {
        models_[1].s[(size_t)i] = { 0.97f, 0.15f + 0.10f * (float)i, 0.0f, 0.0f, 1.0f };
    }
    models_[1].overallGain = 1.0f;
}

const Model& ZPlaneCoefficientBank::getModel(std::uint16_t modelId) const {
    assert(modelId < modelCount_);
    return models_[(size_t)modelId];
}

} // namespace zplane


