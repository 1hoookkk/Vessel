#pragma once
#include <immintrin.h>
#include <cmath>

/*
  ==============================================================================
    VESSEL // System_A01
    File: AVX2_SPH_Kernels.h
    Desc: SIMD-Optimized SPH Kernels for 2D Fluid Dynamics.
    
    Target Architecture: x86_64 (AVX2/FMA)
    Registers: YMM (256-bit, 8 floats)
  ==============================================================================
*/

namespace corium {
namespace simd {

// Constants for 2D SPH (Müller et al., adapted for 2D)
// h = Smoothing Length
// Assuming h is constant for the simulation, we precompute these.

struct KernelConstants {
    float poly6_coeff;
    float spiky_grad_coeff;
    float h2; // h^2
    float h;  // h
    
    KernelConstants(float hVal) : h(hVal) {
        float pi = 3.14159265359f;
        h2 = h * h;
        
        // Poly6 (Density): 4 / (pi * h^8)
        poly6_coeff = 4.0f / (pi * std::pow(h, 8.0f));
        
        // Spiky Gradient (Pressure): -10 / (pi * h^5)
        // Note: The gradient vector is normalized by distance r.
        // F = -Sum [ mass * (pi + pj)/2rho * Gradient ]
        // Grad W = spiky_grad_coeff * (h-r)^3 * (r_vec / r)
        spiky_grad_coeff = -10.0f / (pi * std::pow(h, 5.0f));
    }
};

// -----------------------------------------------------------------------------
// DENSITY KERNEL (Poly6)
// W(r, h) = Coeff * (h^2 - r^2)^3
// -----------------------------------------------------------------------------
inline __m256 eval_poly6_density(__m256 r2, __m256 h2, __m256 poly6_coeff) {
    // Mask: r2 < h2
    __m256 mask = _mm256_cmp_ps(r2, h2, _CMP_LT_OQ);
    
    // term = h2 - r2
    __m256 term = _mm256_sub_ps(h2, r2);
    
    // term^3
    __m256 term2 = _mm256_mul_ps(term, term);
    __m256 term3 = _mm256_mul_ps(term2, term);
    
    // result = coeff * term^3
    __m256 res = _mm256_mul_ps(poly6_coeff, term3);
    
    // Apply mask (set to 0.0 if r >= h)
    return _mm256_and_ps(mask, res);
}

// -----------------------------------------------------------------------------
// PRESSURE FORCE (Spiky Gradient)
// F = -grad(P)
// Returns Force Magnitude / r. 
// Caller must multiply by dx, dy to get vector components.
// Gradient W = coeff * (h - r)^3 * (1/r) * r_vec
// We compute: coeff * (h - r)^3 / r
// -----------------------------------------------------------------------------
inline __m256 eval_spiky_force_factor(__m256 r2, __m256 h_vec, __m256 spiky_coeff) {
    // r = sqrt(r2)
    __m256 r = _mm256_sqrt_ps(r2);
    
    // Mask: r < h AND r > epsilon (avoid div by zero)
    __m256 mask_h = _mm256_cmp_ps(r, h_vec, _CMP_LT_OQ);
    __m256 epsilon = _mm256_set1_ps(1e-5f);
    __m256 mask_eps = _mm256_cmp_ps(r, epsilon, _CMP_GT_OQ);
    __m256 mask = _mm256_and_ps(mask_h, mask_eps);
    
    // term = (h - r)
    __m256 term = _mm256_sub_ps(h_vec, r);
    
    // term^3
    __m256 term2 = _mm256_mul_ps(term, term);
    __m256 term3 = _mm256_mul_ps(term2, term);
    
    // Force Scalar = coeff * term^3
    __m256 force = _mm256_mul_ps(spiky_coeff, term3);
    
    // Divide by r (standard SPH gradient normalization)
    // Using approx rsqrt for speed, then one Newton-Raphson iteration? 
    // Or simple div for precision. div is slower but safer.
    // Let's use div for now.
    __m256 force_div_r = _mm256_div_ps(force, r);
    
    // Apply mask
    return _mm256_and_ps(mask, force_div_r);
}

} // namespace simd
} // namespace corium
