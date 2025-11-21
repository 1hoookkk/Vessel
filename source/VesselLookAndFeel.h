/*
  ==============================================================================
    VESSEL // WIREFRAME
    File: VesselLookAndFeel.h
    Desc: Minimalist "Scientific Wireframe" Aesthetic.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class VesselLookAndFeel : public juce::LookAndFeel_V4 {
public:
    // Palette: High Contrast Scientific
    const juce::Colour kBackground  = juce::Colour(0xFF101010); // Flat Black/Grey
    const juce::Colour kPanel       = juce::Colour(0xFF1A1A1A); // Slightly lighter panels
    const juce::Colour kBorder      = juce::Colour(0xFF404040); // Structural Lines
    const juce::Colour kText        = juce::Colour(0xFFE0E0E0); // Primary Text
    const juce::Colour kTextDim     = juce::Colour(0xFF808080); // Labels
    const juce::Colour kAccent      = juce::Colour(0xFF00FF41); // Matrix Green (Wireframe)
    const juce::Colour kActive      = juce::Colour(0xFFFFFF00); // Yellow (Cursor/Active)

    VesselLookAndFeel() {
        setColour(juce::Slider::thumbColourId, kAccent);
        setColour(juce::Slider::rotarySliderFillColourId, kAccent);
        setColour(juce::Slider::rotarySliderOutlineColourId, kBorder);
        setColour(juce::Label::textColourId, kText);
    }

    juce::Font getMonospaceFont(float height, bool bold = false) {
        return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), height, bold ? juce::Font::bold : juce::Font::plain));
    }

    // Minimalist Flat Knob
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override 
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(2.0f);
        auto centre = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
        
        // 1. Outer Ring (Range)
        float angleRange = rotaryEndAngle - rotaryStartAngle;
        float angle = rotaryStartAngle + sliderPos * angleRange;
        
        juce::Path arc;
        arc.addCentredArc(centre.x, centre.y, radius + 4.0f, radius + 4.0f, 
                          0.0f, rotaryStartAngle, rotaryEndAngle, true);
        
        g.setColour(kBorder);
        g.strokePath(arc, juce::PathStrokeType(2.0f));
        
        // 2. Active Arc
        juce::Path valArc;
        valArc.addCentredArc(centre.x, centre.y, radius + 4.0f, radius + 4.0f, 
                             0.0f, rotaryStartAngle, angle, true);
        g.setColour(kAccent);
        g.strokePath(valArc, juce::PathStrokeType(2.0f));

        // 3. Knob Body (Simple Circle)
        g.setColour(kPanel.brighter(0.1f));
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(kText);
        g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        // 4. Indicator Line
        juce::Path ptr;
        ptr.startNewSubPath(centre.x, centre.y);
        ptr.lineTo(centre.x + radius * std::cos(angle), centre.y + radius * std::sin(angle));
        g.setColour(kText);
        g.strokePath(ptr, juce::PathStrokeType(2.0f));
    }
    
    void drawLabel(juce::Graphics& g, juce::Label& label) override {
        g.setColour(kText);
        g.setFont(getMonospaceFont(12.0f, false));
        g.drawFittedText(label.getText(), label.getLocalBounds(), juce::Justification::centred, 1);
    }
};
