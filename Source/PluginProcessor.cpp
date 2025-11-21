/**
 * PluginProcessor.cpp
 * VESSEL - Z-Plane Filter Plugin Processor Implementation
 *
 * REAL-TIME SAFETY RULES (Ross Bencina Protocol):
 *   - NO malloc, new, delete in processBlock
 *   - NO std::vector::resize or push_back in processBlock
 *   - NO std::mutex or blocking locks in processBlock
 *   - Use std::atomic for lock-free communication
 *   - Use juce::AbstractFifo for lock-free queues
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter IDs (Namespace Discipline)
//==============================================================================

namespace ParameterIDs {
    const juce::String morph { "MORPH" };
    const juce::String freq { "FREQ" };
    const juce::String trans { "TRANS" };
}

//==============================================================================
VesselAudioProcessor::VesselAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                     ),
#else
    :
#endif
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      filterEngine(48000.0f)
{
    // Initialize the filter cube with demo configuration
    filterCube = vessel::dsp::ZPlaneCubeFactory::createDemoCube();

    // Cache atomic parameter pointers for lock-free access in processBlock
    morphParam = apvts.getRawParameterValue(ParameterIDs::morph);
    freqParam = apvts.getRawParameterValue(ParameterIDs::freq);
    transParam = apvts.getRawParameterValue(ParameterIDs::trans);
}

VesselAudioProcessor::~VesselAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VesselAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // MORPH: Interpolates the Z-Plane X-axis (0.0 - 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::morph,
        "Morph",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f, // default
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 3); },
        nullptr
    ));

    // FREQ: Shifts the Y-axis (Pitch Tracking) (0.0 - 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::freq,
        "Frequency",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f, // default
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 3); },
        nullptr
    ));

    // TRANS: Shifts the Z-axis (Transform) (0.0 - 1.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterIDs::trans,
        "Transform",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.5f, // default
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 3); },
        nullptr
    ));

    return layout;
}

//==============================================================================
const juce::String VesselAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VesselAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool VesselAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool VesselAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double VesselAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VesselAudioProcessor::getNumPrograms()
{
    return 1;
}

int VesselAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VesselAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String VesselAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void VesselAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void VesselAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    // Update the filter engine's sample rate
    filterEngine.setSampleRate(static_cast<float>(sampleRate));

    // Reset filter state
    filterEngine.reset();
}

void VesselAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VesselAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // We support mono and stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    // Check that input and output layouts match
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

//==============================================================================
// THE CORE: Real-Time Audio Processing
//==============================================================================

void VesselAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // REAL-TIME SAFE: Lock-free atomic parameter read
    const float morphValue = morphParam->load();
    const float freqValue = freqParam->load();
    const float transValue = transParam->load();

    // Interpolate the cube to get the current frame
    // REAL-TIME SAFE: No allocations in interpolate_cube
    vessel::dsp::ZPlaneFrame currentFrame =
        vessel::dsp::ARMAdilloEngine::interpolate_cube(filterCube, morphValue, freqValue, transValue);

    // Update coefficients
    // REAL-TIME SAFE: No allocations in updateCoefficients
    filterEngine.updateCoefficients(currentFrame);

    // Process audio samples
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(totalNumInputChannels, totalNumOutputChannels);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // REAL-TIME SAFE: No allocations in processSample
            channelData[sample] = filterEngine.processSample(channelData[sample]);
        }
    }

    // Push visualization data (Lock-Free FIFO)
    // Only push every ~32 samples to reduce overhead
    static int visualizationCounter = 0;
    if (++visualizationCounter >= 32)
    {
        visualizationCounter = 0;

        VisualizationSnapshot snapshot;
        snapshot.morph = morphValue;
        snapshot.freq = freqValue;
        snapshot.trans = transValue;
        snapshot.coefficients = filterEngine.getCoefficients();

        pushVisualizationSnapshot(snapshot);
    }
}

//==============================================================================
// Lock-Free Visualization Communication
//==============================================================================

void VesselAudioProcessor::pushVisualizationSnapshot(const VisualizationSnapshot& snapshot)
{
    // REAL-TIME SAFE: AbstractFifo is lock-free
    int start1, size1, start2, size2;
    visualizationFifo.prepareToWrite(1, start1, size1, start2, size2);

    if (size1 > 0)
    {
        visualizationBuffer[static_cast<size_t>(start1)] = snapshot;
        visualizationFifo.finishedWrite(1);
    }
    // If FIFO is full, we just drop the frame (better than blocking)
}

bool VesselAudioProcessor::getVisualizationSnapshot(VisualizationSnapshot& snapshot)
{
    // Called from message thread (editor)
    int start1, size1, start2, size2;
    visualizationFifo.prepareToRead(1, start1, size1, start2, size2);

    if (size1 > 0)
    {
        snapshot = visualizationBuffer[static_cast<size_t>(start1)];
        visualizationFifo.finishedRead(1);
        return true;
    }

    return false;
}

//==============================================================================
bool VesselAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VesselAudioProcessor::createEditor()
{
    return new VesselAudioProcessorEditor(*this);
}

//==============================================================================
void VesselAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter state to XML
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VesselAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter state from XML
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Plugin Entry Point
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VesselAudioProcessor();
}
