/*
  ==============================================================================
    VESSEL - Authentic E-mu Z-Plane Emulation
    VesselGlassCube.cpp
  ==============================================================================
*/

#include "VesselGlassCube.h"
#include "../VesselStyle.h"

namespace vessel
{

VesselGlassCube::VesselGlassCube()
{
    setOpaque(false);
    // Pre-calculate grid
    gridLines = ZPlaneVisualizer::generate_perspective_grid();
    
    // Initialize default cube
    currentCube = PresetConverter::convertToCube(0, 1);
}

void VesselGlassCube::updateState(float morph, float freq, float trans, int morphPair)
{
    bool needsRepaint = false;
    
    if (morphPair != currentMorphPair)
    {
        currentMorphPair = morphPair;
        auto pair = shapeBank.morphPairIndices(morphPair);
        currentCube = PresetConverter::convertToCube(pair.first, pair.second);
        needsRepaint = true;
    }
    
    // Check for significant changes to avoid recalculating ribbons if idle
    const float threshold = 0.001f;
    if (std::abs(morph - currentMorph) > threshold ||
        std::abs(freq - currentFreq) > threshold ||
        std::abs(trans - currentTrans) > threshold ||
        needsRepaint)
    {
        currentMorph = morph;
        currentFreq = freq;
        currentTrans = trans;
        
        // Generate ribbons using the visualizer math
        // This simulates the frequency response of the cube at different morph depths
        ribbons = ZPlaneVisualizer::generate_ribbons(currentCube, freq, trans, 48000.0f); // Default SR
        needsRepaint = true;
    }
    
    if (needsRepaint)
        repaint();
}

void VesselGlassCube::paint(juce::Graphics& g)
{
    // Background (Dark)
    g.fillAll(Vessel::screenInk); // Dark background
    
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    
    // Projection Helpers
    auto project = [&](const VisPoint3D& p) -> juce::Point<float> {
        // Simple perspective projection
        // z is 0..1 (front to back)
        // x is 0..1 (left to right)
        // y is 0..1 (bottom to top magnitude)
        
        float depth = 1.0f + p.z * 1.5f; // Scale factor based on Z
        
        // Center X,Y around 0 before projection
        float x = (p.x * 2.0f - 1.0f);
        float y = (p.y * 2.0f - 1.0f); // Map 0..1 to -1..1
        
        // Project
        float sx = x / depth;
        float sy = y / depth;
        
        // Map back to screen coords
        // Scale to fit
        float scale = std::min(w, h) * 0.45f;
        float screenX = w * 0.5f + sx * w * 0.5f; // Stretch to width?
        float screenY = h * 0.8f - sy * scale; // Offset floor
        
        return {screenX, screenY};
    };
    
    // Draw Grid
    g.setColour(Vessel::inset.withAlpha(0.2f));
    for (const auto& line : gridLines)
    {
        juce::Path p;
        if (line.size() >= 2)
        {
            // Floor grid (Y=0)
            VisPoint3D p1 = line[0]; p1.y = 0.0f;
            VisPoint3D p2 = line[1]; p2.y = 0.0f;
            
            p.startNewSubPath(project(p1));
            p.lineTo(project(p2));
            g.strokePath(p, juce::PathStrokeType(1.0f));
        }
    }
    
    // Draw Ribbons
    for (size_t i = 0; i < ribbons.size(); ++i)
    {
        const auto& ribbon = ribbons[i];
        if (ribbon.empty()) continue;
        
        // Color based on depth
        float depth = (float)i / (float)ribbons.size();
        juce::Colour color = (i == 0) ? Vessel::accent : Vessel::screenBg.withAlpha(0.3f * (1.0f - depth));
        
        g.setColour(color);
        
        juce::Path p;
        bool first = true;
        for (const auto& pt : ribbon)
        {
            auto projected = project(pt);
            if (first) { p.startNewSubPath(projected); first = false; }
            else { p.lineTo(projected); }
        }
        
        // Optional: Fill for the front ribbon
        if (i == 0)
        {
            // GLOW EFFECT (Manual multi-pass stroke)
            g.setColour(Vessel::accent.withAlpha(0.15f));
            g.strokePath(p, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved));
            
            g.setColour(Vessel::accent.withAlpha(0.4f));
            g.strokePath(p, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved));
            
            // Main Solid Stroke
            g.setColour(Vessel::accent);
            g.strokePath(p, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved));

            // Fill under curve
            auto pFill = p;
            pFill.lineTo(project({ribbon.back().x, 0.0f, ribbon.back().z})); // Drop to floor
            pFill.lineTo(project({ribbon.front().x, 0.0f, ribbon.front().z})); // Back to start
            pFill.closeSubPath();
            
            g.setGradientFill(juce::ColourGradient(
                Vessel::accent.withAlpha(0.25f), 0, h * 0.5f,
                Vessel::accent.withAlpha(0.0f), 0, h * 0.9f,
                false));
            g.fillPath(pFill);
        }
        else 
        {
            g.strokePath(p, juce::PathStrokeType(1.0f));
        }
    }
    
    // Draw Puck (Current State)
    // Morph (Z), Freq (X)
    VisPoint3D puckPos { currentFreq, 0.0f, currentMorph }; // Y=0 (Floor)
    auto puckScreen = project(puckPos);
    
    float puckSize = 8.0f / (1.0f + currentMorph);
    g.setColour(Vessel::accent);
    g.fillEllipse(puckScreen.x - puckSize, puckScreen.y - puckSize*0.5f, puckSize*2.0f, puckSize);
    
    // Glow
    g.setColour(Vessel::accent.withAlpha(0.3f));
    g.drawEllipse(puckScreen.x - puckSize*2, puckScreen.y - puckSize, puckSize*4.0f, puckSize*2.0f, 1.0f);
}

} // namespace vessel
