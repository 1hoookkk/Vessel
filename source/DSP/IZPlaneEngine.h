#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "ZPlaneCommon.h"

struct ZPlaneParams
{
    int   morphPair = 0;     // index into MORPH_PAIRS
    float morph     = 0.0f;  // X-Axis [0..1]
    float freq      = 0.0f;  // Y-Axis [0..1]
    float trans     = 0.0f;  // Z-Axis [0..1]
    
    float intensity = 0.0f;  // Legacy/Deprecated
    float driveDb   = 0.0f;  // Legacy/Deprecated
    float sat       = 0.0f;  // [0..1]
    float lfoRate   = 0.0f;  // Hz
    float lfoDepth  = 0.0f;  // [0..1]
    bool  autoMakeup= false;

    // Extended parameters for production use
    float radiusGamma     = 1.0f;  // pole radius scaling
    float postTiltDbPerOct= 0.0f;  // spectral tilt compensation
    float driveHardness   = 0.5f;  // drive characteristic

    // Formant-pitch coupling
    bool  formantLock     = true;  // true=Lock formants, false=Follow pitch
    float pitchRatio      = 1.0f;  // current pitch shift ratio for formant compensation
};

struct IZPlaneEngine
{
    virtual ~IZPlaneEngine() = default;

    virtual void prepare (double fs, int blockSize, int numChannels) = 0;
    virtual void reset() = 0;
    virtual void setParams (const ZPlaneParams&) = 0;

    // base-rate linear cascade (always)
    virtual void processLinear   (juce::AudioBuffer<float>& wet) = 0;
    
    // Raw pointer interface (Modern / JUCE-free)
    virtual void process_block(float* left, float* right, int numSamples) = 0;

    // nonlinear stage (drive/saturation), may be called at base or OS rate
    virtual void processNonlinear(juce::AudioBuffer<float>& wet) = 0;

    // for OS wrapper
    virtual void setProcessingSampleRate (double fs) = 0;

    // intensity≈0 & drive≈1 & sat≈0 & lfoDepth≈0
    virtual bool isEffectivelyBypassed() const = 0;
    
    // Visualization
    virtual std::array<zplane::Pole, 6> getLeftPoles() const = 0;
    virtual std::array<zplane::Pole, 6> getRightPoles() const = 0;
};