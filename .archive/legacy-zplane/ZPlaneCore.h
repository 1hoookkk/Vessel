#pragma once
#include "ZPlaneCommon.h"
#include "ZPlaneMath.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace zplane {

class Core {
public:
    Core() = default;

    void prepare(double sampleRate) {
        fs = (float)sampleRate;
        reset();
    }

    void reset() {
        for (auto& s : states) s = {};
        currentMorph = 0.0f;
        currentIntensity = 0.0f;
    }

    // Set the two shapes to morph between (e.g. Vowel A and Vowel B)
    void setShapes(const Shape& shapeA, const Shape& shapeB) {
        targetShapeA = shapeA;
        targetShapeB = shapeB;
    }

    // Set target parameters for smoothing
    // morph: 0.0 = Shape A, 1.0 = Shape B
    // intensity: 0.0 = bypass/flat, 1.0 = full effect
    void setTargets(float morph, float intensity) {
        currentMorph = morph;
        currentIntensity = intensity;
    }

    // Main processing block
    void processBlock(juce::AudioBuffer<float>& buffer) {
        // Update coefficients once per block (or every N samples if needed)
        // Using direct values for immediate response
        
        // Calculate coefficients for this block
        updateCoefficients(currentMorph, currentIntensity);

        // Process audio
        // Overload for raw pointers (Mono)
        processBlock(buffer.getWritePointer(0), buffer.getNumSamples());
    }
    
    // Overload for raw pointers (Mono)
    void processBlock(float* data, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float x = data[i];
            // Unrolled loop with check or switch?
            // For max performance, we might want template or fixed unroll.
            // But 'activeSections' is runtime.
            // Switch case fallthrough or simple loop?
            // Unrolled 6 stages is requested.
            // If we want to skip, we can just set unused coeffs to identity (b0=1, a1=0, a2=0).
            // Then we can always run 6 stages.
            // This is branchless and stable.
            
            x = processBiquad(x, states[0], coeffs.stages[0]);
            x = processBiquad(x, states[1], coeffs.stages[1]);
            x = processBiquad(x, states[2], coeffs.stages[2]);
            x = processBiquad(x, states[3], coeffs.stages[3]);
            x = processBiquad(x, states[4], coeffs.stages[4]);
            x = processBiquad(x, states[5], coeffs.stages[5]);
            data[i] = x;
        }
    }
    
    void setActiveSections(int n) {
        activeSections = std::clamp(n, 0, 6);
        // Reset unused coeffs to identity?
        // updateCoefficients will handle active ones.
        // We should ensure others are identity.
    }

    // Manual coefficient update
    void updateCoefficients(float morph, float intensity) {
        for (int s = 0; s < 6; ++s) {
            if (s >= activeSections) {
                // Set to identity
                coeffs.stages[s] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
                continue;
            }

            // Interpolate poles
            Pole p = interpolate(targetShapeA.poles[s], targetShapeB.poles[s], morph);
            
            // Apply intensity
            float rMod = std::clamp(p.r * (0.80f + 0.20f * intensity), 0.1f, 0.9995f);
            p.r = rMod;
            
            // Store for visualization
            currentPoles[s] = p;

            // Convert to coeffs
            coeffs.stages[s] = poleToCoeffs(p);
        }
    }
    
    const CascadeCoeffs& getCoeffs() const { return coeffs; }
    const std::array<Pole, 6>& getPoles() const { return currentPoles; }

private:
    Shape targetShapeA;
    Shape targetShapeB;
    CascadeCoeffs coeffs;
    std::array<BiquadState, 6> states;
    std::array<Pole, 6> currentPoles;
    int activeSections = 6;
    
    float currentMorph = 0.0f;
    float currentIntensity = 0.0f;
    float fs = 48000.0f;
};

} // namespace zplane
