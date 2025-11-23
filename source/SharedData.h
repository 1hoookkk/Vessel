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
    // 14-pole (7-stage biquad) filter state
    std::array<zplane::Pole, 7> leftPoles;
    std::array<zplane::Pole, 7> rightPoles;
    
    // Simulation State
    ParticleData particles;
    
    // Real-time Diagnostics & Metering
    float outputRMS = 0.0f;     // For reactive wireframe scaling
    float outputPeak = 0.0f;    // For clipping detection
    int modeIndex = 0;
    float centroidX = 0.5f; // Tension (Input)
    float centroidY = 0.5f; // Hardness (Input)
    
    // Z-Plane Engine State (Result)
    float morph = 0.0f;
    float freq = 0.0f;
    float transformZ = 0.0f;
    int morphPair = 0;
};
