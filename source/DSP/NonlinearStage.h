#pragma once
#include <cmath>

namespace nlin
{
    // simple smooth tanh approx; swap to chowdsp FastMath if desired
    inline float fastTanh (float x)
    {
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    inline void applyDrive (float* x, int N, float driveLin)
    {
        if (std::abs (driveLin - 1.0f) < 1e-6f) return;
        for (int i=0; i<N; ++i) x[i] *= driveLin;
    }

    inline void applySaturation (float* x, int N, float amount01)
    {
        if (amount01 <= 1e-6f) return;
        const float d = 1.0f + 3.0f * amount01;
        for (int i=0; i<N; ++i) x[i] = fastTanh (x[i] * d) / d;
    }
}