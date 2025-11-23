/*
  ==============================================================================
    HEAVY - Industrial Look & Feel
    Knurled knobs and recessed sliders for brutal industrial aesthetic.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../HeavyConstants.h"

class HeavyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HeavyLookAndFeel()
    {
        using namespace Heavy::Palette;
        setColour(juce::Slider::rotarySliderFillColourId, PhosphorOn);
        setColour(juce::Slider::thumbColourId, Chassis);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // ROTARY KNOB (Massive Knurled Aluminum)
    // ═══════════════════════════════════════════════════════════════════════
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        using namespace Heavy::Palette;

        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(4.0f);
        auto center = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.48f;

        // 1. Drop shadow (depth)
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillEllipse(center.x - radius, center.y - radius + 4.0f, radius * 2.0f, radius * 2.0f);

        // 2. Knurled body (gradient for 3D effect)
        juce::ColourGradient grad(juce::Colour(0xFF2A2A2A), center.x - radius, center.y - radius,
                                  juce::Colour(0xFF111111), center.x + radius, center.y + radius, true);
        g.setGradientFill(grad);
        g.fillEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

        // Knurling pattern (radial grooves)
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        for (int i = 0; i < 48; ++i)
        {
            float angle = juce::MathConstants<float>::twoPi * ((float)i / 48.0f);
            float rx = std::cos(angle);
            float ry = std::sin(angle);
            g.drawLine(center.x + rx * (radius * 0.7f), center.y + ry * (radius * 0.7f),
                      center.x + rx * radius, center.y + ry * radius, 1.5f);
        }

        // 3. Position indicator (laser-etched line)
        float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        float lineLen = radius * 0.8f;
        float ix = center.x + std::cos(angle) * lineLen;
        float iy = center.y + std::sin(angle) * lineLen;

        g.setColour(Text);
        g.drawLine(center.x, center.y, ix, iy, 2.5f);

        // 4. Center cap (machined)
        float capRad = radius * 0.15f;
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillEllipse(center.x - capRad, center.y - capRad, capRad * 2.0f, capRad * 2.0f);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // LINEAR SLIDER (Recessed Industrial Slot)
    // ═══════════════════════════════════════════════════════════════════════
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        using namespace Heavy::Palette;

        auto track = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);

        // 1. Recessed slot background
        g.setColour(Recess);
        g.fillRect(track);

        // Border highlights (emboss effect)
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawLine(track.getX(), track.getY(), track.getX(), track.getBottom(), 1.0f); // Left highlight
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(track.getRight(), track.getY(), track.getRight(), track.getBottom(), 1.0f); // Right shadow

        // 2. Thumb (matte black block)
        float thumbH = 30.0f;
        float thumbY = sliderPos - (thumbH * 0.5f);
        auto thumbRect = juce::Rectangle<float>(track.getX() + 2, thumbY, track.getWidth() - 4, thumbH);

        // Fill level (green or warning orange)
        bool isStress = slider.getName() == "HARDNESS";
        bool warning = isStress && (slider.getValue() > 0.8);
        juce::Colour barCol = warning ? Warning : PhosphorOn;

        auto fillRect = juce::Rectangle<float>(track.getX() + 4, thumbRect.getBottom(),
                                              track.getWidth() - 8, track.getBottom() - thumbRect.getBottom());

        g.setColour(barCol.withAlpha(0.3f));
        g.fillRect(fillRect);

        // Thumb rendering
        g.setColour(Chassis.brighter(0.1f));
        g.fillRect(thumbRect);
        g.setColour(juce::Colours::black);
        g.drawRect(thumbRect, 1.0f);

        // Grip lines
        g.setColour(juce::Colours::grey);
        float gripY = thumbRect.getCentreY();
        for (int i = -1; i <= 1; ++i)
        {
            g.drawLine(thumbRect.getX() + 4, gripY + i * 4, thumbRect.getRight() - 4, gripY + i * 4, 1.0f);
        }
    }

    juce::Font getIndustrialFont(float size, bool bold = false)
    {
        int style = bold ? juce::Font::bold : juce::Font::plain;
        return juce::Font(juce::FontOptions("Courier New", size, style));
    }
};
