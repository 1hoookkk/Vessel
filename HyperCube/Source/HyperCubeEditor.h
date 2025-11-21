#pragma once

#include <JuceHeader.h>
#include "HyperCubeProcessor.h"
#include "ZPlaneVisualizer.h" // For math structures if needed in UI

// ==============================================================================
// HyperCubeEditor
// The "Glass Cube" Visualization
// ==============================================================================
class HyperCubeEditor : public juce::AudioProcessorEditor,
                        public juce::OpenGLRenderer,
                        public juce::Timer
{
public:
    HyperCubeEditor(HyperCubeProcessor&);
    ~HyperCubeEditor() override;

    // Component Overrides
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Interactions
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    // OpenGL Overrides
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    
    // Timer (Animation Loop)
    void timerCallback() override;

private:
    HyperCubeProcessor& audioProcessor;
    juce::OpenGLContext openGLContext;

    // UI State
    float currentMorph = 0.0f;
    float currentFreq = 0.5f;
    
    // Visualizer Data
    std::vector<std::vector<VisPoint3D>> ribbons;
    std::vector<std::vector<VisPoint3D>> gridLines;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gritAtt;

    // UI Components (Hidden/Overlay)
    juce::Slider morphSlider;
    juce::Slider freqSlider;
    juce::Slider transformSlider;
    juce::ToggleButton gritButton;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperCubeEditor)
};
