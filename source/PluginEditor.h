/*
  ==============================================================================
    PluginEditor.h
  ==============================================================================
*/
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VesselStyle.h"

class HeavyEditor  : public juce::AudioProcessorEditor,
                     public juce::Timer
{
public:
    HeavyEditor (HeavyProcessor&);
    ~HeavyEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    HeavyProcessor& audioProcessor;
    VesselLookAndFeel vesselLook;

    // --- SLIDERS ---
    juce::Slider hardnessSlider, alloySlider, mixSlider, outputSlider;
    juce::Slider tensionSlider; // The "Flux" control

    // --- LABELS ---
    juce::Label lblHardness, lblAlloy, lblMix, lblOut, lblTension;
    
    // --- ATTACHMENTS (Links UI to DSP) ---
    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    
    std::unique_ptr<Attachment> hardAtt, alloyAtt, mixAtt, outAtt, tensAtt;

    // --- VISUALIZER STATE ---
    juce::Image oilBuffer; // Internal low-res buffer for the "Oil" look

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeavyEditor)
};