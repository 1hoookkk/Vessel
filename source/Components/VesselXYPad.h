/*
 ==============================================================================
    VESSEL - Authentic E-mu Z-Plane Emulation
    VesselXYPad.h
    
    Adapted from foleys_XYDragComponent by Daniel Walz & Julius O. Smith III.
    Includes JOS's "pole-zero" branch enhancements for plotting Poles/Zeros.
 ==============================================================================
 */

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <foleys_gui_magic/foleys_gui_magic.h> // For ParameterAttachment
#include "VesselGlassCube.h"

namespace vessel
{

// JOS's DotType Enum
enum DOT_TYPE { DOT_TYPE_DOT, DOT_TYPE_POLE, DOT_TYPE_ZERO, DOT_TYPE_POLE_ZERO };

/**
 A 2D parameter dragging component with Pole/Zero visualization support.
 Adapted from foleys::XYDragComponent.
 */
class VesselXYPad  : public juce::Component,
                     public juce::SettableTooltipClient
{
public:

    enum ColourIds
    {
        xyDotColourId = 0x2002000,
        xyDotOverColourId,
        xyHorizontalColourId,
        xyHorizontalOverColourId,
        xyVerticalColourId,
        xyVerticalOverColourId
    };

    VesselXYPad();

    void setCrossHair (bool horizontal, bool vertical);
    void setDotType (DOT_TYPE dotType);

    void paint (juce::Graphics& g) override;

    void setParameterX (juce::RangedAudioParameter* parameter);
    void setParameterY (juce::RangedAudioParameter* parameter);
    void setWheelParameter (juce::RangedAudioParameter* parameter);

    void setRightClickParameter (juce::RangedAudioParameter* parameter);

    void setRadius (float radius);
    void setLineThickness (float thickness);
    void setSenseFactor (float factor);
    void setJumpToClick (bool shouldJumpToClick);

    // Visualization: Display poles from Z-Plane engine
    void setPoles(const std::vector<juce::Point<float>>& poles);
    
    // Update the background Glass Cube visualization
    void updateGlassState(float morph, float freq, float trans, int morphPair);

    bool hitTest (int x, int y) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    
    void resized() override;

private:

    void updateWhichToDrag (juce::Point<float> p);

    int getXposition() const;
    int getYposition() const;

    VesselGlassCube glassCube;

    bool mouseOverDot = false;
    bool mouseOverX   = false;
    bool mouseOverY   = false;

    bool wantsHorizontalDrag = true;
    bool wantsVerticalDrag = true;

    DOT_TYPE dotType = DOT_TYPE_DOT;

    foleys::ParameterAttachment<float> xAttachment;
    foleys::ParameterAttachment<float> yAttachment;

    juce::RangedAudioParameter* wheelParameter = nullptr;
    juce::RangedAudioParameter* contextMenuParameter = nullptr;

    bool  jumpToClick = false;
    float radius      = 4.0f;
    float lineThickness = 2.0f;
    float senseFactor = 2.0f;

    std::vector<juce::Point<float>> polesToDraw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VesselXYPad)
};

} // namespace vessel
