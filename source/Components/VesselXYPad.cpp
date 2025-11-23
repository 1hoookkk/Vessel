/*
 ==============================================================================
    VESSEL - Authentic E-mu Z-Plane Emulation
    VesselXYPad.cpp
 ==============================================================================
 */

#include "VesselXYPad.h"
#include "../VesselStyle.h"

namespace vessel
{

VesselXYPad::VesselXYPad()
{
    setOpaque (false);

    setColour (xyDotColourId, Vessel::accent.darker(0.2f));
    setColour (xyDotOverColourId, Vessel::accent);
    setColour (xyHorizontalColourId, Vessel::inset);
    setColour (xyHorizontalOverColourId, Vessel::accent.withAlpha(0.8f));
    setColour (xyVerticalColourId, Vessel::inset);
    setColour (xyVerticalOverColourId, Vessel::accent.withAlpha(0.8f));

    xAttachment.onParameterChangedAsync = [&] { repaint(); };
    yAttachment.onParameterChangedAsync = [&] { repaint(); };
}

void VesselXYPad::setCrossHair (bool horizontal, bool vertical)
{
    wantsVerticalDrag   = horizontal;
    wantsHorizontalDrag = vertical;
}

void VesselXYPad::setDotType (DOT_TYPE dotTypeToDraw)
{
    dotType = dotTypeToDraw;
    repaint();
}

void VesselXYPad::paint (juce::Graphics& g)
{
    // 1. Draw Background Visualization (Glass Cube)
    // We manually invoke the component's paint method to layer it without child clipping issues
    glassCube.setBounds(getLocalBounds());
    glassCube.paint(g);

    const auto x   = getXposition();
    const auto y   = getYposition();
    const auto gap = radius * 1.8f;

    // Draw Unit Circle Reference (Subtle Overlay)
    const float cx = getWidth() * 0.5f;
    const float cy = getHeight() * 0.5f;
    const float displayRadius = std::min(getWidth(), getHeight()) * 0.45f;

    g.setColour(Vessel::inset.withAlpha(0.1f));
    g.drawEllipse(cx - displayRadius, cy - displayRadius, displayRadius * 2.0f, displayRadius * 2.0f, 1.0f);

    // Draw Poles
    for (const auto& pole : polesToDraw)
    {
        // Map unit circle [-1, 1] to screen
        float px = cx + pole.x * displayRadius;
        float py = cy - pole.y * displayRadius;
        
        float poleSize = radius * 1.2f;
        
        // Pole Glow
        g.setColour(Vessel::accent.withAlpha(0.2f));
        g.fillEllipse(px - poleSize * 1.5f, py - poleSize * 1.5f, poleSize * 3.0f, poleSize * 3.0f);
        
        // Pole Core
        g.setColour(Vessel::accent.withAlpha(0.9f));
        g.drawEllipse(px - poleSize * 0.5f, py - poleSize * 0.5f, poleSize, poleSize, 1.5f); 
        g.drawLine(px - poleSize*0.4f, py - poleSize*0.4f, px + poleSize*0.4f, py + poleSize*0.4f, 1.0f); // X
        g.drawLine(px - poleSize*0.4f, py + poleSize*0.4f, px + poleSize*0.4f, py - poleSize*0.4f, 1.0f);
    }

    // Draw Crosshairs (Drag Guides)
    if (wantsVerticalDrag)
    {
        g.setColour (findColour (mouseOverY ? xyVerticalOverColourId : xyVerticalColourId));
        if (x > gap)
            g.fillRect (0.0f, y - 1.0f, x - gap, 2.0f);

        if (x < getRight() - gap)
            g.fillRect (x + gap, y - 1.0f, getRight() - (x + gap), 2.0f);
    }

    if (wantsHorizontalDrag)
    {
        g.setColour (findColour (mouseOverX ? xyHorizontalOverColourId : xyHorizontalColourId));
        if (y > gap)
            g.fillRect (x - 1.0f, 0.0f, 2.0f, y - gap);

        if (y < getBottom() - gap)
            g.fillRect (x - 1.0f, y + gap, 2.0f, getBottom() - (y + gap));
    }

    // Draw Control Point (The "Touch" point)
    g.setColour (findColour (mouseOverDot ? xyDotOverColourId : xyDotColourId));
    
    // Draw fancy cursor
    g.drawEllipse(x - radius*1.5f, y - radius*1.5f, radius*3.0f, radius*3.0f, 2.0f);
    g.fillEllipse(x - radius*0.5f, y - radius*0.5f, radius, radius);
}

void VesselXYPad::setParameterX (juce::RangedAudioParameter* parameter)
{
    xAttachment.attachToParameter (parameter);
}

void VesselXYPad::setLineThickness (float thickness)
{
    lineThickness = thickness;
    repaint();
}

void VesselXYPad::setParameterY (juce::RangedAudioParameter* parameter)
{
    yAttachment.attachToParameter (parameter);
}

void VesselXYPad::setWheelParameter (juce::RangedAudioParameter* parameter)
{
    wheelParameter = parameter;
}

void VesselXYPad::setRightClickParameter (juce::RangedAudioParameter* parameter)
{
    contextMenuParameter = parameter;
}

void VesselXYPad::setRadius (float radiusToUse)
{
    radius = radiusToUse;
    repaint();
}

void VesselXYPad::setSenseFactor (float factor)
{
    senseFactor = factor;
}

void VesselXYPad::setJumpToClick (bool shouldJumpToClick)
{
    jumpToClick = shouldJumpToClick;
}

void VesselXYPad::setPoles(const std::vector<juce::Point<float>>& poles)
{
    polesToDraw = poles;
    repaint();
}

void VesselXYPad::updateGlassState(float morph, float freq, float trans, int morphPair)
{
    glassCube.updateState(morph, freq, trans, morphPair);
    repaint();
}

void VesselXYPad::resized()
{
    glassCube.setBounds(getLocalBounds());
}

void VesselXYPad::updateWhichToDrag (juce::Point<float> pos)
{
    const auto centre = juce::Point<int> (getXposition(), getYposition()).toFloat();

    mouseOverDot = (centre.getDistanceFrom (pos) < radius * senseFactor);
    mouseOverX   = (wantsHorizontalDrag && std::abs (pos.getX() - centre.getX()) < senseFactor + 1.0f);
    mouseOverY   = (wantsVerticalDrag && std::abs (pos.getY() - centre.getY()) < senseFactor + 1.0f);

    repaint();
}

bool VesselXYPad::hitTest (int x, int y)
{
    if (jumpToClick)
        return true;

    const auto click  = juce::Point<int> (x, y).toFloat();
    const auto centre = juce::Point<int> (getXposition(), getYposition()).toFloat();

    if (centre.getDistanceFrom (click) < radius * senseFactor)
        return true;

    if (wantsHorizontalDrag && std::abs (click.getX() - centre.getX()) < senseFactor + 1.0f)
        return true;

    if (wantsVerticalDrag && std::abs (click.getY() - centre.getY()) < senseFactor + 1.0f)
        return true;

    return false;
}

void VesselXYPad::mouseDown (const juce::MouseEvent& event)
{
    if (contextMenuParameter && (event.mods.isPopupMenu()))
    {
        juce::PopupMenu menu;
        int             id      = 0;
        auto            current = contextMenuParameter->getCurrentValueAsText();

        for (const auto& item: contextMenuParameter->getAllValueStrings())
            menu.addItem (++id, item, true, item == current);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea ({ event.getScreenX(), event.getScreenY(), 1, 1 }),
                            [cmp = contextMenuParameter] (int selected)
                            {
                                if (selected <= 0)
                                    return;

                                const auto& range = cmp->getNormalisableRange();
                                auto        value = range.start + (static_cast<float> (selected - 1)) * range.interval;
                                cmp->beginChangeGesture();
                                cmp->setValueNotifyingHost (cmp->convertTo0to1 (value));
                                cmp->endChangeGesture();
                            });

        return;
    }

    if (jumpToClick)
    {
        mouseOverX   = true;
        mouseOverY   = true;
        mouseOverDot = true;

        xAttachment.beginGesture();
        xAttachment.setNormalisedValue (event.position.getX() / float (getWidth()));

        yAttachment.beginGesture();
        yAttachment.setNormalisedValue (1.0f - event.position.getY() / float (getHeight()));

        repaint();
        return;
    }

    updateWhichToDrag (event.position);

    if (mouseOverX || mouseOverDot)
        xAttachment.beginGesture();

    if (mouseOverY || mouseOverDot)
        yAttachment.beginGesture();
}

void VesselXYPad::mouseMove (const juce::MouseEvent& event)
{
    updateWhichToDrag (event.position);
}

void VesselXYPad::mouseDrag (const juce::MouseEvent& event)
{
    if (mouseOverX || mouseOverDot)
        xAttachment.setNormalisedValue (event.position.getX() / float (getWidth()));

    if (mouseOverY || mouseOverDot)
        yAttachment.setNormalisedValue (1.0f - event.position.getY() / float (getHeight()));
}

void VesselXYPad::mouseUp (const juce::MouseEvent& event)
{
    if (contextMenuParameter && (event.mods.isPopupMenu()))
        return;

    if (mouseOverX || mouseOverDot)
        xAttachment.endGesture();

    if (mouseOverY || mouseOverDot)
        yAttachment.endGesture();
}

void VesselXYPad::mouseWheelMove (const juce::MouseEvent& event, const juce::MouseWheelDetails& details)
{
    updateWhichToDrag (event.position);

    if (mouseOverDot && wheelParameter)
    {
        wheelParameter->beginChangeGesture();
        auto range     = wheelParameter->getNormalisableRange();
        auto lastValue = range.convertFrom0to1 (wheelParameter->getValue());
        auto interval  = details.deltaY > 0.0f ? range.interval : -range.interval;
        wheelParameter->setValueNotifyingHost (range.convertTo0to1 (juce::jlimit (range.start, range.end, lastValue + interval)));
        wheelParameter->endChangeGesture();
        return;
    }

    juce::Component::mouseWheelMove (event, details);
}

void VesselXYPad::mouseEnter (const juce::MouseEvent& event)
{
    updateWhichToDrag (event.position);
}

void VesselXYPad::mouseExit (const juce::MouseEvent&)
{
    mouseOverDot = false;
    mouseOverX   = false;
    mouseOverY   = false;

    repaint();
}

int VesselXYPad::getXposition() const
{
    return juce::roundToInt (xAttachment.getNormalisedValue() * getWidth());
}

int VesselXYPad::getYposition() const
{
    return juce::roundToInt ((1.0f - yAttachment.getNormalisedValue()) * getHeight());
}

} // namespace vessel
