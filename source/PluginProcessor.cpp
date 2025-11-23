/*
  ==============================================================================
    HEAVY - Industrial Percussion Synthesizer
    File: PluginProcessor.cpp
    DSP logic: Material physics → Z-Plane resonator
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>

juce::AudioProcessorEditor* HeavyProcessor::createEditor()
{
    return new HeavyEditor(*this);
}

bool HeavyProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Enforce stereo I/O
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::stereo()) return false;
    if (mainIn != juce::AudioChannelSet::stereo()) return false;

    return true;
}

void HeavyProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Safety: expect stereo
    if (buffer.getNumChannels() < 2)
    {
        buffer.clear();
        return;
    }

    // Clear unused channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // ═══════════════════════════════════════════════════════════════════════
    // 1. READ PARAMETERS
    // ═══════════════════════════════════════════════════════════════════════
    float tension  = *treeState.getRawParameterValue("TENSION");   // 0-1 (head tightening)
    float hardness = *treeState.getRawParameterValue("HARDNESS");  // 0-1 (damping)
    float alloy    = *treeState.getRawParameterValue("ALLOY");     // 0-1 (material path)
    float mix      = *treeState.getRawParameterValue("MIX");       // 0-1
    float outputDb = *treeState.getRawParameterValue("OUTPUT");    // dB

    // Update material state for exciters/labels
    const int materialIndex = juce::jlimit(0, 7, juce::roundToInt(alloy * 7.0f));
    MaterialType material = static_cast<MaterialType>(materialIndex);
    if (material != currentMaterial_)
    {
        currentMaterial_ = material;
        currentProps_ = MaterialProperties::get(material);
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 2. MAP MACROS → H-CHIP (k1/k2 + cube focus)
    // ═══════════════════════════════════════════════════════════════════════
    ZPlaneParams zParams = updateVoicePhysics(tension, hardness, alloy);
    
    // Check if Morph Pair Changed, Update Cube
    if (zParams.morphPair != currentParams.morphPair) {
        auto pair = shapeBank.morphPairIndices(zParams.morphPair);
        currentCube = PresetConverter::convertToCube(pair.first, pair.second);
    }
    currentParams = zParams;

    // ═══════════════════════════════════════════════════════════════════════
    // 3. INTERPOLATE FRAME (The Brain)
    // ═══════════════════════════════════════════════════════════════════════
    // ARMAdillo engine calculates the target frame for this block
    // We pass (morph, freq, trans) -> (x, y, z)
    ZPlaneFrame targetFrame = ARMAdilloEngine::interpolate_cube(currentCube, zParams.morph, zParams.freq, zParams.trans);

    // ═══════════════════════════════════════════════════════════════════════
    // 4. AUDIO PROCESSING (The Body)
    // ═══════════════════════════════════════════════════════════════════════

    // Store dry signal for mix (use pre-allocated buffer)
    dryBuffer_.makeCopyOf(buffer, true);

    // AUTO-GAIN: Measure input RMS
    float inputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1)
        inputRMS = (inputRMS + buffer.getRMSLevel(1, 0, buffer.getNumSamples())) * 0.5f;

    // Apply Drive (Input Gain)
    if (zParams.driveDb > 0.1f) {
        float driveGain = std::pow(10.0f, zParams.driveDb / 20.0f);
        buffer.applyGain(driveGain);
    }

    // Process Filters
    auto* l = buffer.getWritePointer(0);
    auto* r = buffer.getWritePointer(1);
    int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate();
    if (sampleRate <= 0.0f) sampleRate = 48000.0f;

    process_block(&engineL, l, numSamples, targetFrame, sampleRate);
    process_block(&engineR, r, numSamples, targetFrame, sampleRate);

    // AUTO-GAIN: Measure output RMS and compensate
    float outputRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() > 1)
        outputRMS = (outputRMS + buffer.getRMSLevel(1, 0, buffer.getNumSamples())) * 0.5f;

    // Calculate dynamic compensation (with smoothing to prevent pumping)
    const float smoothingCoeff = 0.05f; // ~20ms at 44.1kHz
    smoothedInputRMS_ += smoothingCoeff * (inputRMS - smoothedInputRMS_);
    smoothedOutputRMS_ += smoothingCoeff * (outputRMS - smoothedOutputRMS_);

    // Compensation ratio (with safety limits)
    float targetGain = 1.0f;
    if (smoothedOutputRMS_ > 0.0001f && smoothedInputRMS_ > 0.0001f)
    {
        targetGain = smoothedInputRMS_ / smoothedOutputRMS_;
        targetGain = juce::jlimit(0.05f, 20.0f, targetGain); // Wider range for aggressive compensation
    }

    // Smooth the compensation gain to prevent zipper noise
    smoothedCompensationGain_ += smoothingCoeff * (targetGain - smoothedCompensationGain_);

    // Apply auto-gain + fixed makeup (14-pole cascade with drive needs substantial boost)
    const float baselineMakeup = juce::Decibels::decibelsToGain(24.0f); // Increased from 12dB
    const float totalGain = smoothedCompensationGain_ * baselineMakeup;
    buffer.applyGain(totalGain);

    // Wet/Dry mix
    const float wetAmt = juce::jlimit(0.0f, 1.0f, mix);
    const float dryAmt = 1.0f - wetAmt;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dryPtr = dryBuffer_.getReadPointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wet[i] = wet[i] * wetAmt + dryPtr[i] * dryAmt;
        }
    }

    // Master output gain
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);
    buffer.applyGain(outputGain);

    // ═══════════════════════════════════════════════════════════════════════
    // 6. METERING (for reactive visualization)
    // ═══════════════════════════════════════════════════════════════════════
    float finalRMS = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
    float finalPeak = buffer.getMagnitude(0, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() > 1)
    {
        finalRMS = (finalRMS + buffer.getRMSLevel(1, 0, buffer.getNumSamples())) * 0.5f;
        finalPeak = juce::jmax(finalPeak, buffer.getMagnitude(1, 0, buffer.getNumSamples()));
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 7. UPDATE UI STATE (TRIPLE BUFFER)
    // ═══════════════════════════════════════════════════════════════════════
    {
        auto& state = tripleBuffer.getBackBuffer();

        // DSP pole data for radar visualization
        state.leftPoles = getLeftPoles();
        state.rightPoles = getRightPoles();

        // Metering data for reactive wireframe
        state.outputRMS = finalRMS;
        state.outputPeak = finalPeak;

        // Material/parameter feedback
        state.modeIndex = juce::jlimit(0, 7, (int)std::round(alloy * 7.0f));
        state.centroidX = tension;      // X: Tension
        state.centroidY = hardness;     // Y: Hardness
        state.transformZ = zParams.trans;

        // Clear deprecated fields
        state.particles.activeCount = 0;

        tripleBuffer.push();
    }
}

// ═══════════════════════════════════════════════════════════════════════
// UI HELPER METHODS
// ═══════════════════════════════════════════════════════════════════════

std::vector<juce::Point<float>> HeavyProcessor::getDisplayPoles() const
{
    std::vector<juce::Point<float>> cartesianPoles;
    cartesianPoles.reserve(14); // 7 L/R pairs

    // Get current state (most recent from triple buffer)
    SystemState state = tripleBuffer.getCurrentState();

    // Convert polar poles to cartesian for radar display
    auto convertPole = [](const zplane::Pole& p) -> juce::Point<float> {
        float x = p.r * std::cos(p.theta);
        float y = p.r * std::sin(p.theta);
        return {x, y};
    };

    // Left channel poles (first 6)
    for (int i = 0; i < 7; ++i)
    {
        cartesianPoles.push_back(convertPole(state.leftPoles[i]));
    }

    // Right channel poles
    for (int i = 0; i < 7; ++i)
    {
        cartesianPoles.push_back(convertPole(state.rightPoles[i]));
    }

    return cartesianPoles;
}

// Helper: interpolatePoles for visualization
std::array<zplane::Pole, 7> HeavyProcessor::interpolatePoles(float t) const {
    std::array<zplane::Pole, 7> out;
    float sampleRate = (float)getSampleRate();
    if (sampleRate <= 0.0f) sampleRate = 48000.0f;

    // Use current params
    ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(currentCube, currentParams.morph, currentParams.freq, currentParams.trans);

    for (int i = 0; i < 7; ++i) {
        float k1 = frame.sections[i].k1;
        float freqHz = 20.0f * std::pow(1000.0f, k1);
        float theta = 2.0f * 3.14159f * freqHz / sampleRate;

        float k2 = frame.sections[i].k2;
        float r = k2 * 0.98f; 

        out[i] = {r, theta};
    }
    return out;
}

std::array<zplane::Pole, 7> HeavyProcessor::getLeftPoles() const {
    return interpolatePoles(0.0f);
}
std::array<zplane::Pole, 7> HeavyProcessor::getRightPoles() const {
    return interpolatePoles(0.0f);
}


ZPlaneParams HeavyProcessor::updateVoicePhysics(float tension, float hardness, float alloy) const
{
    ZPlaneParams params{};

    const float sr = (float)((getSampleRate() > 0.0) ? getSampleRate() : 48000.0);
    const float nyquist = sr * 0.49f; // leave headroom for stability

    const float t = juce::jlimit(0.0f, 1.0f, tension);
    const float h = juce::jlimit(0.0f, 1.0f, hardness);
    const float a = juce::jlimit(0.0f, 1.0f, alloy);

    // ── TENSION → k1 (frequency) ──────────────────────────────────────────
    constexpr float minHz = 40.0f;      // Sub thud
    constexpr float maxHz = 12000.0f;   // Tight ping
    constexpr float tensionExp = 1.8f;  // Inverse-log feel

    const float tensionSkew = std::pow(t, tensionExp);
    float freqHz = minHz * std::pow(maxHz / minHz, tensionSkew);
    freqHz = juce::jlimit(minHz, nyquist, freqHz);

    const float invLog1000 = 1.0f / std::log(1000.0f);
    float k1 = juce::jlimit(0.0f, 1.0f, std::log(freqHz / 20.0f) * invLog1000);

    // ── HARDNESS → k2 (resonance / damping) ───────────────────────────────
    constexpr float k2Min = 0.05f;      // Dead/matte (lowered for more dynamic range)
    constexpr float k2Max = 0.985f;     // Stability guard (push closer to edge)
    constexpr float hardnessExp = 0.7f; // Aggressive curve (< 1.0 = more top-end sensitivity)

    const float hardnessSkew = std::pow(h, hardnessExp);
    // Remove ring emphasis bump - let it hit hard at full hardness
    float k2 = juce::jlimit(k2Min, k2Max, k2Min + (k2Max - k2Min) * hardnessSkew);

    // ── ALLOY PATH → 3D cube focus (X,Y,Z) ────────────────────────────────
    struct FocusPoint { float pos; float x; float y; float z; int morphPair; };
    static constexpr std::array<FocusPoint, 8> path = {{
        {0.00f, 0.05f, 0.18f, 0.20f, 0}, // IRON (sub/dark)
        {0.15f, 0.12f, 0.28f, 0.32f, 1}, // STEEL
        {0.30f, 0.24f, 0.40f, 0.48f, 2}, // BRASS
        {0.45f, 0.40f, 0.52f, 0.38f, 3}, // ALUMINUM (neutral/fast)
        {0.60f, 0.58f, 0.68f, 0.62f, 4}, // GLASS
        {0.75f, 0.72f, 0.78f, 0.82f, 5}, // QUARTZ
        {0.88f, 0.84f, 0.64f, 0.58f, 6}, // CARBON (phasey)
        {1.00f, 0.95f, 0.88f, 0.95f, 7}, // URANIUM (unstable)
    }};

    const auto lerp = [](float aVal, float bVal, float tVal) { return aVal + (bVal - aVal) * tVal; };

    const FocusPoint* lo = &path.front();
    const FocusPoint* hi = &path.back();
    for (size_t i = 1; i < path.size(); ++i)
    {
        if (a <= path[i].pos)
        {
            lo = &path[i - 1];
            hi = &path[i];
            break;
        }
    }

    if (a >= path.back().pos)
    {
        lo = hi = &path.back();
    }

    const float segT = (hi->pos > lo->pos) ? (a - lo->pos) / (hi->pos - lo->pos) : 0.0f;

    const float focusX = lerp(lo->x, hi->x, segT);
    const float focusY = lerp(lo->y, hi->y, segT);
    const float focusZ = lerp(lo->z, hi->z, segT);
    int morphPair = static_cast<int>(std::round(lerp((float)lo->morphPair, (float)hi->morphPair, segT)));
    morphPair = juce::jlimit(0, 7, morphPair);

    // Blend alloy focus with macro tension/hardness so the slider dominates
    params.morphPair = morphPair;
    params.morph     = focusX;
    params.freq      = juce::jlimit(0.0f, 1.0f, 0.25f * focusY + 0.75f * k1); // heavy weight on tension
    params.trans     = juce::jlimit(0.0f, 1.0f, 0.35f * focusZ + 0.65f * k2); // keep alloy flavor
    params.intensity = 1.0f;
    // Drive: DISABLED for testing - int32 has zero headroom, +18dB creates square wave clipping
    // Let the resonance peaks create the grit instead
    params.driveDb   = 0.0f;

    return params;
}


void HeavyProcessor::triggerNote(float velocity)
{
    // Trigger exciter engines
    float pitch = 200.0f; // Base frequency, will be modulated by TENSION

    exciterL.trigger(currentProps_.exciter, velocity, currentProps_.exciterDecay, pitch);
    exciterR.trigger(currentProps_.exciter, velocity, currentProps_.exciterDecay, pitch);
}

void HeavyProcessor::releaseNote()
{
    // For now, exciters are fire-and-forget with their own decay
    // Future: Could implement sustain/release stages
}

// Plugin factory
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HeavyProcessor();
}