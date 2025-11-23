/*
  ==============================================================================
    PluginEditor.cpp
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

HeavyEditor::HeavyEditor (HeavyProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Apply Vessel Design
    setLookAndFeel(&vesselLook);

    // Helper to init "Industrial Bar" sliders
    auto setupBar = [&](juce::Slider& s, juce::Label& l, std::unique_ptr<Attachment>& att, juce::String pID, juce::String title) {
        s.setSliderStyle(juce::Slider::LinearBar);
        addAndMakeVisible(s);
        
        l.setText(title, juce::dontSendNotification);
        l.setFont(Vessel::getTechFont(10.0f, true));
        l.setColour(juce::Label::textColourId, Vessel::text);
        addAndMakeVisible(l);
        
        att = std::make_unique<Attachment>(audioProcessor.treeState, pID, s);
    };

    // 1. SETUP TOP CONTROLS (Vertical Layout conceptually)
    setupBar(hardnessSlider, lblHardness, hardAtt, "HARDNESS", "HARDNESS");
    setupBar(alloySlider,    lblAlloy,    alloyAtt, "ALLOY",    "ALLOY");
    setupBar(mixSlider,      lblMix,      mixAtt,   "MIX",      "MIX");
    setupBar(outputSlider,   lblOut,      outAtt,   "OUTPUT",   "LEVEL");

    // 2. SETUP TENSION (The "Flux" Bottom Slider)
    tensionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tensionSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(tensionSlider);
    
    lblTension.setText("TENSION / FLUX", juce::dontSendNotification);
    lblTension.setFont(Vessel::getTechFont(11.0f, true));
    lblTension.setColour(juce::Label::textColourId, Vessel::text);
    addAndMakeVisible(lblTension);
    
    tensAtt = std::make_unique<Attachment>(audioProcessor.treeState, "TENSION", tensionSlider);

    // 3. INIT VISUALIZER
    oilBuffer = juce::Image(juce::Image::ARGB, 64, 48, true); // Low res for "Gameboy" pixelation

    setSize (800, 500);
    startTimer(30); // 30Hz Repaint
}

HeavyEditor::~HeavyEditor()
{
    setLookAndFeel(nullptr);
}

void HeavyEditor::timerCallback()
{
    repaint();
}

void HeavyEditor::paint (juce::Graphics& g)
{
    // A. CHASSIS
    g.fillAll(Vessel::bg);

    // B. LAYOUT PREP (Manual Grid calc)
    auto bounds = getLocalBounds().reduced(24);
    auto bottomStrip = bounds.removeFromBottom(60);
    auto topArea = bounds;

    // C. VISUALIZER (Left side)
    auto screenRect = topArea.removeFromLeft(500).reduced(0, 10);
    
    // Draw Inset Bezel
    Vessel::drawRecessedTrack(g, screenRect);
    
    // 1. Render POLES to Oil Buffer
    // This runs on UI thread, but gets atomic data from processor
    {
        juce::Graphics oilG(oilBuffer);
        oilG.fillAll(Vessel::screenBg); // Clear LCD
        
        auto poles = audioProcessor.getDisplayPoles(); // CONNECTED!
        
        oilG.setColour(Vessel::screenInk);
        for (auto p : poles)
        {
            // Map Normalized (0..1) to (0..64)
            // Flip Y because DSP Y=1 is high freq usually
            float x = p.getX() * oilBuffer.getWidth(); 
            float y = (1.0f - p.getY()) * oilBuffer.getHeight();
            
            // Draw metaball blob
            float size = 8.0f; 
            oilG.fillEllipse(x - size/2, y - size/2, size, size);
        }
    }
    
    // 2. Upscale Buffer to Screen (Nearest Neighbor)
    g.setImageResamplingQuality(juce::Graphics::lowResamplingQuality);
    g.drawImage(oilBuffer, screenRect.toFloat());
    
    // 3. Grid Overlay
    g.setColour(Vessel::border.withAlpha(0.3f));
    for (int i=0; i < screenRect.getWidth(); i+=10)
        g.drawVerticalLine(screenRect.getX() + i, screenRect.getY(), screenRect.getBottom());


    // D. LABELS & LAYOUT FOR SLIDERS
    // We already attached them, now we align them relative to controls
    // Using simple offset logic instead of complex flexbox for speed here
}

void HeavyEditor::resized()
{
    auto bounds = getLocalBounds().reduced(24);
    
    // 1. Bottom Flux Section
    auto bottomArea = bounds.removeFromBottom(50);
    lblTension.setBounds(bottomArea.removeFromLeft(100));
    tensionSlider.setBounds(bottomArea);

    bounds.removeFromBottom(20); // Spacer

    // 2. Right Control Column
    auto visualizerArea = bounds.removeFromLeft(500);
    auto controlsArea = bounds;
    controlsArea.removeFromLeft(30); // Spacer

    int h = 60; // Row height
    
    // Hardness
    auto row1 = controlsArea.removeFromTop(h);
    lblHardness.setBounds(row1.removeFromTop(15));
    hardnessSlider.setBounds(row1.reduced(0, 5));

    // Alloy
    auto row2 = controlsArea.removeFromTop(h);
    lblAlloy.setBounds(row2.removeFromTop(15));
    alloySlider.setBounds(row2.reduced(0, 5));
    
    // Mix
    auto row3 = controlsArea.removeFromTop(h);
    lblMix.setBounds(row3.removeFromTop(15));
    mixSlider.setBounds(row3.reduced(0, 5));

    // Output
    auto row4 = controlsArea.removeFromTop(h);
    lblOut.setBounds(row4.removeFromTop(15));
    outputSlider.setBounds(row4.reduced(0, 5));
}
// ... existing code ...

// --- THE VISUALIZER BRIDGE ---
// This calculates the positions of the "Magnets" (Poles) for the UI.
// It runs on the Message Thread, so it reads from the safe TripleBuffer.
std::vector<juce::Point<float>> HeavyProcessor::getDisplayPoles() const
{
    std::vector<juce::Point<float>> displayPoles;

    // 1. READ SAFE STATE (Wait-Free)
    // We get the most recent valid audio state without locking the audio thread
    SystemState uiState = tripleBuffer.getBackBuffer(); 

    // 2. GENERATE VISUALIZATION
    // Since we want the "Z-Plane" look, we calculate based on the current morph parameters.
    
    // TENSION = Rotation (Angle)
    // HARDNESS = Resonance (Radius)
    // ALLOY = Spacing/spread
    
    float tension = *treeState.getRawParameterValue("TENSION");
    float hardness = *treeState.getRawParameterValue("HARDNESS");
    float alloy = *treeState.getRawParameterValue("ALLOY");

    // Create 14 poles (7 pairs) to match your Z-Plane filter
    // This logic ensures something beautiful appears even before DSP is perfect
    int numPoles = 14;
    
    for (int i = 0; i < numPoles; ++i)
    {
        // Calculate basic circular distribution
        float angleNormalized = (float)i / (float)numPoles;
        float baseAngle = angleNormalized * juce::MathConstants<float>::twoPi;
        
        // Apply "Tension" (Rotation shift)
        float currentAngle = baseAngle + (tension * 3.0f); 
        
        // Apply "Hardness" (Radius/Magnitude)
        // Harder materials push poles closer to unit circle (more resonance)
        float radius = 0.4f + (hardness * 0.55f); 
        
        // Apply "Alloy" (Morphing shape complexity)
        // This warps the circle into an ellipse or clover
        float warp = std::sin(currentAngle * 3.0f) * (alloy * 0.3f);
        radius += warp;

        // Convert to Cartesian X/Y for the Oil Renderer (0.0 to 1.0 range)
        // 0.5 is center
        float x = 0.5f + (std::cos(currentAngle) * radius * 0.4f);
        float y = 0.5f + (std::sin(currentAngle) * radius * 0.4f);

        displayPoles.push_back({ x, y });
    }

    return displayPoles;
}