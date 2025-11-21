#pragma once
#include <array>
#include <utility>

struct IShapeBank
{
    virtual ~IShapeBank() = default;

    virtual std::pair<int,int>       morphPairIndices (int pairIndex) const = 0;
    virtual const std::array<float,12>& shape         (int index)     const = 0;
    virtual int numPairs()  const = 0;
    virtual int numShapes() const = 0;
};