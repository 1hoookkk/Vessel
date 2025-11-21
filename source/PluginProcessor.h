/*
  ==============================================================================
    VESSEL // System_A01
    Dept: Artifacts
    File: PluginProcessor.h
    Desc: Main Processor Header.
  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <foleys_gui_magic/foleys_gui_magic.h>
#include "DSP/AuthenticEMUEngine.h"
#include "DSP/EMUStaticShapeBank.h"
#include "SharedData.h"
#include "Utility/WaitFreeTripleBuffer.h"

class VesselProcessor : public foleys::MagicProcessor {
public:
    VesselProcessor() 
        : MagicProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          treeState(*this, nullptr, "PARAMETERS", createParameterLayout()),
          engine(shapeBank)
    {
        FOLEYS_SET_SOURCE_PATH(__FILE__);

        // Seed UI buffer
        auto& initState = tripleBuffer.getBackBuffer();
        initState = SystemState{};
        tripleBuffer.push();
    }
    
    ~VesselProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        engine.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    // juce::AudioProcessorEditor* createEditor() override;
    // bool hasEditor() const override { return true; }
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    
    // JUCE Boilerplate
    const juce::String getName() const override { return "VESSEL"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock& destData) override {
        auto state = treeState.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
    void setStateInformation(const void* data, int sizeInBytes) override {
        std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
        if (xmlState.get() != nullptr)
            if (xmlState->hasTagName(treeState.state.getType()))
                treeState.replaceState(juce::ValueTree::fromXml(*xmlState));
    }

    // PUBLIC ARTIFACTS
    juce::AudioProcessorValueTreeState treeState;
    AuthenticEMUEngine engine; 
    WaitFreeTripleBuffer<SystemState> tripleBuffer;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>("MORPH", "Morph", 0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("FREQ", "Frequency", 0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>("TRANS", "Transform", 0.0f, 1.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterInt>("MODE", "Algorithm", 0, 5, 0));

        return { params.begin(), params.end() };
    }

    EMUStaticShapeBank shapeBank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VesselProcessor)
};
