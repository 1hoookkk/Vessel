/*
  ==============================================================================
    VesselStyle.h
    The Visual DNA: Ceramic, Orange, and Ink.
  ==============================================================================
*/
#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace Vessel
{
    // --- PALETTE (Dark Industrial / Glass Cube Vibe) ---
    const juce::Colour bg           = juce::Colour::fromString("FF121214"); // Main Chassis (Deep Matte)
    const juce::Colour inset        = juce::Colour::fromString("FF0A0A0C"); // Recessed Areas
    const juce::Colour accent       = juce::Colour::fromString("FFFF3300"); // Safety Orange (Glowing)
    const juce::Colour text         = juce::Colour::fromString("FF888888"); // Label Grey
    const juce::Colour textDark     = juce::Colour::fromString("FFDDDDDD"); // Value White
    const juce::Colour screenBg     = juce::Colour::fromString("FF000000"); // Void
    const juce::Colour screenInk    = juce::Colour::fromString("FF151515"); // Old LCD Dark

    // --- FONTS ---
    static juce::Font getTechFont(float height, bool bold = false)
    {
        return juce::Font("Arial", height, bold ? juce::Font::bold : juce::Font::plain);
    }
}

class VesselLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VesselLookAndFeel() 
    {
        setColour(juce::Slider::thumbColourId, Vessel::accent);
        setColour(juce::Slider::trackColourId, Vessel::inset);
        setColour(juce::Label::textColourId, Vessel::text);
        setColour(juce::ComboBox::backgroundColourId, Vessel::inset);
        setColour(juce::ComboBox::textColourId, Vessel::textDark);
        setColour(juce::ComboBox::outlineColourId, Vessel::text.withAlpha(0.2f));
    }

    // Industrial Rotary Knob
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();

        // 1. Base (Dark Well)
        g.setColour(Vessel::inset);
        g.fillEllipse(centre.getX() - radius, centre.getY() - radius, radius * 2, radius * 2);

        // 2. Knob Body (Matte Grey)
        auto knobRadius = radius * 0.85f;
        g.setColour(juce::Colour::fromString("FF252528")); 
        g.fillEllipse(centre.getX() - knobRadius, centre.getY() - knobRadius, knobRadius * 2, knobRadius * 2);
        
        // 3. Indicator (Orange Line)
        juce::Path p;
        p.addRectangle(-2.0f, -knobRadius, 4.0f, knobRadius * 0.35f);
        p.applyTransform(juce::AffineTransform::rotation(toAngle).translated(centre));
        
        g.setColour(Vessel::accent);
        g.fillPath(p);

        // 4. Value Arc (Subtle)
        juce::Path arc;
        arc.addCentredArc(centre.getX(), centre.getY(), radius * 0.95f, radius * 0.95f, 
                          0.0f, rotaryStartAngle, toAngle, true);
        g.setColour(Vessel::accent.withAlpha(0.3f));
        g.strokePath(arc, juce::PathStrokeType(2.0f));
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);
        
        // TYPE A: THE "TRIMMER" (Drive, Freq, Reso)
        // Identified by LinearBar style in this setup
        if (style == juce::Slider::LinearBar)
        {
            // 1. Track Background (Recessed)
            g.setColour(Vessel::inset);
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // 2. The Marker Line
            float pos = slider.isHorizontal() ? sliderPos : (height - sliderPos);
            auto markerRect = juce::Rectangle<float>(pos - 1.5f, bounds.getY(), 3.0f, bounds.getHeight());
            
            g.setColour(Vessel::accent);
            g.fillRect(markerRect);
            
            // 3. Value Text (Right aligned inside)
            g.setColour(Vessel::textDark);
            g.setFont(Vessel::getTechFont(11.0f, true));
            // Draw value slightly offset
            g.drawText(juce::String(slider.getValue(), 0), bounds.reduced(6, 0), juce::Justification::centredRight);
        }
        // TYPE B: THE "FADER" (Flux / Morph)
        // Identified by LinearHorizontal style
        else 
        {
            // 1. The Track Line
            auto trackRect = bounds.withHeight(4.0f).withY(bounds.getCentreY() - 2.0f);
            g.setColour(Vessel::inset.brighter(0.1f));
            g.fillRoundedRectangle(trackRect, 2.0f);
            
            // 2. The Handle (Dark Box with Shadow)
            float thumbW = 30.0f;
            float thumbH = 18.0f;
            auto thumbBounds = juce::Rectangle<float>(sliderPos - (thumbW / 2), bounds.getCentreY() - (thumbH / 2), thumbW, thumbH);
            
            // Drop Shadow
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRect(thumbBounds.translated(0, 2.0f));
            
            // Cap Body
            g.setColour(juce::Colour::fromString("FF303033"));
            g.fillRect(thumbBounds);
            g.setColour(Vessel::text.withAlpha(0.5f));
            g.drawRect(thumbBounds, 1.0f);
            
            // Orange Indicator on Cap
            g.setColour(Vessel::accent);
            g.fillRect(thumbBounds.getCentreX() - 1.0f, thumbBounds.getY() + 3.0f, 2.0f, thumbH - 6.0f);
        }
    }
    
    // Minimal Label drawing
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        g.fillAll (label.findColour (juce::Label::backgroundColourId));

        if (! label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const juce::Font font (getLabelFont (label));

            g.setColour (label.findColour (juce::Label::textColourId).withMultipliedAlpha (alpha));
            g.setFont (font);

            auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());

            g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                              juce::jmax (1, (int) (textArea.getHeight() / font.getHeight())),
                              label.getMinimumHorizontalScale());
        }
    }
};