/**
 * PluginProcessor.h
 * VESSEL - Z-Plane Filter Plugin Processor
 *
 * This is the main audio processor class that integrates the ZPlaneCore engine
 * with JUCE's plugin framework.
 *
 * Thread Safety:
 *   - Uses std::atomic<float>* for lock-free parameter access in processBlock
 *   - Uses juce::AbstractFifo for lock-free communication with the editor
 *   - NO malloc, new, or blocking operations in processBlock
 */

#pragma once

#include <JuceHeader.h>
#include "../Structure/DSP/ZPlaneCore.h"
#include <atomic>
#include <array>

//==============================================================================
// Visualization Data Structure (Lock-Free Communication)
//==============================================================================

struct VisualizationSnapshot {
    float morph;
    float freq;
    float trans;
    std::array<vessel::dsp::BiquadCoeffs, 7> coefficients;

    VisualizationSnapshot() noexcept
        : morph(0.0f), freq(0.0f), trans(0.0f) {}
};

//==============================================================================
class VesselAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    VesselAudioProcessor();
    ~VesselAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter Access (Thread-Safe)
    //==============================================================================

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    /**
     * Lock-Free Visualization Data Access
     * Returns true if new data is available
     */
    bool getVisualizationSnapshot(VisualizationSnapshot& snapshot);

private:
    //==============================================================================
    // Parameter Layout Factory
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==============================================================================
    // Parameter State
    juce::AudioProcessorValueTreeState apvts;

    // Cached atomic parameter pointers (lock-free access in processBlock)
    std::atomic<float>* morphParam = nullptr;
    std::atomic<float>* freqParam = nullptr;
    std::atomic<float>* transParam = nullptr;

    //==============================================================================
    // DSP Engine (The Heart)
    vessel::dsp::ZPlaneCube filterCube;
    vessel::dsp::EmuHChipFilter filterEngine;

    //==============================================================================
    // Visualization Communication (Lock-Free FIFO)
    static constexpr int visualizationFifoSize = 32;
    juce::AbstractFifo visualizationFifo { visualizationFifoSize };
    std::array<VisualizationSnapshot, visualizationFifoSize> visualizationBuffer;

    // Write visualization data from audio thread (REAL-TIME SAFE)
    void pushVisualizationSnapshot(const VisualizationSnapshot& snapshot);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VesselAudioProcessor)
};
