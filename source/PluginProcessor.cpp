/*
  ==============================================================================
    HEAVY - Industrial Percussion Synthesizer
    File: PluginProcessor.cpp
    DSP logic: Material physics -> Z-Plane resonator
  ==============================================================================
*/

#include "PluginProcessor.h"
// #include "PluginEditor.h" // Removed
#include <array>
#include <cmath>

#include "Utility/WaitFreeTripleBuffer.h"
#include "Components/VesselXYPad.h"
#include "Components/VesselGlassCube.h"
#include "VesselStyle.h"

using namespace Heavy;

// Helper: GuiItem factory wrapper for vessel::VesselXYPad
static std::unique_ptr<foleys::GuiItem> createVesselXYPadFactory(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
{
    struct VesselXYPadItem : public foleys::GuiItem
    {
        vessel::VesselXYPad pad;
        HeavyProcessor* processor = nullptr;

        VesselXYPadItem(foleys::MagicGUIBuilder& b, const juce::ValueTree& n) : foleys::GuiItem(b, n)
        {
            addAndMakeVisible(pad);
            // Get reference to our specific processor
            processor = dynamic_cast<HeavyProcessor*>(b.getMagicState().getProcessor());
        }

        void update() override
        {
            if (processor)
            {
                // Retrieve display poles from the processor (thread-safe UI helper)
                auto poles = processor->getDisplayPoles();
                pad.setPoles(poles);
                
                // Update Glass Cube background state
                auto state = processor->tripleBuffer.getCurrentState();
                pad.updateGlassState(state.morph, state.freq, state.transformZ, state.morphPair);
            }
        }

        juce::Component* getWrappedComponent() override { return &pad; }
    };

    return std::make_unique<VesselXYPadItem>(builder, node);
}

// Helper: GuiItem factory wrapper for vessel::VesselGlassCube
// (Kept for reference, but XML will use VesselXYPad which now includes GlassCube)
static std::unique_ptr<foleys::GuiItem> createVesselGlassCubeFactory(foleys::MagicGUIBuilder& builder, const juce::ValueTree& node)
{
    struct VesselGlassCubeItem : public foleys::GuiItem
    {
        vessel::VesselGlassCube cube;
        HeavyProcessor* processor = nullptr;

        VesselGlassCubeItem(foleys::MagicGUIBuilder& b, const juce::ValueTree& n) : foleys::GuiItem(b, n)
        {
            addAndMakeVisible(cube);
            // Get reference to our specific processor
            processor = dynamic_cast<HeavyProcessor*>(b.getMagicState().getProcessor());
        }

        void update() override
        {
            if (processor)
            {
                // Retrieve state from triple buffer
                auto state = processor->tripleBuffer.getCurrentState();
                cube.updateState(state.morph, state.freq, state.transformZ, state.morphPair);
            }
        }

        juce::Component* getWrappedComponent() override { return &cube; }
    };

    return std::make_unique<VesselGlassCubeItem>(builder, node);
}

// HeavyProcessor::HeavyProcessor() ... is in header

void HeavyProcessor::initialiseBuilder (foleys::MagicGUIBuilder& builder)
{
    // Register custom components (wrap the JUCE component in a foleys::GuiItem)
    builder.registerFactory("VesselXYPad", &createVesselXYPadFactory);
    builder.registerFactory("VesselGlassCube", &createVesselGlassCubeFactory);
    
    // (Optional) Industrial LookAndFeel registration intentionally omitted here
    // to avoid ownership/initialization ordering issues within foleys builder.
}

juce::ValueTree HeavyProcessor::createGuiValueTree()
{
    // MINIMAL STRIPPED UI - FLEXBOX LAYOUT
    // This defines the "Gold Master" minimal layout programmatically.
    // In production, this would be loaded from magic.xml binary resource.
    
    juce::String xml = R"(
    <magic>
      <Styles>
        <Style name="default">
          <Nodes lookAndFeel="Vessel"/>
          <Classes>
            <plot-view border="2" background="FF121214" margin="10"/>
            <group margin="5" padding="5" border="0" background="FF121214"/>
            <label font-size="14" font-prop="bold" text="FF888888"/>
            <slider slider-type="rotary" slider-textbox="no-textbox" rotary-fill="FFFF3300" rotary-outline="FF252528" thumb="FFFF3300"/>
          </Classes>
          <Palettes/>
        </Style>
      </Styles>
      <View id="root" class="group" flex-direction="column">
        <Label text="VESSEL // SYSTEM_A01" font-size="20" height="40" justification="centred" text="FFDDDDDD"/>
        
        <View flex-direction="row" flex="1.0">
          <!-- MAIN VIEW: HYBRID (XY Control + Glass Cube Background) -->
          <View class="group" flex="2.0" margin="10" padding="0" border="2">
             <VesselXYPad parameter-x="TENSION" parameter-y="HARDNESS" radius="8" line-thickness="2.0" dot-type="pole-zero" flex="1.0"/>
          </View>
          
          <!-- SIDEBAR: MACROS -->
          <View class="group" flex="1.0" flex-direction="column" margin="10" border="1" background="FF0E0E10">
             <Label text="ALLOY"/>
             <Slider parameter="ALLOY" flex="1.0" slider-type="linear-horizontal"/>

             <Label text="FLUX"/>
             <Slider parameter="FLUX" flex="1.0" slider-type="linear-horizontal"/>

             <Label text="FILTER MODEL"/>
             <ComboBox parameter="FILTER_MODEL" flex="1.0"/>
             
             <Label text="MIX"/>
             <Slider parameter="MIX" flex="1.0" slider-type="linear-horizontal"/>
             
             <Label text="OUTPUT"/>
             <Slider parameter="OUTPUT" flex="1.0" slider-type="linear-horizontal"/>
          </View>
        </View>
      </View>
    </magic>
    )";

    return juce::ValueTree::fromXml(xml);
}

// Remove createEditor implementation
// juce::AudioProcessorEditor* HeavyProcessor::createEditor() { return new HeavyEditor(*this); }

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

    const int numSamples = buffer.getNumSamples();
    float sampleRate = (float)getSampleRate();
    if (sampleRate <= 0.0f) sampleRate = 48000.0f;

    // Clear unused channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, numSamples);

    // ---------------------------------------------------------------------
    // 1. READ PARAMETERS (Thread-Safe: Direct atomic reads)
    // ---------------------------------------------------------------------
    // Note: getRawParameterValue returns atomic<float>*, dereference is atomic on modern CPUs
    float tension  = treeState.getRawParameterValue("TENSION")->load();   // 0-1 (head tightening)
    float hardness = treeState.getRawParameterValue("HARDNESS")->load();  // 0-1 (damping)
    float alloy    = treeState.getRawParameterValue("ALLOY")->load();     // 0-1 (material path)
    float mix      = treeState.getRawParameterValue("MIX")->load();       // 0-1
    float outputDb = treeState.getRawParameterValue("OUTPUT")->load();    // dB
    float fluxAmt  = treeState.getRawParameterValue("FLUX")->load();      // 0-1 depth for Z mod
    int   filterModel = static_cast<int>(std::round(treeState.getRawParameterValue("FILTER_MODEL")->load()));

    // Update material state for exciters/labels
    const int materialIndex = juce::jlimit(0, 7, juce::roundToInt(alloy * 7.0f));
    MaterialType material = static_cast<MaterialType>(materialIndex);
    if (material != currentMaterial_)
    {
        currentMaterial_ = material;
        currentProps_ = MaterialProperties::get(material);
    }

    // ---------------------------------------------------------------------
    // 2. MAP MACROS -> H-CHIP (k1/k2 + cube focus)
    // ---------------------------------------------------------------------
    ZPlaneParams zParams = updateVoicePhysics(tension, hardness, alloy);

    // Legacy filter model selection (dropdown overrides path pair)
    filterModel = juce::jlimit(0, shapeBank.numPairs() - 1, filterModel);
    zParams.morphPair = filterModel;

    // Simple envelope follower for Z-axis motion ("Flux")
    float env = envFollower_;
    const float releaseCoeff = 0.999f; // slow release, instant attack
    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
    for (int i = 0; i < numSamples; ++i)
    {
        float inputAbs = std::abs(inL[i]);
        if (inR) inputAbs = juce::jmax(inputAbs, std::abs(inR[i]));

        if (inputAbs > env)
            env = inputAbs;
        else
            env *= releaseCoeff;
    }
    env = juce::jlimit(0.0f, 1.0f, env);
    envFollower_ = env;
    // Smooth the FLUX control per-block to reduce stepping on fast transients
    {
        const float blockDuration = (float)numSamples / sampleRate; // seconds
        const float tau = 0.010f; // 10 ms smoothing time constant
        const float smoothAlpha = 1.0f - std::exp(-blockDuration / tau);
        smoothedFlux_ += smoothAlpha * (fluxAmt - smoothedFlux_);
    }
    const float fluxDepth = juce::jlimit(0.0f, 1.0f, smoothedFlux_);
    zParams.trans = juce::jlimit(0.0f, 1.0f, zParams.trans + env * fluxDepth);
    
    // Check if Morph Pair Changed, Update Cube
    if (zParams.morphPair != currentParams.morphPair) {
        auto pair = shapeBank.morphPairIndices(zParams.morphPair);
        currentCube = PresetConverter::convertToCube(pair.first, pair.second);
    }
    currentParams = zParams;

    // ---------------------------------------------------------------------
    // 3. INTERPOLATE FRAME (The Brain)
    // ---------------------------------------------------------------------
    // ARMAdillo engine calculates the target frame for this block
    // We pass (morph, freq, trans) -> (x, y, z)
    ZPlaneFrame targetFrame = ARMAdilloEngine::interpolate_cube(currentCube, zParams.morph, zParams.freq, zParams.trans);

    // ---------------------------------------------------------------------
    // 4. AUDIO PROCESSING (The Body)
    // ---------------------------------------------------------------------

    // Store dry signal for mix (use pre-allocated buffer)
    dryBuffer_.makeCopyOf(buffer, true);

    // AUTO-GAIN: Measure input RMS
    float inputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (buffer.getNumChannels() > 1)
        inputRMS = (inputRMS + buffer.getRMSLevel(1, 0, numSamples)) * 0.5f;

    // Apply Drive (Input Gain)
    if (zParams.driveDb > 0.1f) {
        float driveGain = std::pow(10.0f, zParams.driveDb / 20.0f);
        buffer.applyGain(driveGain);
    }

    // Process Filters
    auto* l = buffer.getWritePointer(0);
    auto* r = buffer.getWritePointer(1);

    process_block(&engineL, l, numSamples, targetFrame, sampleRate);
    process_block(&engineR, r, numSamples, targetFrame, sampleRate);

    // AUTO-GAIN: Measure output RMS and compensate
    float outputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (buffer.getNumChannels() > 1)
        outputRMS = (outputRMS + buffer.getRMSLevel(1, 0, numSamples)) * 0.5f;

    // Calculate dynamic compensation (with smoothing to prevent pumping)
    const float smoothingCoeff = 0.05f; // ~20ms at 44.1kHz
    smoothedInputRMS_ += smoothingCoeff * (inputRMS - smoothedInputRMS_);
    smoothedOutputRMS_ += smoothingCoeff * (outputRMS - smoothedOutputRMS_);

    // STABILITY CHECK: If output is NaN or inf, reset filters immediately
    if (!std::isfinite(outputRMS) || outputRMS > 10.0f)
    {
        engineL.resetStates();
        engineR.resetStates();
        smoothedInputRMS_ = 0.0f;
        smoothedOutputRMS_ = 0.0f;
        smoothedCompensationGain_ = 1.0f;
        buffer.clear(); // Mute this block
    }
    else
    {
        // Compensation ratio (with TIGHTER safety limits for extreme settings)
        float targetGain = 1.0f;
        if (smoothedOutputRMS_ > 0.0001f && smoothedInputRMS_ > 0.0001f)
        {
            targetGain = smoothedInputRMS_ / smoothedOutputRMS_;
            targetGain = juce::jlimit(0.1f, 10.0f, targetGain); // Tighter: prevent runaway
        }

        // Smooth the compensation gain to prevent zipper noise
        smoothedCompensationGain_ += smoothingCoeff * (targetGain - smoothedCompensationGain_);

        // Apply auto-gain + fixed makeup
        // REDUCED: 24dB was way too hot, especially at high resonance
        const float baselineMakeup = juce::Decibels::decibelsToGain(12.0f); // Back to sane levels
        const float totalGain = smoothedCompensationGain_ * baselineMakeup;
        buffer.applyGain(totalGain);
    }

    // Wet/Dry mix
    const float wetAmt = juce::jlimit(0.0f, 1.0f, mix);
    const float dryAmt = 1.0f - wetAmt;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dryPtr = dryBuffer_.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            wet[i] = wet[i] * wetAmt + dryPtr[i] * dryAmt;
        }
    }

    // DC blocker + mild reconstruction low-pass (post mix)
    float prevInL = dcPrevInputL_, prevInR = dcPrevInputR_;
    float dcStateL = dcBlockerStateL_, dcStateR = dcBlockerStateR_;
    float reconL = reconStateL_, reconR = reconStateR_;
    const float reconKeep = reconCoeff_;
    const float reconFeed = 1.0f - reconKeep;

    for (int i = 0; i < numSamples; ++i)
    {
        // High-pass to remove DC bias
        float dcOutL = l[i] - prevInL + dcPole_ * dcStateL;
        float dcOutR = r[i] - prevInR + dcPole_ * dcStateR;

        prevInL = l[i];
        prevInR = r[i];
        dcStateL = dcOutL;
        dcStateR = dcOutR;

        // Gentle HF smoothing to emulate reconstruction filter
        dcOutL = reconFeed * dcOutL + reconKeep * reconL;
        dcOutR = reconFeed * dcOutR + reconKeep * reconR;
        reconL = dcOutL;
        reconR = dcOutR;

        l[i] = dcOutL;
        r[i] = dcOutR;
    }

    dcPrevInputL_ = prevInL; dcPrevInputR_ = prevInR;
    dcBlockerStateL_ = dcStateL; dcBlockerStateR_ = dcStateR;
    reconStateL_ = reconL; reconStateR_ = reconR;

    // Master output gain
    const float outputGain = juce::Decibels::decibelsToGain(outputDb);
    buffer.applyGain(outputGain);

    // ---------------------------------------------------------------------
    // 6. METERING (for reactive visualization)
    // ---------------------------------------------------------------------
    float finalRMS = buffer.getRMSLevel(0, 0, numSamples);
    float finalPeak = buffer.getMagnitude(0, 0, numSamples);

    if (buffer.getNumChannels() > 1)
    {
        finalRMS = (finalRMS + buffer.getRMSLevel(1, 0, numSamples)) * 0.5f;
        finalPeak = juce::jmax(finalPeak, buffer.getMagnitude(1, 0, numSamples));
    }

    // ---------------------------------------------------------------------
    // 7. UPDATE UI STATE (TRIPLE BUFFER)
    // ---------------------------------------------------------------------
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
        
        state.morph = zParams.morph;
        state.freq = zParams.freq;
        state.transformZ = zParams.trans;
        state.morphPair = zParams.morphPair;

        // Clear deprecated fields
        state.particles.activeCount = 0;

        tripleBuffer.push();
    }
}

// -------------------------------------------------------------------------
// UI HELPER METHODS
// -------------------------------------------------------------------------

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

    // �� TENSION -> k1 (frequency) ������������������������������������������
    constexpr float minHz = 40.0f;      // Sub thud
    constexpr float maxHz = 12000.0f;   // Tight ping
    constexpr float tensionExp = 1.8f;  // Inverse-log feel

    const float tensionSkew = std::pow(t, tensionExp);
    float freqHz = minHz * std::pow(maxHz / minHz, tensionSkew);
    freqHz = juce::jlimit(minHz, nyquist, freqHz);

    const float invLog1000 = 1.0f / std::log(1000.0f);
    float k1 = juce::jlimit(0.0f, 1.0f, std::log(freqHz / 20.0f) * invLog1000);

    // �� HARDNESS -> k2 (resonance / damping) �������������������������������
    constexpr float k2Min = 0.05f;      // Dead/matte (lowered for more dynamic range)
    constexpr float k2Max = 0.985f;     // Stability guard (push closer to edge)
    constexpr float hardnessExp = 0.7f; // Aggressive curve (< 1.0 = more top-end sensitivity)

    const float hardnessSkew = std::pow(h, hardnessExp);
    // Remove ring emphasis bump - let it hit hard at full hardness
    float k2 = juce::jlimit(k2Min, k2Max, k2Min + (k2Max - k2Min) * hardnessSkew);

    // �� ALLOY PATH -> 3D cube focus (X,Y,Z) ��������������������������������
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
