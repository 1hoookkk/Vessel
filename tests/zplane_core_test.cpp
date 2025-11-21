/**
 * Z-Plane Core Verification Tests
 * Focus: ensure Y/Z axes influence the response and haunted saturation stays bounded.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "EmuZPlaneCore.h"
#include "PresetConverter.h"

namespace {

bool buffersDiffer(const std::vector<float>& a, const std::vector<float>& b, float eps = 1e-6f) {
    if (a.size() != b.size()) return true;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::abs(a[i] - b[i]) > eps) return true;
    }
    return false;
}

bool test_branchless_saturation() {
    std::cout << "\n=== Branchless/Haunted Saturation Clip Test ===" << std::endl;
    dsp_accum_t huge = static_cast<dsp_accum_t>(Q31_MAX) + 5000;
    dsp_sample_t clipped = haunted_saturate(huge, 0.0f);
    dsp_sample_t haunted = haunted_saturate(huge, 16.0f);

    std::cout << "Clipped:  " << clipped << " (expect " << Q31_MAX << ")" << std::endl;
    std::cout << "Haunted:  " << haunted << " (expect masked lower bits)" << std::endl;

    return clipped == Q31_MAX && haunted != clipped;
}

bool test_axis_activation() {
    std::cout << "\n=== Axis Activation (Y/Z) Test ===" << std::endl;
    EmuHChipFilter filter;
    ZPlaneCube cube = PresetConverter::convertToCube(0, 12); // strong vowel pair

    const int blockSize = 64;
    std::vector<float> input(blockSize, 0.0f);
    input[0] = 1.0f; // impulse

    std::vector<float> outBase(blockSize, 0.0f);
    std::vector<float> outY(blockSize, 0.0f);
    std::vector<float> outZ(blockSize, 0.0f);

    filter.reset();
    filter.process_block(cube, 0.0f, 0.0f, 0.0f, input, outBase);

    filter.reset();
    filter.process_block(cube, 0.0f, 1.0f, 0.0f, input, outY);

    filter.reset();
    filter.process_block(cube, 0.0f, 0.0f, 1.0f, input, outZ);

    bool yDiff = buffersDiffer(outBase, outY);
    bool zDiff = buffersDiffer(outBase, outZ);

    std::cout << "Y-axis response differs: " << (yDiff ? "YES" : "NO") << std::endl;
    std::cout << "Z-axis response differs: " << (zDiff ? "YES" : "NO") << std::endl;

    return yDiff && zDiff;
}

} // namespace

int main() {
    int passed = 0;
    int total = 2;

    if (test_branchless_saturation()) {
        std::cout << "[PASS] Haunted saturation bounds" << std::endl;
        passed++;
    } else {
        std::cout << "[FAIL] Haunted saturation bounds" << std::endl;
    }

    if (test_axis_activation()) {
        std::cout << "[PASS] Y/Z axis activation" << std::endl;
        passed++;
    } else {
        std::cout << "[FAIL] Y/Z axis activation" << std::endl;
    }

    std::cout << "\nTests passed: " << passed << "/" << total << std::endl;
    return passed == total ? 0 : 1;
}
