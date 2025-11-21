/**
 * PluginEditor.h
 * VESSEL - Z-Plane Filter Plugin Editor
 *
 * Aesthetic: "Fragile Brutalism" / "Haunted Hardware"
 *   - High contrast monochromatic (terminal green on deep black)
 *   - 1px thin lines, no gradients
 *   - Minimalist technical lab equipment appearance
 *   - Custom rotary controls (no standard JUCE sliders)
 */

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ZPlaneDisplay.h"

//==============================================================================
// Custom Look and Feel: Fragile Brutalism
//==============================================================================

class FragileBrutalismLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FragileBrutalismLookAndFeel()
    {
        // Terminal green color scheme
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff00ff41));
        setColour(juce::Slider::trackColourId, juce::Colour(0xff003311));
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00ff41));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff004422));
        setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
        setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));
    }

    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        juce::ignoreUnused(slider);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Draw outer circle (1px thin line)
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.3f));
        g.drawEllipse(centerX - radius, centerY - radius,
                     radius * 2.0f, radius * 2.0f, 1.0f);

        // Draw arc to show value (minimal)
        juce::Path valueArc;
        valueArc.addCentredArc(centerX, centerY,
                              radius * 0.9f, radius * 0.9f,
                              0.0f,
                              rotaryStartAngle, angle,
                              true);
        g.setColour(juce::Colour(0xff00ff41));
        g.strokePath(valueArc, juce::PathStrokeType(1.0f));

        // Draw pointer (thin line from center)
        juce::Path pointer;
        float pointerLength = radius * 0.7f;
        pointer.startNewSubPath(centerX, centerY);
        pointer.lineTo(centerX + pointerLength * std::cos(angle - juce::MathConstants<float>::halfPi),
                      centerY + pointerLength * std::sin(angle - juce::MathConstants<float>::halfPi));
        g.strokePath(pointer, juce::PathStrokeType(1.0f));
    }

    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
        label->setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
        label->setJustificationType(juce::Justification::centred);
        return label;
    }
};

//==============================================================================
// Main Plugin Editor
//==============================================================================

class VesselAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VesselAudioProcessorEditor(VesselAudioProcessor&);
    ~VesselAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    VesselAudioProcessor& audioProcessor;

    // Custom Look and Feel
    FragileBrutalismLookAndFeel lookAndFeel;

    // Visualization
    ZPlaneDisplay visualizer;

    // Parameter Controls
    juce::Slider morphSlider;
    juce::Slider freqSlider;
    juce::Slider transSlider;

    // Labels
    juce::Label morphLabel;
    juce::Label freqLabel;
    juce::Label transLabel;
    juce::Label titleLabel;

    // Parameter Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transAttachment;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VesselAudioProcessorEditor)
};
