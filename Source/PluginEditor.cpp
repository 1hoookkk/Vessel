/**
 * PluginEditor.cpp
 * VESSEL - Z-Plane Filter Plugin Editor Implementation
 *
 * Layout:
 *   - Top: Title and branding
 *   - Center: Large visualization display (ribbons)
 *   - Bottom: Three rotary controls (MORPH, FREQ, TRANS)
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VesselAudioProcessorEditor::VesselAudioProcessorEditor(VesselAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), visualizer(p)
{
    // Apply custom look and feel
    setLookAndFeel(&lookAndFeel);

    // Setup title label
    titleLabel.setText("V E S S E L", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
    addAndMakeVisible(titleLabel);

    // Setup visualizer
    addAndMakeVisible(visualizer);

    // Setup parameter controls
    setupSlider(morphSlider, morphLabel, "MORPH");
    setupSlider(freqSlider, freqLabel, "FREQ");
    setupSlider(transSlider, transLabel, "TRANS");

    // Create parameter attachments
    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "MORPH", morphSlider);
    freqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "FREQ", freqSlider);
    transAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "TRANS", transSlider);

    // Set editor size
    setSize(800, 600);
}

VesselAudioProcessorEditor::~VesselAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void VesselAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background: Deep black
    g.fillAll(juce::Colour(0xff000000));

    // Draw subtle border frame (Fragile Brutalism aesthetic)
    auto bounds = getLocalBounds();
    g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
    g.drawRect(bounds, 1);

    // Draw section dividers
    int titleHeight = 50;
    int controlHeight = 180;

    // Line under title
    g.drawHorizontalLine(titleHeight, 0.0f, static_cast<float>(getWidth()));

    // Line above controls
    g.drawHorizontalLine(getHeight() - controlHeight, 0.0f, static_cast<float>(getWidth()));

    // Draw subtle grid in visualization area (technical aesthetic)
    int vizTop = titleHeight;
    int vizHeight = getHeight() - titleHeight - controlHeight;
    int vizCenterY = vizTop + vizHeight / 2;

    g.setColour(juce::Colour(0xff00ff41).withAlpha(0.05f));

    // Horizontal center line
    g.drawHorizontalLine(vizCenterY, 0.0f, static_cast<float>(getWidth()));

    // Vertical dividers (4 sections)
    for (int i = 1; i < 4; ++i)
    {
        float x = (getWidth() / 4.0f) * i;
        g.drawVerticalLine(static_cast<int>(x),
                          static_cast<float>(vizTop),
                          static_cast<float>(vizTop + vizHeight));
    }
}

void VesselAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title area (top 50px)
    int titleHeight = 50;
    titleLabel.setBounds(bounds.removeFromTop(titleHeight));

    // Control area (bottom 180px)
    int controlHeight = 180;
    auto controlArea = bounds.removeFromBottom(controlHeight);

    // Visualization area (remaining space)
    visualizer.setBounds(bounds);

    // Layout controls in a row
    int controlWidth = controlArea.getWidth() / 3;
    int sliderSize = 120;
    int labelHeight = 20;
    int padding = 20;

    // MORPH control (left)
    auto morphArea = controlArea.removeFromLeft(controlWidth);
    morphLabel.setBounds(morphArea.removeFromTop(labelHeight).reduced(padding, 5));
    morphSlider.setBounds(morphArea.withSizeKeepingCentre(sliderSize, sliderSize));

    // FREQ control (center)
    auto freqArea = controlArea.removeFromLeft(controlWidth);
    freqLabel.setBounds(freqArea.removeFromTop(labelHeight).reduced(padding, 5));
    freqSlider.setBounds(freqArea.withSizeKeepingCentre(sliderSize, sliderSize));

    // TRANS control (right)
    auto transArea = controlArea;
    transLabel.setBounds(transArea.removeFromTop(labelHeight).reduced(padding, 5));
    transSlider.setBounds(transArea.withSizeKeepingCentre(sliderSize, sliderSize));
}

//==============================================================================
void VesselAudioProcessorEditor::setupSlider(juce::Slider& slider,
                                            juce::Label& label,
                                            const juce::String& labelText)
{
    // Configure slider
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setRange(0.0, 1.0, 0.001);
    slider.setPopupDisplayEnabled(true, true, this);
    addAndMakeVisible(slider);

    // Configure label
    label.setText(labelText, juce::dontSendNotification);
    label.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff00ff41));
    addAndMakeVisible(label);
}
