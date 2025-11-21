#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../../source/DSP/PresetConverter.h"
#include "../../source/DSP/EmuZPlaneCore.h"
#include <iostream>

TEST_CASE("PresetConverter Basic Mapping", "[converter]") {
    
    SECTION("Cube injects axis variation") {
        ZPlaneCube cube = PresetConverter::convertToCube(0, 0); // same shape, rely on procedural Y/Z

        const auto& base = cube.corners[0];
        const auto& freqShift = cube.corners[2]; // Y-axis high
        const auto& resShift  = cube.corners[4]; // Z-axis high

        REQUIRE(base.sections[0].k1 != Catch::Approx(freqShift.sections[0].k1));
        REQUIRE(base.sections[0].k2 != Catch::Approx(resShift.sections[0].k2));
    }

    SECTION("Cube morph differentiates X corners") {
        ZPlaneCube cube = PresetConverter::convertToCube(0, 1);

        float k2A = cube.corners[0].sections[0].k2;
        float k2B = cube.corners[1].sections[0].k2;

        REQUIRE(k2A != Catch::Approx(k2B));
    }
}
