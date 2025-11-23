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
#include <foleys_gui_magic/foleys_gui_magic.h>
#include "HeavyConstants.h"
#include "DSP/EmuHChipFilter.h"
#include "DSP/PresetConverter.h"
#include "DSP/IZPlaneEngine.h"
#include "DSP/EMUStaticShapeBank.h"
#include "DSP/EMUAuthenticTables.h"
#include "DSP/MaterialPhysics.h"
#include "DSP/ExciterEngine.h"
#include "SharedData.h"
#include "Utility/WaitFreeTripleBuffer.h"
#include "Components/VesselXYPad.h"

using namespace Heavy;

class HeavyProcessor : public foleys::MagicProcessor
{
public:
    HeavyProcessor()
        : foleys::MagicProcessor(BusesProperties()
                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          treeState(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        // Seed UI buffer
        auto& initState = tripleBuffer.getBackBuffer();
        initState = SystemState{};
        tripleBuffer.push();
        
        // Note: Magic State initialization happens in initialiseBuilder/createGuiValueTree
    }

    ~HeavyProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        const double fs = (sampleRate > 0.0 ? sampleRate : 48000.0);

        // Reset filters
        engineL = HChipFilter();
        engineR = HChipFilter();
        // Desynchronize L/R control update phase to reduce correlated zipper noise
        engineL.controlCounter = 0;
        engineR.controlCounter = (engineL.controlCounter + stereoControlOffset_) & 31; // offset by half the 32-sample control period
        
        exciterL.prepare(fs);
        exciterR.prepare(fs);

        // Pre-allocate dry buffer to avoid allocation in processBlock
        dryBuffer_.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, false, true);

        // Reset smoothers
        smoothTension.reset(fs, 0.05);
        smoothHardness.reset(fs, 0.05);
        smoothAlloy.reset(fs, 0.05);
        smoothMix.reset(fs, 0.02);
        smoothOutput.reset(fs, 0.02);
        // Flux smoothing state (per-block exponential smoothing)
        smoothedFlux_ = 0.0f;

        // DC blocker pole (FS-aware). Use a low cutoff (Hz) to remove DC.
        const float dcCutoffHz = 5.0f; // gentle DC removal
        dcPole_ = std::exp(-juce::MathConstants<float>::twoPi * dcCutoffHz / (float)fs);

        // Reset envelope/DC/reconstruction states
        envFollower_ = 0.0f;
        dcPrevInputL_ = dcPrevInputR_ = 0.0f;
        dcBlockerStateL_ = dcBlockerStateR_ = 0.0f;
        reconStateL_ = reconStateR_ = 0.0f;

        // Mild reconstruction filter (post DAC smoothing)
        const float targetFc = 18000.0f;
        const float safeFc = juce::jlimit(2000.0f, (float)(fs * 0.48), targetFc);
        reconCoeff_ = std::exp(-juce::MathConstants<float>::twoPi * safeFc / (float)fs);
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    // Foleys Magic Overrides
    void initialiseBuilder (foleys::MagicGUIBuilder& builder) override;
    juce::ValueTree createGuiValueTree() override;

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

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void getStateInformation(juce::MemoryBlock& destData) override
    {
        // We only save parameters, layout is fixed/binary
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

        // FLUX: Envelope follower depth mapped to Z-axis
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "FLUX", "Flux",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f, 1.0f), 0.0f)); // Off by default

        // MIX: Wet/Dry blend
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "MIX", "Mix", 0.0f, 1.0f, 1.0f)); // Default: 100% wet (synth mode)

        // OUTPUT: Master gain
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            "OUTPUT", "Output", -24.0f, 12.0f, 0.0f));

        // FILTER MODEL: Choice of legacy EMU morph pairs
        juce::StringArray modelNames;
        for (int i = 0; i < AUTHENTIC_EMU_NUM_PAIRS; ++i)
            modelNames.add(AUTHENTIC_EMU_IDS[i]);

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            "FILTER_MODEL", "Filter Model", modelNames, 0));

        return { params.begin(), params.end() };
    }

    EMUStaticShapeBank shapeBank;
    ExciterEngine exciterL, exciterR;

    // Smoothed Parameters
    juce::LinearSmoothedValue<float> smoothTension;
    juce::LinearSmoothedValue<float> smoothHardness;
    juce::LinearSmoothedValue<float> smoothAlloy;
    juce::LinearSmoothedValue<float> smoothMix;
    juce::LinearSmoothedValue<float> smoothOutput;

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

    // Flux smoothing (per-block) and DC blocker pole
    float smoothedFlux_ = 0.0f;
    float dcPole_ = 0.995f;

    // Stereo decoherence offset (control counter offset between channels)
    const int stereoControlOffset_ = 16;

    // Envelope follower for Z-axis modulation
    float envFollower_ = 0.0f;

    // Post-stage conditioning (DC block + mild reconstruction low-pass)
    float dcPrevInputL_ = 0.0f, dcPrevInputR_ = 0.0f;
    float dcBlockerStateL_ = 0.0f, dcBlockerStateR_ = 0.0f;
    float reconStateL_ = 0.0f, reconStateR_ = 0.0f;
    float reconCoeff_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeavyProcessor)
};
