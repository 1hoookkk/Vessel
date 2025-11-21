/*
  ==============================================================================
    VESSEL // System_A01
    File: PluginEditor.cpp
    Desc: Layout and Styling Implementation (Lexicon/Hardware Era).
  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cmath>

namespace {
constexpr int kHeaderHeight = 68;
constexpr int kFooterHeight = 118;
constexpr int kMargin       = 14;
}

VesselEditor::VesselEditor(VesselProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), processorRef(p), screen(p.tripleBuffer)
{
    // 1. Style Init
    setLookAndFeel(&vesselLook);
    
    // 2. Monitor
    addAndMakeVisible(screen);
    
    // Interaction: Deprecated direct drag. Parameters controlled via knobs.

    // 3. Typography Setup
    lblTitle.setText("VESSEL", juce::dontSendNotification);
    lblTitle.setFont(vesselLook.getMonospaceFont(24.0f, true));
    lblTitle.setJustificationType(juce::Justification::topLeft);
    lblTitle.setColour(juce::Label::textColourId, vesselLook.kText);
    addAndMakeVisible(lblTitle);

    lblVersion.setText("v1.0.0 [Z-Plane]", juce::dontSendNotification);
    lblVersion.setFont(vesselLook.getMonospaceFont(10.0f, false));
    lblVersion.setJustificationType(juce::Justification::topRight);
    lblVersion.setColour(juce::Label::textColourId, vesselLook.kTextDim);
    addAndMakeVisible(lblVersion);

    // 4. Controls
    setupKnob(algoKnob, lblAlgo, "MODE", "Select Processing Algorithm");
    lblAlgoValue.setFont(vesselLook.getMonospaceFont(12.0f, false));
    lblAlgoValue.setJustificationType(juce::Justification::centred);
    lblAlgoValue.setColour(juce::Label::textColourId, vesselLook.kAccent); 
    addAndMakeVisible(lblAlgoValue);
    algoAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "MODE", algoKnob);
    algoKnob.onValueChange = [this]() { updateModeLabel(); };
    updateModeLabel();
    
    setupKnob(morphKnob, lblMorph, "MORPH", "Z-Plane Morphing (X-Axis)");
    morphAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "MORPH", morphKnob);
    
    setupKnob(freqKnob, lblFreq, "FREQ", "Frequency Scaling (Y-Axis)");
    freqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "FREQ", freqKnob);
    
    setupKnob(transKnob, lblTrans, "TRANS", "Transformation / Depth (Z-Axis)");
    transAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(vts, "TRANS", transKnob);

    // 5. Window Sizing
    setResizable(true, true);
    setResizeLimits(380, 520, 900, 960);
    setSize(420, 620);
}

VesselEditor::~VesselEditor()
{
    setLookAndFeel(nullptr);
}

void VesselEditor::setupKnob(juce::Slider& s, juce::Label& l, juce::String id, juce::String tooltip)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    s.setTooltip(tooltip);
    addAndMakeVisible(s);
    
    l.setText(id, juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, vesselLook.kTextDim);
    addAndMakeVisible(l);
}

void VesselEditor::paint(juce::Graphics& g)
{
    // 1. Background - Flat Minimalist
    g.fillAll(vesselLook.kBackground);
    
    // 2. Structural Layout (Wireframe Borders)
    g.setColour(vesselLook.kBorder);
    
    auto bounds = getLocalBounds();
    
    // Header Divider
    g.drawHorizontalLine(kHeaderHeight, 0.0f, (float)getWidth());
    
    // Footer Divider
    g.drawHorizontalLine(getHeight() - kFooterHeight, 0.0f, (float)getWidth());

    // 3. Screen Bezel (Simple Outline)
    auto screenArea = bounds.reduced(kMargin).withTrimmedTop(kHeaderHeight).withTrimmedBottom(kFooterHeight);
    g.drawRect(screenArea.expanded(2), 1);

    // 4. Minimal Status Indicator (Small Square)
    bool isAudioActive = false;
    if (auto* ph = processorRef.getPlayHead()) {
        if (auto pos = ph->getPosition()) {
            isAudioActive = pos->getIsPlaying() || pos->getIsRecording();
        }
    }
    
    int ledY = 24;
    int ledX = getWidth() - 24;
    
    g.setColour(isAudioActive ? vesselLook.kAccent : vesselLook.kBorder);
    g.fillRect(ledX - 3, ledY - 3, 6, 6); // Square LED
    
    g.setColour(vesselLook.kTextDim);
    g.setFont(vesselLook.getMonospaceFont(10.0f, false));
    g.drawText("SIGNAL", ledX - 50, ledY - 8, 40, 16, juce::Justification::right);

    // 5. Product ID
    g.setColour(vesselLook.kTextDim);
    g.drawText("VESSEL // Z-PLANE", kMargin, kHeaderHeight + 4, 200, 12, juce::Justification::left);
}

void VesselEditor::drawScrew(juce::Graphics&, float, float) 
{
    // Deprecated in Wireframe design
}

void VesselEditor::resized()
{
    auto area = getLocalBounds().reduced(kMargin);

    // 1. Header
    auto header = area.removeFromTop(kHeaderHeight);
    lblTitle.setBounds(header.removeFromLeft(header.getWidth() / 2));
    lblVersion.setBounds(header);

    // 2. Footer
    auto footer = area.removeFromBottom(kFooterHeight);
    
    // 3. Screen (Fills rest)
    screen.setBounds(area);

    // 4. Footer Layout (2x2 Grid)
    const int knobSize = 60;
    const int pad = 10;
    
    // Row 1: Algo, Morph
    // Row 2: Freq, Trans
    // Wait, 1 row of 4 is better for "Channel Strip" vibe.
    
    int w = footer.getWidth() / 4;
    int y = footer.getY() + 20;
    
    // Algo
    algoKnob.setBounds(footer.getX() + 0*w + (w-knobSize)/2, y, knobSize, knobSize);
    lblAlgo.setBounds(algoKnob.getX(), y - 15, knobSize, 15);
    lblAlgoValue.setBounds(algoKnob.getX() - 10, y + knobSize + 2, knobSize + 20, 15);
    
    // Morph
    morphKnob.setBounds(footer.getX() + 1*w + (w-knobSize)/2, y, knobSize, knobSize);
    lblMorph.setBounds(morphKnob.getX(), y - 15, knobSize, 15);
    
    // Freq
    freqKnob.setBounds(footer.getX() + 2*w + (w-knobSize)/2, y, knobSize, knobSize);
    lblFreq.setBounds(freqKnob.getX(), y - 15, knobSize, 15);
    
    // Trans
    transKnob.setBounds(footer.getX() + 3*w + (w-knobSize)/2, y, knobSize, knobSize);
    lblTrans.setBounds(transKnob.getX(), y - 15, knobSize, 15);
}