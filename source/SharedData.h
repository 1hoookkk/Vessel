#pragma once
#include "DSP/ZPlaneCommon.h"
#include <array>

// Central State Definition for Audio-to-UI Transport
// Designed to be Triple-Buffered (Wait-Free)

struct ParticleData {
    // Fixed size for safety/static allocation. 
    static constexpr int kMaxParticles = 512;
    
    // Structure of Arrays (SoA) layout for future SIMD readiness
    std::array<float, kMaxParticles> x;
    std::array<float, kMaxParticles> y;
    int activeCount = 0;
};

struct SystemState {
    // 14-pole (6-stage biquad) filter state
    std::array<zplane::Pole, 6> leftPoles;
    std::array<zplane::Pole, 6> rightPoles;
    
    // Simulation State
    ParticleData particles;
    
    // Real-time Diagnostics
    float kineticEnergy = 0.0f;
    int modeIndex = 0;
    float centroidX = 0.5f; // Morph
    float centroidY = 0.5f; // Freq
    float transformZ = 0.0f; // Trans
};
