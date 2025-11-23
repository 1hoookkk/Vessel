/**
 * DIAGNOSTIC VERSION - Grey Box Test
 * Adds debug output to see what's actually happening with the filter
 */

#include "EmuZPlaneCore.h"
#include "PresetConverter.h"
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    std::cout << "=== GREY BOX DIAGNOSTIC ===" << std::endl;

    const float sampleRate = 44100.0f;
    EmuHChipFilter filter(sampleRate);
    ZPlaneCube cube = PresetConverter::convertToCube(0, 12);

    // Test interpolation at different morph values
    std::cout << "\n1. Testing Trilinear Interpolation:" << std::endl;

    for (float morph = 0.0f; morph <= 1.0f; morph += 0.5f) {
        ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(cube, morph, 0.5f, 0.5f);
        std::cout << "\nMorph = " << morph << ":" << std::endl;
        std::cout << "Section 0: k1=" << frame.sections[0].k1
                  << ", k2=" << frame.sections[0].k2
                  << ", gain=" << frame.sections[0].gain << std::endl;
        std::cout << "Section 3: k1=" << frame.sections[3].k1
                  << ", k2=" << frame.sections[3].k2
                  << ", gain=" << frame.sections[3].gain << std::endl;
    }

    // Test coefficient decoding
    std::cout << "\n2. Testing Coefficient Decode:" << std::endl;

    ZPlaneFrame testFrame = ARMAdilloEngine::interpolate_cube(cube, 0.5f, 0.5f, 0.5f);

    for (int i = 0; i < 3; ++i) {  // Check first 3 sections
        ZPlaneSection sec = testFrame.sections[i];
        BiquadCoeffs coeffs = ARMAdilloEngine::decode_section(sec, sampleRate);

        std::cout << "\nSection " << i << ":" << std::endl;
        std::cout << "  k1=" << sec.k1 << ", k2=" << sec.k2 << ", gain=" << sec.gain << std::endl;
        std::cout << "  Fixed-point coeffs:" << std::endl;
        std::cout << "    b1=" << coeffs.b1 << " (" << fixed_to_float(coeffs.b1) << ")" << std::endl;
        std::cout << "    b2=" << coeffs.b2 << " (" << fixed_to_float(coeffs.b2) << ")" << std::endl;
        std::cout << "    a0=" << coeffs.a0 << " (" << fixed_to_float(coeffs.a0) << ")" << std::endl;
        std::cout << "    a1=" << coeffs.a1 << " (" << fixed_to_float(coeffs.a1) << ")" << std::endl;
        std::cout << "    a2=" << coeffs.a2 << " (" << fixed_to_float(coeffs.a2) << ")" << std::endl;
    }

    // Test impulse response
    std::cout << "\n3. Testing Impulse Response:" << std::endl;

    std::vector<float> impulse(64, 0.0f);
    impulse[0] = 1.0f;
    std::vector<float> output(64, 0.0f);

    filter.reset();
    filter.process_block(cube, 0.5f, 0.5f, 0.5f, impulse.data(), output.data(), 64);

    std::cout << "Input [0-5]: ";
    for (int i = 0; i < 6; ++i) std::cout << impulse[i] << " ";
    std::cout << std::endl;

    std::cout << "Output [0-5]: ";
    for (int i = 0; i < 6; ++i) std::cout << output[i] << " ";
    std::cout << std::endl;

    std::cout << "Output [10-15]: ";
    for (int i = 10; i < 16; ++i) std::cout << output[i] << " ";
    std::cout << std::endl;

    // Check if output is all zeros (filter not working)
    float maxOutput = 0.0f;
    for (float val : output) {
        maxOutput = std::max(maxOutput, std::abs(val));
    }

    std::cout << "\nMax output amplitude: " << maxOutput << std::endl;

    if (maxOutput < 0.0001f) {
        std::cout << "\n✗ FILTER IS NOT WORKING - Output is essentially zero!" << std::endl;
        std::cout << "  Possible causes:" << std::endl;
        std::cout << "  - Coefficients are zeroed out" << std::endl;
        std::cout << "  - Gain is too low" << std::endl;
        std::cout << "  - Biquad processing is broken" << std::endl;
    } else {
        std::cout << "\n✓ Filter is producing output" << std::endl;
        std::cout << "  If Grey Box test sounds unchanged, the resonance might be too subtle" << std::endl;
    }

    return 0;
}
