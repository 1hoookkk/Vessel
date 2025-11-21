#pragma once
#include "IShapeBank.h"
#include "EMUAuthenticTables.h" // Contains the actual shape data

class EMUStaticShapeBank final : public IShapeBank
{
public:
    std::pair<int, int> morphPairIndices(int pairIndex) const override
    {
        if (pairIndex >= 0 && pairIndex < AUTHENTIC_EMU_NUM_PAIRS)
            return { MORPH_PAIRS[pairIndex][0], MORPH_PAIRS[pairIndex][1] };
        return { 0, 0 }; // Default to first pair
    }

    const std::array<float, 12>& shape(int index) const override
    {
        const auto& bank = getShapeBank();
        if (index >= 0 && index < AUTHENTIC_EMU_NUM_SHAPES)
            return bank[static_cast<size_t>(index)];

        return bank[0]; // Default to first shape
    }

    int numPairs() const override { return AUTHENTIC_EMU_NUM_PAIRS; }
    int numShapes() const override { return AUTHENTIC_EMU_NUM_SHAPES; }

private:
    // Materialize the C tables into safe std::array storage once.
    static const std::array<std::array<float, 12>, AUTHENTIC_EMU_NUM_SHAPES>& getShapeBank()
    {
        static const std::array<std::array<float, 12>, AUTHENTIC_EMU_NUM_SHAPES> bank = [] {
            std::array<std::array<float, 12>, AUTHENTIC_EMU_NUM_SHAPES> out{};
            for (int i = 0; i < AUTHENTIC_EMU_NUM_SHAPES; ++i)
                for (int j = 0; j < 12; ++j)
                    out[static_cast<size_t>(i)][static_cast<size_t>(j)] = AUTHENTIC_EMU_SHAPES[i][j];
            return out;
        }();

        return bank;
    }
};
