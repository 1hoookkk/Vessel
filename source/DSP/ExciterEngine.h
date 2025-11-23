/*
  ==============================================================================
    HEAVY - Exciter Engine
    Generates impact/strike bursts that excite the Z-Plane resonator.
    Velocity-sensitive, material-dependent.
  ==============================================================================
*/

#pragma once
#include "../HeavyConstants.h"
#include <cmath>

namespace Heavy
{
    // ═══════════════════════════════════════════════════════════════════════
    // EXCITER GENERATOR
    // Creates the initial "hit" that feeds into Z-Plane filter
    // ═══════════════════════════════════════════════════════════════════════
    class ExciterEngine
    {
    public:
        ExciterEngine()
        {
            reset();
        }

        void prepare(double sampleRate)
        {
            sampleRate_ = sampleRate;
            reset();
        }

        void reset()
        {
            phase_ = 0.0f;
            envelopeLevel_ = 0.0f;
            isActive_ = false;
        }

        // Trigger a new strike
        void trigger(ExciterType type, float velocity, float decayMs, float pitch)
        {
            type_ = type;
            velocity_ = juce::jlimit(0.0f, 1.0f, velocity);

            // Decay time in samples
            decaySamples_ = (decayMs / 1000.0f) * (float)sampleRate_;

            // Pitch for tone-based exciters (Hz)
            frequency_ = pitch;

            // Reset state
            phase_ = 0.0f;
            envelopeLevel_ = 1.0f;
            sampleCount_ = 0;
            isActive_ = true;
        }

        // Generate next sample
        float processSample()
        {
            if (!isActive_) return 0.0f;

            // Envelope (exponential decay)
            envelopeLevel_ = std::exp(-5.0f * (float)sampleCount_ / decaySamples_);

            if (envelopeLevel_ < 0.001f)
            {
                isActive_ = false;
                return 0.0f;
            }

            float sample = 0.0f;

            switch (type_)
            {
                case ExciterType::IMPULSE:
                    // Sharp click (only first sample)
                    sample = (sampleCount_ == 0) ? 1.0f : 0.0f;
                    break;

                case ExciterType::TONE_BURST:
                    // Sine wave burst
                    sample = std::sin(phase_);
                    phase_ += juce::MathConstants<float>::twoPi * frequency_ / (float)sampleRate_;
                    break;

                case ExciterType::NOISE_BURST:
                    // White noise
                    sample = (random_.nextFloat() * 2.0f) - 1.0f;
                    break;

                case ExciterType::DIRTY_NOISE:
                    // Saturated noise + distortion
                    sample = (random_.nextFloat() * 2.0f) - 1.0f;
                    sample = std::tanh(sample * 3.0f); // Soft clip
                    break;

                case ExciterType::MIXED:
                    // Hybrid: impulse + tone (velocity-dependent mix)
                    {
                        float impulse = (sampleCount_ == 0) ? 1.0f : 0.0f;
                        float tone = std::sin(phase_);
                        phase_ += juce::MathConstants<float>::twoPi * frequency_ / (float)sampleRate_;

                        // Soft hits = more impulse, hard hits = more tone
                        float toneMix = velocity_ * velocity_; // Quadratic
                        sample = impulse * (1.0f - toneMix) + tone * toneMix;
                    }
                    break;

                default:
                    break;
            }

            sampleCount_++;

            // Apply envelope and velocity scaling
            return sample * envelopeLevel_ * velocity_;
        }

        bool isActive() const { return isActive_; }

    private:
        double sampleRate_ = 44100.0;
        ExciterType type_ = ExciterType::IMPULSE;

        float velocity_ = 0.8f;
        float frequency_ = 200.0f;
        float decaySamples_ = 1000.0f;

        float phase_ = 0.0f;
        float envelopeLevel_ = 0.0f;
        int sampleCount_ = 0;
        bool isActive_ = false;

        juce::Random random_;
    };

} // namespace Heavy
