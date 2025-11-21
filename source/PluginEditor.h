/*
  ==============================================================================
    VESSEL // System_A01
    File: PluginEditor.h
    Desc: UI Header. Declarations Only.
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ZPlaneRibbonDisplay.h"
#include "VesselLookAndFeel.h"

// Forward Declaration
class VesselProcessor;

class VesselEditor : public juce::AudioProcessorEditor
{
public:
    VesselEditor(VesselProcessor&, juce::AudioProcessorValueTreeState&);
    ~VesselEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Reference to Logic
    VesselProcessor& processorRef;
    
    // Visualization Component
    ZPlaneRibbonDisplay screen;
    VesselLookAndFeel vesselLook;
    
    // UI Components
    juce::Label lblTitle;
    juce::Label lblVersion;
    
    juce::Slider algoKnob;
    juce::Label lblAlgo;
    juce::Label lblAlgoValue;
    
    juce::Slider morphKnob;
    juce::Label lblMorph;
    
    juce::Slider freqKnob;
    juce::Label lblFreq;
    
    juce::Slider transKnob;
    juce::Label lblTrans;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> algoAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transAttach;

    // Helpers
    void setupKnob(juce::Slider& s, juce::Label& l, juce::String id, juce::String tooltip);
    void drawScrew(juce::Graphics& g, float x, float y);
    void updateModeLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VesselEditor)
};
