/*
  ==============================================================================
    VESSEL // System_A01
    Dept: Artifacts
    File: PluginProcessor.cpp
    Desc: DSP Logic Implementation.
  ==============================================================================
*/

#include "PluginProcessor.h"
// #include "PluginEditor.h"

// juce::AudioProcessorEditor* VesselProcessor::createEditor() {
//    return new VesselEditor(*this, treeState);
// }

bool VesselProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Enforce stereo I/O; reject mono or other layouts to keep DSP/channel assumptions safe.
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void VesselProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Safety: we expect stereo; bail cleanly if host mis-configures.
    if (buffer.getNumChannels() < 2) {
        buffer.clear();
        return;
    }

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // 1. READ PARAMETERS
    float paramMorph = *treeState.getRawParameterValue("MORPH"); // 0..1
    float paramFreq = *treeState.getRawParameterValue("FREQ");   // 0..1
    float paramTrans = *treeState.getRawParameterValue("TRANS"); // 0..1
    int mode = (int)*treeState.getRawParameterValue("MODE");

    // 2. MAP PARAMETERS TO DSP
    ZPlaneParams p;
    p.morphPair = std::clamp(mode, 0, 5);
    p.morph = paramMorph;
    p.freq = paramFreq;
    p.trans = paramTrans;
    
    // Default other params
    p.intensity = 1.0f; 
    p.driveDb = 0.0f; 

    engine.setParams(p);

    // 3. AUDIO PROCESSING (Single Cascade)
    engine.processLinear(buffer);

    // 4. UI STATE PUSH
    {
        auto& state = tripleBuffer.getBackBuffer();
        
        // DSP Feedback
        state.leftPoles = engine.getLeftPoles();
        state.rightPoles = engine.getRightPoles();
        state.modeIndex = mode;
        
        // Placeholder Physics Feedback (for UI compatibility until UI is updated)
        state.kineticEnergy = 0.0f; 
        state.centroidX = paramMorph;
        state.centroidY = paramFreq;
        state.transformZ = paramTrans;
        state.particles.activeCount = 0;

        tripleBuffer.push();
    }
}

// Creation Factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new VesselProcessor();
}
