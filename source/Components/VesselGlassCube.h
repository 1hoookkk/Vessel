/*
  ==============================================================================
    VESSEL - Authentic E-mu Z-Plane Emulation
    VesselGlassCube.h
    The "Glass Cube" 3D Visualization Component
  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <foleys_gui_magic/foleys_gui_magic.h>
#include "../DSP/zplane_visualizer_math.h"
#include "../DSP/EmuZPlaneCore.h"
#include "../DSP/EMUStaticShapeBank.h"
#include "../DSP/PresetConverter.h"

namespace vessel
{

class VesselGlassCube : public juce::Component
{
public:
    VesselGlassCube();
    ~VesselGlassCube() override = default;

    void paint(juce::Graphics& g) override;
    
    // Setter for state updates
    void updateState(float morph, float freq, float trans, int morphPair);

private:
    std::vector<std::vector<VisPoint3D>> ribbons;
    std::vector<std::vector<VisPoint3D>> gridLines;
    
    float currentMorph = 0.0f;
    float currentFreq = 0.0f;
    float currentTrans = 0.0f;
    int currentMorphPair = -1;
    
    EMUStaticShapeBank shapeBank;
    ZPlaneCube currentCube;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VesselGlassCube)
};

} // namespace vessel
