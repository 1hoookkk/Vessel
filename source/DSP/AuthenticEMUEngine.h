#pragma once
#include "IZPlaneEngine.h"
#include "IShapeBank.h"
#include "EmuZPlaneCore.h" // New Brain
#include "PresetConverter.h" // The Bridge
#include "NonlinearStage.h"
#include <cstdint>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

class AuthenticEMUEngine final : public IZPlaneEngine
{
public:
    explicit AuthenticEMUEngine (IShapeBank& bank) : shapes (bank) {}

    void prepare (double fs, int blockSize, int numChannels) override
    {
        fsHost = (float) fs;
        engineL.setSampleRate(fsHost);
        engineR.setSampleRate(fsHost);
        
        // Pre-allocate buffers if needed
        
        reset();
        updateCube();
    }

    void reset() override
    {
        engineL.reset();
        engineR.reset();
    }

    void setParams (const ZPlaneParams& p) override
    {
        if (p.morphPair != params.morphPair) {
            params = p; // Update first so updateCube uses new index
            updateCube();
        } else {
            params = p;
        }
    }
    
    void setProcessingSampleRate (double fs) override { 
        fsHost = (float) fs;
        engineL.setSampleRate(fsHost);
        engineR.setSampleRate(fsHost);
    }

    bool isEffectivelyBypassed() const override
    {
        return false; // Always on
    }

    void processNonlinear (juce::AudioBuffer<float>& buffer) override
    {
        // Pass-through: The ZPlaneFilter handles saturation
    }

    void process_block(float* left, float* right, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Zero-allocation processing using raw pointers
        
        // Process L
        engineL.process_block(currentCube, params.morph, params.freq, params.trans, left, left, numSamples);
        
        // Process R
        engineR.process_block(currentCube, params.morph, params.freq, params.trans, right, right, numSamples);
    }

    void processLinear (juce::AudioBuffer<float>& buffer) override
    {
        const int numSamples = buffer.getNumSamples();
        if (buffer.getNumChannels() < 2)
            return;
        
        auto* l = buffer.getWritePointer(0);
        auto* r = buffer.getWritePointer(1);
        
        process_block(l, r, numSamples);
    }

    std::array<zplane::Pole, 6> getLeftPoles() const override
    {
        // Approximate poles from current interpolation
        // This is for the legacy UI.
        return interpolatePoles(params.morph);
    }

    std::array<zplane::Pole, 6> getRightPoles() const override
    {
        return interpolatePoles(params.morph);
    }

private:
    IShapeBank& shapes;
    ZPlaneParams params;
    float fsHost = 48000.0f;

    // Stereo Engines
    EmuHChipFilter engineL;
    EmuHChipFilter engineR;
    
    ZPlaneCube currentCube;

    void updateCube() {
        auto pair = shapes.morphPairIndices(params.morphPair);
        currentCube = PresetConverter::convertToCube(pair.first, pair.second);
    }
    
    std::array<zplane::Pole, 6> interpolatePoles(float t) const {
        // Recover poles from the k-space cube for visualization
        // This is a rough approximation to keep the old UI working.
        std::array<zplane::Pole, 6> out;
        
        // Validate sample rate to prevent division by zero
        float safeSampleRate = fsHost > 0.0f ? fsHost : 48000.0f;
        
        // We only need to look at the first 6 sections
        // X-axis interpolation
        ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(currentCube, t, 0.0f, 0.0f);
        
        for (int i = 0; i < 6; ++i) {
            // Reverse map k1 -> Freq -> Theta
            float k1 = frame.sections[i].k1;
            float freqHz = 20.0f * std::pow(1000.0f, k1);
            float theta = 2.0f * 3.14159f * freqHz / safeSampleRate;
            
            // Reverse map k2 -> Radius
            float k2 = frame.sections[i].k2;
            float r = k2 * 0.98f;
            
            out[i] = {r, theta};
        }
        return out;
    }
};
