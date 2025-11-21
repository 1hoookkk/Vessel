#pragma once

#include <JuceHeader.h>
#include "DSP/EmuZPlaneCore.h"

// ==============================================================================
// HyperCubeProcessor
// The Host/Game Loop for the H-Chip Emulation
// ==============================================================================
class HyperCubeProcessor : public juce::AudioProcessor
{
public:
    HyperCubeProcessor();
    ~HyperCubeProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HyperCube"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return "Default"; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public for Editor access (Visualizer hooks)
    // In a real app we might use a more decoupled approach, but for this spec we access directly
    std::vector<BiquadCoeffs> getCoeffsForVisualizer(float x, float y, float z);
    
    juce::AudioProcessorValueTreeState apvts;

private:
    // DSP Core
    EmuHChipFilter leftFilter;
    EmuHChipFilter rightFilter;
    ZPlaneCube morphCube; // The "Data Cartridge"

    // Parameters
    std::atomic<float>* morphParam = nullptr;
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* transformParam = nullptr;
    std::atomic<float>* gritParam = nullptr;

    // Internal Setup
    void initializeZPlaneCube(); // Sets up a dummy filter cube for testing
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperCubeProcessor)
};
