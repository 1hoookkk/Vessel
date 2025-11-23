/*
  ==============================================================================
    HEAVY - Industrial Percussion Synthesizer
    Core constants, enums, and color palette.
    Vantablack Industrial aesthetic.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace Heavy
{
    // ═══════════════════════════════════════════════════════════════════════
    // MATERIAL TYPES (8 Discrete Alloys)
    // ═══════════════════════════════════════════════════════════════════════
    enum class MaterialType : int
    {
        IRON = 0,      // Damp, Heavy, Sub-Bass
        STEEL,         // Punchy, Mid-range (snare body)
        BRASS,         // Resonant, Warm (harmonic series)
        ALUMINUM,      // Dry, Short, Bright
        GLASS,         // Brittle, High Freq (modal shatter)
        QUARTZ,        // Clean, Piercing, Singing (high Q)
        CARBON,        // Fiber, Composite, Synthetic (phase cancellation)
        URANIUM        // Unstable, Distorted (chaos + saturation)
    };

    inline const char* getMaterialName(MaterialType type)
    {
        switch (type)
        {
            case MaterialType::IRON:     return "IRON";
            case MaterialType::STEEL:    return "STEEL";
            case MaterialType::BRASS:    return "BRASS";
            case MaterialType::ALUMINUM: return "ALUMINUM";
            case MaterialType::GLASS:    return "GLASS";
            case MaterialType::QUARTZ:   return "QUARTZ";
            case MaterialType::CARBON:   return "CARBON";
            case MaterialType::URANIUM:  return "URANIUM";
            default:                     return "UNKNOWN";
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // EXCITER TYPES (Impact Sound)
    // ═══════════════════════════════════════════════════════════════════════
    enum class ExciterType
    {
        IMPULSE,        // Sharp click (Dirac-like)
        TONE_BURST,     // Tuned sine burst (pitched mallet)
        NOISE_BURST,    // White noise burst (snare wire)
        DIRTY_NOISE,    // Saturated noise + distortion (chaos)
        MIXED           // Hybrid impulse + tone
    };

    // ═══════════════════════════════════════════════════════════════════════
    // COLOR PALETTE (Vantablack Industrial)
    // ═══════════════════════════════════════════════════════════════════════
    namespace Palette
    {
        const juce::Colour Chassis      = juce::Colour(0xFF111111); // Black Iron
        const juce::Colour Recess       = juce::Colour(0xFF050505); // Vantablack (Screen/Slots)
        const juce::Colour Text         = juce::Colour(0xFFE0E0E0); // Laser Etched White
        const juce::Colour TextDim      = juce::Colour(0xFF707070); // Dimmed labels
        const juce::Colour PhosphorOff  = juce::Colour(0xFF1A2E1A); // Dim radar trace
        const juce::Colour PhosphorOn   = juce::Colour(0xFF00FF41); // Matrix Green (Active)
        const juce::Colour Warning      = juce::Colour(0xFFFF3300); // Safety Orange (Clip/Limit)
        const juce::Colour Knurled      = juce::Colour(0xFF2A2A2A); // Knob/slider surface
    }

    // ═══════════════════════════════════════════════════════════════════════
    // TYPOGRAPHY
    // ═══════════════════════════════════════════════════════════════════════
    inline juce::Font getIndustrialFont(float size, bool bold = false)
    {
        int style = bold ? juce::Font::bold : juce::Font::plain;
        return juce::Font(juce::FontOptions("Courier New", size, style));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // SPACING TOKENS
    // ═══════════════════════════════════════════════════════════════════════
    constexpr int kGridUnit = 4;
    constexpr int kPadSmall = kGridUnit * 2;   // 8px
    constexpr int kPadMedium = kGridUnit * 4;  // 16px
    constexpr int kPadLarge = kGridUnit * 6;   // 24px
    constexpr int kBorderWidth = 1;

} // namespace Heavy
