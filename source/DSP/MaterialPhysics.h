/*
  ==============================================================================
    HEAVY - Material Physics Engine
    Maps material types to DSP parameters and Z-Plane configurations.
    Each material has unique resonant fingerprint.
  ==============================================================================
*/

#pragma once
#include "../HeavyConstants.h"
#include "PresetConverter.h"
#include "EMUAuthenticTables.h"

namespace Heavy
{
    // ═══════════════════════════════════════════════════════════════════════
    // MATERIAL PROPERTIES
    // Physical characteristics that define acoustic behavior
    // ═══════════════════════════════════════════════════════════════════════
    struct MaterialProperties
    {
        // Physics
        float basePitch;        // Base frequency multiplier (0.5 = -1 oct, 2.0 = +1 oct)
        float decay;            // Base decay time in seconds
        float damping;          // High-frequency loss (0.0 = none, 1.0 = max)
        float resonance;        // Q factor / pole radius (0.0-1.0)
        float harmonicity;      // 0.0 = noise-like, 1.0 = pure harmonic

        // DSP Mapping
        int shapeA;             // EMU preset index A (0-31)
        int shapeB;             // EMU preset index B (0-31)
        float morphPosition;    // Position between A/B (0.0-1.0)

        // Exciter
        ExciterType exciter;
        float exciterDecay;     // Exciter envelope decay (ms)

        // Saturation
        float saturation;       // Nonlinear drive amount

        // Factory function
        static MaterialProperties get(MaterialType type);
    };

    // ═══════════════════════════════════════════════════════════════════════
    // MATERIAL DEFINITIONS
    // Hand-tuned configurations for 8 materials
    // ═══════════════════════════════════════════════════════════════════════
    inline MaterialProperties MaterialProperties::get(MaterialType type)
    {
        switch (type)
        {
            case MaterialType::IRON:
                // Damp, heavy, sub-bass (808 kick character)
                return {
                    0.6f,           // basePitch (lower)
                    0.8f,           // decay (short)
                    0.7f,           // damping (dead)
                    0.3f,           // resonance (low Q)
                    0.5f,           // harmonicity (mix)
                    0,              // shapeA: Vowel_Ae (tight low end)
                    12,             // shapeB: Vowel_Ih
                    0.2f,           // morph (closer to A)
                    ExciterType::TONE_BURST,
                    8.0f,           // exciterDecay
                    0.2f            // saturation (clean)
                };

            case MaterialType::STEEL:
                // Punchy, mid-range (snare body)
                return {
                    1.0f,           // basePitch (neutral)
                    1.2f,           // decay (medium)
                    0.4f,           // damping (moderate)
                    0.5f,           // resonance (medium Q)
                    0.6f,           // harmonicity
                    7,              // shapeA: Bell_Metallic
                    5,              // shapeB: Metallic
                    0.5f,           // morph (centered)
                    ExciterType::MIXED,
                    12.0f,
                    0.3f
                };

            case MaterialType::BRASS:
                // Resonant, warm (harmonic series, bell-like)
                return {
                    1.0f,
                    2.5f,           // decay (long ring)
                    0.2f,           // damping (bright)
                    0.7f,           // resonance (high Q)
                    0.9f,           // harmonicity (tonal)
                    7,              // Bell_Metallic
                    5,              // Metallic
                    0.8f,           // morph (more bell)
                    ExciterType::TONE_BURST,
                    20.0f,
                    0.1f            // saturation (clean)
                };

            case MaterialType::ALUMINUM:
                // Dry, short, bright (rim shot)
                return {
                    1.4f,           // basePitch (higher)
                    0.4f,           // decay (very short)
                    0.8f,           // damping (dead)
                    0.2f,           // resonance (low Q)
                    0.4f,           // harmonicity (clicky)
                    3,              // Resonant Peak
                    4,              // Wide Spectrum
                    0.1f,           // morph (tight)
                    ExciterType::IMPULSE,
                    5.0f,
                    0.1f
                };

            case MaterialType::GLASS:
                // Brittle, high freq (modal shatter, cymbal-like)
                return {
                    2.0f,           // basePitch (+1 oct)
                    1.8f,           // decay (medium-long)
                    0.1f,           // damping (bright, airy)
                    0.85f,          // resonance (very high Q)
                    0.7f,           // harmonicity
                    3,              // Resonant Peak
                    18,             // Formant Filter
                    0.7f,
                    ExciterType::NOISE_BURST,
                    15.0f,
                    0.05f
                };

            case MaterialType::QUARTZ:
                // Clean, piercing, singing (crystal resonance)
                return {
                    1.8f,           // basePitch
                    3.5f,           // decay (very long sustain)
                    0.05f,          // damping (minimal loss)
                    0.95f,          // resonance (near self-oscillation)
                    0.98f,          // harmonicity (pure sine)
                    16,             // Classic Sweep
                    31,             // Ultimate
                    0.6f,
                    ExciterType::TONE_BURST,
                    25.0f,
                    0.0f            // saturation (pristine)
                };

            case MaterialType::CARBON:
                // Fiber, composite, synthetic (phase cancellation, FM-like)
                return {
                    1.2f,
                    1.0f,
                    0.5f,
                    0.6f,
                    0.3f,           // harmonicity (inharmonic)
                    1,              // Vocal Morph
                    8,              // Aggressive Lead
                    0.5f,
                    ExciterType::MIXED,
                    10.0f,
                    0.5f            // saturation (moderate)
                };

            case MaterialType::URANIUM:
                // Unstable, distorted (chaos + saturation, radioactive)
                return {
                    0.8f,
                    2.0f,
                    0.3f,
                    0.75f,          // resonance (unstable)
                    0.1f,           // harmonicity (chaotic)
                    1,              // Vocal Morph
                    8,              // Aggressive Lead
                    0.9f,           // morph (extreme)
                    ExciterType::DIRTY_NOISE,
                    18.0f,
                    0.9f            // saturation (heavy distortion)
                };

            default:
                return get(MaterialType::STEEL); // Fallback
        }
    }

} // namespace Heavy
