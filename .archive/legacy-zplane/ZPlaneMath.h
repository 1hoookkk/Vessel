#pragma once
#include "ZPlaneCommon.h"
#include "ZPoleMath.h" // Reuse existing robust math
#include <cmath>
#include <algorithm>

namespace zplane {

// Wrapper around existing ZPoleMath for consistency
inline Pole interpolate(const Pole& A, const Pole& B, float t) {
    auto [r, th] = zpm::interpolatePoleLogSpace(A.r, A.theta, B.r, B.theta, t);
    return { r, th };
}

// Convert pole (r, theta) to Biquad coefficients (b0, a1, a2)
// Applies bandpass normalization (gain at peak = 1.0)
inline BiquadCoeffs poleToCoeffs(const Pole& p) {
    BiquadCoeffs c;
    
    // Standard Z-plane mapping
    // H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
    
    c.a1 = -2.0f * p.r * std::cos(p.theta);
    c.a2 = p.r * p.r;

    // Bandpass normalization (zeros at DC and Nyquist)
    // Boosted gain for audibility in cascade
    c.b0 = (1.0f - p.r); 
    c.b1 = 0.0f;
    c.b2 = -c.b0;

    return c;
}

// Optimized single biquad process
// Transposed Direct Form II
inline float processBiquad(float input, BiquadState& state, const BiquadCoeffs& coeffs) {
    // y[n] = b0*x[n] + s1[n-1]
    // s1[n] = s2[n-1] + b1*x[n] - a1*y[n]
    // s2[n] = b2*x[n] - a2*y[n]
    
    float output = coeffs.b0 * input + state.z1;
    
    state.z1 = state.z2 + coeffs.b1 * input - coeffs.a1 * output;
    state.z2 = coeffs.b2 * input - coeffs.a2 * output;
    
    return output;
}

} // namespace zplane
