/*
  ==============================================================================
    VESSEL // WIREFRAME
    File: VectorScreen.h
    Desc: 3D Surface Visualization of Physics State (Minimalist).
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/WaitFreeTripleBuffer.h"
#include "SharedData.h"
#include "VesselLookAndFeel.h"

class VectorScreen : public juce::Component, public juce::Timer {
public:
    WaitFreeTripleBuffer<SystemState>& bufferRef;
    SystemState cachedState{};
    
    // Interaction Callback: returns normalized X, Y (0..1)
    std::function<void(float, float)> onDrag;

    // Palette
    const juce::Colour kBG       = juce::Colour(0xFF050505); 
    const juce::Colour kGrid     = juce::Colour(0xFF00FF41); // Bright Green
    const juce::Colour kGridDim  = juce::Colour(0xFF004010); // Dim Green

    VectorScreen(WaitFreeTripleBuffer<SystemState>& buffer) : bufferRef(buffer) {
        startTimerHz(60); 
    }

    void timerCallback() override {
        if (bufferRef.pull(cachedState)) {
            repaint();
        }
    }
    
    // INTERACTION LOGIC
    void mouseDown(const juce::MouseEvent& e) override {
        updateTarget(e.getPosition());
    }
    
    void mouseDrag(const juce::MouseEvent& e) override {
        updateTarget(e.getPosition());
    }
    
    void updateTarget(juce::Point<int> pos) {
        if (onDrag) {
            float w = (float)getWidth();
            float h = (float)getHeight();
            float x = std::clamp((float)pos.x / w, 0.0f, 1.0f);
            float y = std::clamp((float)pos.y / h, 0.0f, 1.0f);
            onDrag(x, y);
        }
    }

    void paint(juce::Graphics& g) override {
        const float w = (float)getWidth();
        const float h = (float)getHeight();
        
        g.fillAll(kBG);
        
        // Draw the 3D Grid
        drawGrid(g, w, h);
        
        // Draw Data Overlay (Minimal Text)
        drawInfo(g);
    }

private:
    void drawGrid(juce::Graphics& g, float w, float h) {
        // Simple Perspective Grid
        // We simulate a 3D floor plane
        
        float horizonY = h * 0.3f;
        float bottomY  = h * 0.9f;
        
        int numVLines = 12;
        int numHLines = 8;
        
        // Center of the "Action" based on physics
        float centerX = cachedState.centroidX * w; // 0..w
        // Use energy to modulate grid height
        float energy = cachedState.kineticEnergy;
        
        g.setColour(kGridDim);

        // Vertical Lines (perspective)
        for (int i = 0; i <= numVLines; ++i) {
            float t = (float)i / numVLines; // 0..1
            float xTop = w * (0.3f + t * 0.4f); // Converge at top
            float xBot = w * (0.0f + t * 1.0f); // Spread at bottom
            
            // Simple warping based on centroid
            float dist = std::abs(xBot - centerX);
            float warp = (1.0f - std::clamp(dist / (w*0.5f), 0.0f, 1.0f)) * energy * 50.0f;
            
            g.drawLine(xTop, horizonY, xBot, bottomY - warp, 1.0f);
        }
        
        // Horizontal Lines
        for (int i = 0; i <= numHLines; ++i) {
            float t = (float)i / numHLines;
            // Exponential spacing for depth
            float tExp = t * t; 
            float y = horizonY + tExp * (bottomY - horizonY);
            
            float warpY = std::sin(t * 10.0f + (float)juce::Time::getMillisecondCounter() * 0.002f) * energy * 10.0f;
            
            g.drawLine(w * 0.1f, y + warpY, w * 0.9f, y + warpY, 1.0f);
        }

        // Draw the "Cursor" / Source
        float cx = cachedState.centroidX * w;
        float cy = cachedState.centroidY * h;
        
        g.setColour(kGrid);
        g.drawVerticalLine((int)cx, 0.0f, h);
        g.drawHorizontalLine((int)cy, 0.0f, w);
        
        g.drawEllipse(cx - 5, cy - 5, 10, 10, 2.0f);
    }
    
    void drawInfo(juce::Graphics& g) {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 10.0f, juce::Font::plain)));
        
        g.drawText("POLES: " + juce::String(cachedState.leftPoles.size()), 10, 10, 100, 20, juce::Justification::topLeft);
        g.drawText("ENERGY: " + juce::String(cachedState.kineticEnergy, 2), 10, 25, 100, 20, juce::Justification::topLeft);
    }
};