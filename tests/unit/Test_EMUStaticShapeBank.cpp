#include <catch2/catch_test_macros.hpp>
#include "DSP/EMUStaticShapeBank.h"

TEST_CASE("EMUStaticShapeBank Structure and Data", "[unit][dsp]") {
    EMUStaticShapeBank bank;

    SECTION("Bank Dimensions are Correct") {
        REQUIRE(bank.numShapes() == 32); // 32 Authentic Shapes defined in tables
        REQUIRE(bank.numPairs() > 0);
    }

    SECTION("Shape Access is Safe") {
        // Check boundary access
        const auto& s0 = bank.shape(0);
        REQUIRE(s0[0] > 0.0f); // Radius should be positive

        const auto& sLast = bank.shape(bank.numShapes() - 1);
        REQUIRE(sLast[0] > 0.0f);

        // Check out of bounds (should return default/safe)
        const auto& sOOB = bank.shape(999);
        REQUIRE(sOOB[0] == bank.shape(0)[0]);
    }

    SECTION("Morph Pairs are Valid") {
        auto pair = bank.morphPairIndices(0);
        REQUIRE(pair.first >= 0);
        REQUIRE(pair.second >= 0);
        REQUIRE(pair.first < bank.numShapes());
        REQUIRE(pair.second < bank.numShapes());
    }
}
