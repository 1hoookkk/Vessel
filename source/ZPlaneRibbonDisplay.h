#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/WaitFreeTripleBuffer.h"
#include "SharedData.h"
#include "DSP/EmuZPlaneCore.h"
#include "DSP/PresetConverter.h"
#include "DSP/zplane_visualizer_math.h"

class ZPlaneRibbonDisplay : public juce::Component, public juce::Timer {
public:
    WaitFreeTripleBuffer<SystemState>& bufferRef;
    SystemState cachedState{};
    
    // Local copy of the filter cube for visualization
    ZPlaneCube vizCube;
    int currentModeIndex = -1;

    // Colors (from CLAUDE.md)
    const juce::Colour kActiveCurve = juce::Colours::gold;
    const juce::Colour kFuturePast  = juce::Colours::blueviolet.withAlpha(0.3f);
    const juce::Colour kGrid        = juce::Colours::cyan.withAlpha(0.2f);
    const juce::Colour kBackground  = juce::Colour(0xFF050505);

    ZPlaneRibbonDisplay(WaitFreeTripleBuffer<SystemState>& buffer) : bufferRef(buffer) {
        startTimerHz(30); // 30 FPS for heavy ribbon calc
    }

    void timerCallback() override {
        if (bufferRef.pull(cachedState)) {
            // Check if mode changed
            if (cachedState.modeIndex != currentModeIndex) {
                currentModeIndex = cachedState.modeIndex;
                updateCube();
            }
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(kBackground);
        
        const float w = (float)getWidth();
        const float h = (float)getHeight();

        // Generate Geometry
        // Use params from cachedState
        // Note: sample rate fixed at 48k for visualizer or passed in?
        // Ideally passed in via SystemState, but 48k is fine for vis.
        auto ribbons = ZPlaneVisualizer::generate_ribbons(
            vizCube, 
            cachedState.centroidY, // Freq (Y)
            cachedState.transformZ, // Trans (Z)
            48000.0f
        );

        auto gridLines = ZPlaneVisualizer::generate_perspective_grid();

        // Render
        // Perspective Projection Helper
        auto project = [&](VisPoint3D p) -> juce::Point<float> {
            // Simple perspective: x = freq, y = mag, z = depth
            // Screen X = x * scale + z * offset
            // Screen Y = y * scale + z * offset
            
            // Lexicon Style: Depth goes "into" the screen (scale shrinks)
            float depthScale = 1.0f - (p.z * 0.4f); // Back is smaller
            float screenX = p.x * w * 0.8f + w * 0.1f; // Margin
            // Shift X based on Z to give "angle"
            screenX += (p.z * w * 0.1f); 
            
            float screenY = (1.0f - p.y) * h * 0.8f + h * 0.1f; // Invert Y (0 is bottom)
            // Shift Y up/down based on Z? Usually floor is flat.
            screenY -= (p.z * h * 0.1f);

            return {screenX, screenY};
        };

        // Draw Grid
        g.setColour(kGrid);
        for (const auto& line : gridLines) {
            juce::Path p;
            auto p1 = project(line[0]);
            auto p2 = project(line[1]);
            g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.0f);
        }

        // Draw Ribbons
        // We want to draw from back (z=1) to front (z=0) for painter's algorithm
        // generate_ribbons returns 0..1 (front to back?). 
        // Visualizer logic: i=0 -> morph_x=0.
        // If Morph=0 is "Front", then i=SLICE_COUNT-1 is "Back".
        // We should iterate reverse.
        
        for (int i = (int)ribbons.size() - 1; i >= 0; --i) {
            const auto& ribbon = ribbons[i];
            if (ribbon.empty()) continue;

            // Determine color
            // Active slice is closest to currentMorph?
            // currentMorph is cachedState.centroidX
            float sliceMorph = ribbon[0].z;
            float dist = std::abs(sliceMorph - cachedState.centroidX);
            
            juce::Colour col = kFuturePast;
            float thick = 1.0f;
            
            // Highlight active zone
            if (dist < 0.1f) {
                float ratio = 1.0f - (dist / 0.1f);
                col = kFuturePast.interpolatedWith(kActiveCurve, ratio);
                thick = 1.0f + ratio * 2.0f;
            }

            g.setColour(col);
            
            juce::Path path;
            auto start = project(ribbon[0]);
            path.startNewSubPath(start);
            
            for (size_t j = 1; j < ribbon.size(); ++j) {
                path.lineTo(project(ribbon[j]));
            }
            
            g.strokePath(path, juce::PathStrokeType(thick));
        }
    }

private:
    void updateCube() {
        // Map modeIndex to Shape Pair
        // Access static bank from PresetConverter or EMUStaticShapeBank?
        // Wait, PresetConverter::convertToCube takes (indexA, indexB).
        // I need to know which indices correspond to 'modeIndex'.
        // EMUStaticShapeBank has `morphPairIndices`.
        // I should probably include EMUStaticShapeBank.h to look it up.
        
        // Or replicate the lookup here since it's simple.
        // See EMUAuthenticTables.h:
        /*
        static const int MORPH_PAIRS[6][2] = {
            { 0, 12 }, // Vowel_Ae <-> Vowel_Ih
            { 7, 5 },  // Bell_Metallic <-> Metallic
            { 3, 18 }, // Resonant Peak <-> Formant Filter
            { 3, 4 },  // Resonant Peak <-> Wide Spectrum
            { 1, 8 },  // Vocal Morph <-> Aggressive Lead
            { 16, 31 } // Classic Sweep <-> Ultimate
        };
        */
       
       // Hardcoding for now to avoid another dependency, or include EMUAuthenticTables.h via PresetConverter.h
       // PresetConverter.h includes EMUAuthenticTables.h. So `MORPH_PAIRS` is visible if static.
       // Yes, `static const int MORPH_PAIRS...` in header.
       
       if (currentModeIndex >= 0 && currentModeIndex < 6) {
           int a = MORPH_PAIRS[currentModeIndex][0];
           int b = MORPH_PAIRS[currentModeIndex][1];
           vizCube = PresetConverter::convertToCube(a, b);
       }
    }
};
