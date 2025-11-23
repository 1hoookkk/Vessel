/*
  ==============================================================================
    HEAVY - Pressure Bar
    Tactile trigger with LED decay visualization.
    Carbon fiber aesthetic with position-based velocity.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../HeavyConstants.h"

// Forward declaration
class HeavyProcessor;

class HeavyBar : public juce::Component, private juce::Timer
{
public:
    HeavyBar(HeavyProcessor& processor)
        : processor_(processor)
    {
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        isPressed_ = true;

        // Position-based velocity: left = soft (0.2), right = hard (1.0)
        float normalizedX = (float)e.getPosition().x / (float)getWidth();
        velocity_ = juce::jlimit(0.2f, 1.0f, normalizedX);

        // Trigger note on processor
        processor_.triggerNote(velocity_);

        // Start decay visualization
        decayLevel_ = 1.0f;
        startTimerHz(60); // Smooth 60 FPS animation

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        isPressed_ = false;
        processor_.releaseNote();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace Heavy::Palette;

        auto bounds = getLocalBounds().toFloat();

        // 1. Base structure (gunmetal chassis)
        g.setColour(juce::Colour(0xFF1A1A1A));
        g.fillRect(bounds);

        // 2. Carbon fiber texture (diagonal weave pattern)
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        for (float i = -bounds.getHeight(); i < bounds.getWidth(); i += 8.0f)
        {
            g.drawLine(i, 0, i + bounds.getHeight(), bounds.getHeight(), 2.0f);
        }
        for (float i = 0; i < bounds.getWidth() + bounds.getHeight(); i += 8.0f)
        {
            g.drawLine(i, 0, i - bounds.getHeight(), bounds.getHeight(), 2.0f);
        }

        // 3. Pressed state (depth effect)
        if (isPressed_)
        {
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.fillRect(bounds);
        }
        else
        {
            // Chamfer highlight (top edge)
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawLine(0, 0, bounds.getWidth(), 0, 2.0f);
        }

        // 4. LED decay strip (recessed groove)
        float stripH = 8.0f;
        auto stripRect = bounds.reduced(20.0f, (bounds.getHeight() - stripH) / 2.0f);

        // Dark slot background
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(stripRect, 4.0f);

        // Active decay visualization
        if (decayLevel_ > 0.01f)
        {
            // Width shrinks as envelope decays
            auto activeRect = stripRect;
            activeRect.setWidth(stripRect.getWidth() * decayLevel_);

            // Color transition: White → Orange → Red
            juce::Colour glowColor;
            if (decayLevel_ > 0.8f)
                glowColor = juce::Colours::white;
            else if (decayLevel_ > 0.3f)
                glowColor = Warning; // Orange
            else
                glowColor = juce::Colour(0xFF8B0000); // Dark red

            // Outer glow
            g.setColour(glowColor.withAlpha(0.4f));
            g.fillRoundedRectangle(activeRect.expanded(2.0f), 5.0f);

            // Core light
            g.setColour(glowColor);
            g.fillRoundedRectangle(activeRect, 3.0f);
        }

        // 5. Label text
        g.setColour(Text.withAlpha(0.4f));
        g.setFont(Heavy::getIndustrialFont(10.0f, false));
        g.drawText("STRIKE", bounds, juce::Justification::centred);
    }

private:
    HeavyProcessor& processor_;
    bool isPressed_ = false;
    float velocity_ = 0.8f;
    float decayLevel_ = 0.0f;

    void timerCallback() override
    {
        // Exponential decay simulation
        decayLevel_ *= 0.92f; // ~8% decay per frame at 60 FPS

        if (decayLevel_ < 0.001f)
        {
            stopTimer();
            decayLevel_ = 0.0f;
        }

        repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeavyBar)
};
