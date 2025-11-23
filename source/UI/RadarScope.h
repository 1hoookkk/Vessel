/*
  ==============================================================================
    HEAVY - RadarScope
    Green phosphor Z-Plane pole constellation display.
    Military radar aesthetic with phosphor persistence trails.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "../HeavyConstants.h"
#include "../SharedData.h"
#include "../Utility/WaitFreeTripleBuffer.h"

class RadarScope : public juce::Component, private juce::Timer
{
public:
    RadarScope(WaitFreeTripleBuffer<SystemState>& buffer)
        : tripleBuffer_(buffer)
    {
        startTimerHz(30); // 30 FPS radar refresh
    }

    void resized() override
    {
        if (getLocalBounds().isEmpty()) return;

        // Reallocate phosphor buffer
        int w = getWidth();
        int h = getHeight();
        if (w > 0 && h > 0)
        {
            phosphorBuffer_ = juce::Image(juce::Image::ARGB, w, h, true);
            phosphorBuffer_.clear(phosphorBuffer_.getBounds());
        }

        // Rebuild static geometry
        rebuildStaticGeometry();
    }

    void paint(juce::Graphics& g) override
    {
        using namespace Heavy::Palette;

        // 1. Vantablack background
        g.fillAll(Recess);

        // 2. Static radar grid
        g.setColour(PhosphorOff);
        g.strokePath(staticGridPath_, juce::PathStrokeType(1.0f));

        // 3. Unit circle (stability boundary)
        g.setColour(PhosphorOff);
        g.strokePath(unitCirclePath_, juce::PathStrokeType(1.5f));

        // 4. Phosphor trail layer
        if (phosphorBuffer_.isValid())
        {
            g.drawImageAt(phosphorBuffer_, 0, 0);
        }

        // 5. CLIPPING FLASH (screen inverts on overload)
        if (cachedState_.outputPeak > 0.99f)
        {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.fillAll();
        }

        // 6. Material name overlay
        g.setColour(Text.withAlpha(0.6f));
        g.setFont(Heavy::getIndustrialFont(10.0f, false));
        juce::String matName = Heavy::getMaterialName(currentMaterial_);
        g.drawText(matName, getLocalBounds().removeFromTop(20), juce::Justification::centred);
    }

private:
    WaitFreeTripleBuffer<SystemState>& tripleBuffer_;
    SystemState cachedState_{};
    Heavy::MaterialType currentMaterial_ = Heavy::MaterialType::STEEL;

    // Cached geometry (built in resized(), zero allocations in paint)
    juce::Path staticGridPath_;
    juce::Path unitCirclePath_;
    juce::Image phosphorBuffer_;

    void timerCallback() override
    {
        if (!isVisible()) return;

        // Pull latest state from audio thread
        if (tripleBuffer_.pull(cachedState_))
        {
            currentMaterial_ = static_cast<Heavy::MaterialType>(
                juce::jlimit(0, 7, cachedState_.modeIndex)
            );

            updatePhosphorLayer();
            repaint();
        }
    }

    void rebuildStaticGeometry()
    {
        staticGridPath_.clear();
        unitCirclePath_.clear();

        auto bounds = getLocalBounds().toFloat();
        auto center = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;

        // Unit circle (r = 1.0, stability boundary)
        unitCirclePath_.addEllipse(center.x - radius, center.y - radius, radius * 2.0f, radius * 2.0f);

        // Crosshairs (real/imaginary axes)
        staticGridPath_.startNewSubPath(center.x - radius, center.y);
        staticGridPath_.lineTo(center.x + radius, center.y);
        staticGridPath_.startNewSubPath(center.x, center.y - radius);
        staticGridPath_.lineTo(center.x, center.y + radius);

        // Inner circle (r = 0.5)
        staticGridPath_.addEllipse(center.x - radius * 0.5f, center.y - radius * 0.5f,
                                   radius, radius);

        // Radial lines (every 45°)
        for (int i = 1; i < 8; i += 2) // Skip cardinal directions (already have crosshairs)
        {
            float angle = i * juce::MathConstants<float>::pi / 4.0f;
            float x = center.x + radius * std::cos(angle);
            float y = center.y + radius * std::sin(angle);
            staticGridPath_.startNewSubPath(center.x, center.y);
            staticGridPath_.lineTo(x, y);
        }
    }

    void updatePhosphorLayer()
    {
        if (!phosphorBuffer_.isValid()) return;

        juce::Graphics bufG(phosphorBuffer_);

        // 1. Decay previous frame (phosphor persistence)
        bufG.setOpacity(0.88f); // 88% retention
        bufG.drawImageAt(phosphorBuffer_, 0, 0);

        // 2. Draw new pole positions
        bufG.setOpacity(1.0f);

        auto bounds = getLocalBounds().toFloat();
        auto center = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.42f;

        // REACTIVE SCALING: Poles pulse with audio energy
        float rmsScale = 1.0f + (cachedState_.outputRMS * 3.0f); // Scale up to 4x at max
        rmsScale = juce::jlimit(1.0f, 2.5f, rmsScale); // Clamp to prevent excessive pulsing

        // Convert polar poles to cartesian and render
        auto drawPoleSet = [&](const std::array<zplane::Pole, 7>& poles, juce::Colour color)
        {
            for (const auto& pole : poles)
            {
                // Polar to cartesian conversion with reactive scaling
                float scaledRadius = pole.r * radius * rmsScale;
                float x = center.x + (std::cos(pole.theta) * scaledRadius);
                float y = center.y - (std::sin(pole.theta) * scaledRadius); // Invert Y

                // Draw crosshair marker (size also scales with RMS)
                float size = 4.0f * (0.8f + cachedState_.outputRMS * 2.0f);
                size = juce::jlimit(3.0f, 8.0f, size);

                bufG.setColour(color);
                bufG.drawLine(x - size, y, x + size, y, 1.5f);
                bufG.drawLine(x, y - size, x, y + size, 1.5f);

                // Warning glow if near unit circle (unstable)
                if (pole.r > 0.92f)
                {
                    bufG.setColour(Heavy::Palette::Warning.withAlpha(0.6f));
                    bufG.drawEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f, 1.0f);
                }
            }
        };

        // Draw left channel (green)
        drawPoleSet(cachedState_.leftPoles, Heavy::Palette::PhosphorOn);

        // Draw right channel (dimmer green)
        drawPoleSet(cachedState_.rightPoles, Heavy::Palette::PhosphorOn.withAlpha(0.6f));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RadarScope)
};
