#pragma once

#include <array>
#include <cmath>
#include <algorithm>
#include <juce_dsp/juce_dsp.h>

class ChaosModulator
{
public:
    ChaosModulator() noexcept = default;

    void reset(float sampleRate) noexcept
    {
        fs = sampleRate;
        // Initialize state to a non-zero point to avoid stuck-at-zero
        state = { 1.0f, 1.0f, 1.0f };
        phase = 0.0f;
        
        // Set default parameters
        setParameters(1.0f, 0.5f, 0.0f);
    }

    void setParameters(float speed, float chaosAmount, float drift) noexcept
    {
        // Map speed to time step (dt). We want a small dt for stability, but adjustable for speed.
        // We'll map speed [0,1] to dt [0.0001, 0.01] (or similar). Adjust as needed.
        dt = 0.0001f + 0.0099f * speed;

        // Map chaosAmount to rho (Rayleigh number). Typical Lorenz: rho=28 for chaos, but we can vary.
        // We'll map [0,1] to [10, 40] for example.
        rho = 10.0f + 30.0f * chaosAmount;

        // Drift parameter: we'll use it to modulate sigma or beta slowly.
        driftAmount = drift;
    }

    std::array<float, 3> getNextSample() noexcept
    {
        // Update the drift LFO (very slow, in the order of minutes)
        phase += driftLFORate / fs;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Calculate drifted parameters
        float driftedSigma = sigma + driftAmount * 2.0f * std::sin(2.0f * 3.14159f * phase);
        float driftedBeta = beta + driftAmount * 2.0f * std::cos(2.0f * 3.14159f * phase);

        // Perform one RK4 step
        auto k1 = lorenz(state, driftedSigma, rho, driftedBeta);
        auto k2 = lorenz(state + 0.5f * dt * k1, driftedSigma, rho, driftedBeta);
        auto k3 = lorenz(state + 0.5f * dt * k2, driftedSigma, rho, driftedBeta);
        auto k4 = lorenz(state + dt * k3, driftedSigma, rho, driftedBeta);

        state += (dt / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);

        // Normalize and clamp the outputs to [0,1]
        // The raw Lorenz output ranges: X/Y ~ [-20,20], Z ~ [0,50]
        float normX = (state[0] + 20.0f) / 40.0f;
        float normY = (state[1] + 20.0f) / 40.0f;
        float normZ = state[2] / 50.0f;

        normX = std::clamp(normX, 0.0f, 1.0f);
        normY = std::clamp(normY, 0.0f, 1.0f);
        normZ = std::clamp(normZ, 0.0f, 1.0f);

        return { normX, normY, normZ };
    }

private:
    // Lorenz system parameters (classic values for chaos, but we vary rho)
    float sigma = 10.0f;
    float beta = 8.0f / 3.0f;

    // Current state [x, y, z]
    std::array<float, 3> state = { 0.0f, 0.0f, 0.0f };

    // Time step and sample rate
    float dt = 0.001f;
    float fs = 48000.0f;

    // Parameters that can be set by the user
    float rho = 28.0f;
    float driftAmount = 0.0f;

    // Drift LFO
    float phase = 0.0f;
    // Very slow LFO: period of 60 seconds (1/60 Hz)
    float driftLFORate = 1.0f / 60.0f;

    // Lorenz system derivative function
    static std::array<float, 3> lorenz(const std::array<float, 3>& s, float s_sigma, float s_rho, float s_beta) noexcept
    {
        return {
            s_sigma * (s[1] - s[0]),
            s[0] * (s_rho - s[2]) - s[1],
            s[0] * s[1] - s_beta * s[2]
        };
    }
};
