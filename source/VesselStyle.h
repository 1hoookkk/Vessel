/*
  ==============================================================================
    VesselStyle.h
    The Visual DNA for HEAVY.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>

namespace Vessel
{
    // --- PALETTE ---
    const juce::Colour bg           = juce::Colour::fromString("FFF0F0F2"); // Chassis
    const juce::Colour inset        = juce::Colour::fromString("FFE5E5E8"); // Recessed areas
    const juce::Colour border       = juce::Colour::fromString("FFD5D5D8"); // Hairlines
    const juce::Colour accent       = juce::Colour::fromString("FFFF4400"); // Safety Orange
    const juce::Colour text         = juce::Colour::fromString("FF505050"); // Tech Grey
    const juce::Colour screenBg     = juce::Colour::fromString("FFC8C8C6"); // Unlit LCD
    const juce::Colour screenInk    = juce::Colour::fromString("FF151515"); // Pixel Dark

    // --- FONTS ---
    static juce::Font getTechFont(float height, bool bold = false)
    {
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, 
                          bold ? juce::Font::bold : juce::Font::plain);
    }

    // --- DRAWING HELPERS ---
    static void drawRecessedTrack(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(inset);
        g.fillRect(bounds);
        
        // Inner shadow
        g.setColour(juce::Colours::black.withAlpha(0.08f));
        g.drawRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f, 1.0f); 
        g.drawRect(bounds.getX(), bounds.getY(), 2.0f, bounds.getHeight(), 1.0f);
        
        g.setColour(border);
        g.drawRect(bounds, 1.0f);
    }
}

class VesselLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VesselLookAndFeel() {}

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);

        // 1. BACKGROUND TRACK
        if (style == juce::Slider::LinearBar)
        {
             // Full thick bar (Vertical controls)
             Vessel::drawRecessedTrack(g, bounds);
        }
        else 
        {
             // Thin track (Bottom FLUX slider)
             auto track = bounds.withHeight(6.0f).withY(bounds.getCentreY() - 3.0f);
             Vessel::drawRecessedTrack(g, track);
        }

        // 2. FILL / HANDLE
        if (style == juce::Slider::LinearBar)
        {
            // The "Filled Energy Bar" look
            float valNorm = (float)((slider.getValue() - slider.getMinimum()) / (slider.getMaximum() - slider.getMinimum()));
            float fillW = bounds.getWidth() * valNorm;
            
            // Draw Orange Fill
            g.setColour(Vessel::accent);
            g.fillRect(bounds.getX(), bounds.getY(), 2.0f, bounds.getHeight()); // Anchor
            g.setColour(Vessel::accent.withAlpha(0.3f));
            g.fillRect(bounds.getX(), bounds.getY(), fillW, bounds.getHeight()); // Trail
            g.setColour(Vessel::accent);
            g.fillRect(bounds.getX() + fillW - 2.0f, bounds.getY(), 4.0f, bounds.getHeight()); // Head
            
            // Value Text overlay
            g.setColour(Vessel::text);
            g.setFont(Vessel::getTechFont(10.0f, true));
            g.drawText(juce::String(slider.getValue(), 2), bounds.reduced(6,0), juce::Justification::centredRight);
        }
        else
        {
            // The "Cursor" look for the FLUX slider
            // Map position
            float pos = slider.isHorizontal() ? sliderPos - (float)x : sliderPos - (float)y;
            
            g.setColour(juce::Colours::white);
            // Draw a block cursor
            auto thumb = juce::Rectangle<float>(pos - 10, bounds.getY() + 2, 20, bounds.getHeight() - 4);
            
            // Drop shadow for thumb
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRect(thumb.translated(1, 1));
            
            g.setColour(Vessel::bg); // Thumb plastic
            g.fillRect(thumb);
            g.setColour(Vessel::border);
            g.drawRect(thumb, 1.0f);
            
            // Orange centerline
            g.setColour(Vessel::accent);
            g.fillRect(thumb.getCentreX() - 1, thumb.getY() + 4, 2.0f, thumb.getHeight() - 8);
        }
    }
};