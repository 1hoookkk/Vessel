#pragma once
#include <array>
#include <cstdint>
#include <cmath>

namespace zplane {

// Represents a single conjugate pole pair (z-plane)
struct Pole {
    float r;     // Radius [0..1)
    float theta; // Angle [0..pi]
};

// Represents a complete Z-Plane filter configuration (12th order = 6 biquads)
struct Shape {
    std::array<Pole, 6> poles;

    // Helper to convert flat array (e.g., from tables) to Shape
    // Expected layout: [r0, theta0, r1, theta1, ..., r5, theta5]
    static Shape fromFlat(const float* data) {
        Shape s;
        for (int i = 0; i < 6; ++i) {
            s.poles[i] = { data[i * 2], data[i * 2 + 1] };
        }
        return s;
    }
};

// Runtime state for a single biquad
struct BiquadState {
    float z1 = 0.0f;
    float z2 = 0.0f;
};

// Coefficients for a single biquad
struct BiquadCoeffs {
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
};

// Collection of coefficients for the full cascade
struct CascadeCoeffs {
    std::array<BiquadCoeffs, 6> stages;
};

} // namespace zplane

// Legacy / Visualization Types (Moved from BiquadCascade.h)
// Kept to preserve public API of AuthenticEMUEngine
struct BiquadSection
{
    float b0=1, b1=0, b2=0, a1=0, a2=0, z1=0, z2=0;
    inline void  reset() noexcept { z1 = z2 = 0; }
    inline float tick (float x) noexcept
    {
        const float y = b0*x + z1;
        z1 = b1*x - a1*y + z2;
        z2 = b2*x - a2*y;

        // Denormal protection
        constexpr float denormEps = 1.0e-20f;
        if (std::abs(z1) < denormEps) z1 = 0.0f;
        if (std::abs(z2) < denormEps) z2 = 0.0f;

        return y;
    }
};

struct BiquadCascade6
{
    BiquadSection s[6];
    void  reset() noexcept { for (auto& q : s) q.reset(); }
    float processSample (float x) noexcept
    {
        for (int i=0; i<6; ++i) x = s[i].tick (x);
        return x;
    }
    static inline void poleToBandpass (float r, float th, BiquadSection& sec)
    {
        float a1 = -2.0f * r * std::cos (th);
        float a2 =  r * r;
        float b0 = (1.0f - r) * 0.5f;

        // clamps for stability in cascade
        a1 = std::fmax (-1.999f, std::fmin (1.999f, a1));
        a2 = std::fmax (-0.999f, std::fmin (0.999f, a2));

        sec.a1 = a1;  sec.a2 = a2;
        sec.b0 = b0;  sec.b1 = 0.0f;  sec.b2 = -b0; // zeros at DC & Nyquist
    }
};
