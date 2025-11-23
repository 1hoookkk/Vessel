/*
  ==============================================================================
    HEAVY - Industrial Percussion Synthesizer
    File: PluginProcessor.h
    Main audio processor with material-based DSP.
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "HeavyConstants.h"
#include "DSP/EmuHChipFilter.h"
#include "DSP/PresetConverter.h"
#include "DSP/IZPlaneEngine.h"
#include "DSP/EMUStaticShapeBank.h"
#include "DSP/MaterialPhysics.h"
#include "DSP/ExciterEngine.h"
#include "SharedData.h"
#include "Utility/WaitFreeTripleBuffer.h"

using namespace Heavy;

class HeavyProcessor : public juce::AudioProcessor
{
public:
    HeavyProcessor()
        : AudioProcessor(BusesProperties()
                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        // Seed UI buffer
        auto& initState = tripleBuffer.getBackBuffer();
        initState = SystemState{};
        tripleBuffer.push();
    }

    ~HeavyProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        // Reset filters
        engineL = HChipFilter();
        engineR = HChipFilter();
        
        exciterL.prepare(sampleRate);
        exciterR.prepare(sampleRate);

        // Pre-allocate dry buffer to avoid allocation in processBlock
        dryBuffer_.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, false, true);
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // JUCE Boilerplate
    const juce::String getName() const override { return "HEAVY"; }
    bool acceptsMidi() const override { return true; } // MIDI for triggering
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; } // Long decay possible
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto state = treeState.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }

    void setStateInformation(const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName(treeState.state.getType()))
                treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    // PUBLIC ACCESS
    juce::AudioProcessorValueTreeState treeState;
    
    // DSP Members
    HChipFilter engineL;
    HChipFilter engineR;
    ZPlaneCube currentCube;
    ZPlaneParams currentParams;
    
    WaitFreeTripleBuffer<SystemState> tripleBuffer;

    // UI Helper Methods
    std::vector<juce::Point<float>> getDisplayPoles() const;
    void triggerNote(float velocity);
    void releaseNote();
    
    // Accessors for internal DSP state (for Visualization)
    std::array<zplane::Pole, 7> getLeftPoles() const;
    std::array<zplane::Pole, 7> getRightPoles() const;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // Macro Controls (skewed for physical feel)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "TENSION", "Tension",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.6f), 0.35f)); // Bias toward lower tension

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "HARDNESS", "Hardness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.25f), 0.45f)); // Emphasize mid “ring” region

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "ALLOY", "Alloy",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.5f)); // Neutral center

        // MIX: Wet/Dry blend
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "MIX", "Mix", 0.0f, 1.0f, 1.0f)); // Default: 100% wet (synth mode)

        // OUTPUT: Master gain
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "OUTPUT", "Output", -24.0f, 12.0f, 0.0f));

        return { params.begin(), params.end() };
    }

    EMUStaticShapeBank shapeBank;
    ExciterEngine exciterL, exciterR;

    // Current material state
    MaterialType currentMaterial_ = MaterialType::STEEL;
    MaterialProperties currentProps_;

    ZPlaneParams updateVoicePhysics(float tension, float hardness, float alloy) const;
    
    // Helper to recover poles for UI
    std::array<zplane::Pole, 7> interpolatePoles(float t) const;

    // Pre-allocated dry buffer (avoid allocation in processBlock)
    juce::AudioBuffer<float> dryBuffer_;

    // Auto-gain compensation state
    float smoothedInputRMS_ = 0.0f;
    float smoothedOutputRMS_ = 0.0f;
    float smoothedCompensationGain_ = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeavyProcessor)
};