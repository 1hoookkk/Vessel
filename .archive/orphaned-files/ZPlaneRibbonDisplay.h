/*
  ==============================================================================
    VESSEL // Z-PLANE POLE CONSTELLATION
    Fragile Brutalism: Paper tear reveals void with dancing DSP poles.
    Zero allocations, phosphor trails, mechanical rattle.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/WaitFreeTripleBuffer.h"
#include "SharedData.h"
#include "DSP/EMUAuthenticTables.h"
#include "UI/FragileBrutalism.h"
#include <array>
#include <cmath>

class ZPlaneRibbonDisplay : public juce::Component, public juce::Timer
{
public:
    ZPlaneRibbonDisplay(WaitFreeTripleBuffer<SystemState>& buffer)
        : bufferRef(buffer)
    {
        currentModeIndex = 0;
        trailWriteIndex = 0;
        startTimerHz(30); // 30 FPS sufficient for pole motion
    }

    void timerCallback() override
    {
        if (bufferRef.pull(cachedState))
        {
            // Check if mode changed
            if (cachedState.modeIndex != currentModeIndex)
            {
                currentModeIndex = cachedState.modeIndex;
                clearTrails();
            }

            // Store current pole positions in circular buffer
            storePoleSnapshot();

            // Update mechanical rattle
            float rms = 0.0f;
            for (int i = 0; i < 6; ++i)
                rms += cachedState.leftPoles[i].r + cachedState.rightPoles[i].r;
            rms /= 12.0f;

            float transient = 0.0f; // Could extract from pole velocity
            rattle.update(rms * 0.4f, transient);

            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        using namespace FragileBrutalism;

        const int w = getWidth();
        const int h = getHeight();

        // PAPER BACKGROUND
        g.fillAll(juce::Colour(kPaper));

        // VOID TEAR (visualizer area)
        juce::Rectangle<int> voidArea = getBounds().reduced(kPadMedium);
        g.setColour(juce::Colour(kVoid));
        g.fillRect(voidArea);

        // INK BORDER around void
        g.setColour(juce::Colour(kInk));
        g.drawRect(voidArea, kBorderWidth);

        // PHOSPHOR RENDERING (within void)
        phosphor.resize(voidArea.getWidth(), voidArea.getHeight());

        auto drawScene = [&](juce::Graphics& bufG) {
            bufG.fillAll(juce::Colour(kVoid));

            const float cx = voidArea.getWidth() * 0.5f;
            const float cy = voidArea.getHeight() * 0.5f;
            const float radius = juce::jmin(cx, cy) * 0.85f;

            // Draw graticule
            drawGraticule(bufG, cx, cy, radius);

            // Draw unit circle (stability boundary)
            bufG.setColour(juce::Colour(kInk).withAlpha(0.3f));
            bufG.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

            // Draw pole trails (back to front)
            drawPoleTrails(bufG, cx, cy, radius);
        };

        // Composite with persistence (using scoped clip/origin)
        {
            juce::Graphics::ScopedSaveState savedState(g);
            g.reduceClipRegion(voidArea);
            g.setOrigin(voidArea.getPosition());
            phosphor.drawWithPersistence(g, drawScene, 0.90f);
        }

        // HEADER TEXT (with mechanical rattle on paper)
        juce::Point<float> rattleOffset = rattle.getOffset();

        juce::String modeName = (currentModeIndex >= 0 && currentModeIndex < AUTHENTIC_EMU_NUM_PAIRS)
            ? juce::String(AUTHENTIC_EMU_IDS[currentModeIndex])
            : juce::String("---");
        modeName = modeName.replaceCharacter('_', ' ').replaceCharacter('-', ' ');

        g.setColour(juce::Colour(kInk));
        g.setFont(getMonoFont(12.0f, true));
        juce::Rectangle<int> headerRect(
            kPadMedium + (int)rattleOffset.x,
            kPadSmall + (int)rattleOffset.y,
            w - kPadMedium * 2,
            16
        );
        g.drawText("POLE CONSTELLATION // " + modeName, headerRect, juce::Justification::left);

        // PARAMETER READOUT
        g.setColour(juce::Colour(kInkFaded));
        g.setFont(getMonoFont(8.0f, false));
        juce::String coords = juce::String::formatted(
            "M:%03d F:%03d R:%03d",
            (int)(cachedState.centroidX * 100.0f),
            (int)(cachedState.centroidY * 100.0f),
            (int)(cachedState.transformZ * 100.0f)
        );
        g.drawText(coords, kPadMedium, headerRect.getBottom() + 2, w - kPadMedium * 2, 12,
                  juce::Justification::left);
    }

    void resized() override
    {
        phosphor.clear();
    }

private:
    WaitFreeTripleBuffer<SystemState>& bufferRef;
    SystemState cachedState{};
    int currentModeIndex;

    // CIRCULAR BUFFER FOR TRAILS (pre-allocated, zero allocations)
    static constexpr int TRAIL_CAPACITY = 30;
    struct PoleSnapshot {
        std::array<juce::Point<float>, 6> leftPoles;  // Cartesian (x, y)
        std::array<juce::Point<float>, 6> rightPoles;
    };
    std::array<PoleSnapshot, TRAIL_CAPACITY> trailBuffer;
    int trailWriteIndex;
    int trailCount = 0;

    // Aesthetic systems
    FragileBrutalism::MechanicalRattle rattle;
    FragileBrutalism::PhosphorBuffer phosphor;

    // Polar to Cartesian conversion
    inline juce::Point<float> polarToCartesian(float r, float theta) const
    {
        return {r * std::cos(theta), r * std::sin(theta)};
    }

    void storePoleSnapshot()
    {
        auto& snapshot = trailBuffer[trailWriteIndex];

        // Convert polar poles to cartesian and store
        for (int i = 0; i < 6; ++i)
        {
            snapshot.leftPoles[i] = polarToCartesian(
                cachedState.leftPoles[i].r,
                cachedState.leftPoles[i].theta
            );
            snapshot.rightPoles[i] = polarToCartesian(
                cachedState.rightPoles[i].r,
                cachedState.rightPoles[i].theta
            );
        }

        trailWriteIndex = (trailWriteIndex + 1) % TRAIL_CAPACITY;
        if (trailCount < TRAIL_CAPACITY)
            trailCount++;
    }

    void clearTrails()
    {
        trailWriteIndex = 0;
        trailCount = 0;
    }

    void drawGraticule(juce::Graphics& g, float cx, float cy, float radius)
    {
        using namespace FragileBrutalism;

        g.setColour(juce::Colour(kInk).withAlpha(0.1f));

        // Crosshairs (real/imaginary axes)
        g.drawLine(cx - radius, cy, cx + radius, cy, 1.0f);
        g.drawLine(cx, cy - radius, cx, cy + radius, 1.0f);

        // Inner circle at r=0.5
        g.drawEllipse(cx - radius * 0.5f, cy - radius * 0.5f,
                     radius, radius, 1.0f);

        // Radial lines (every 45°)
        for (int i = 0; i < 8; ++i)
        {
            float angle = i * juce::MathConstants<float>::pi / 4.0f;
            float x = cx + radius * std::cos(angle);
            float y = cy + radius * std::sin(angle);
            g.drawLine(cx, cy, x, y, 1.0f);
        }
    }

    void drawPoleTrails(juce::Graphics& g, float cx, float cy, float radius)
    {
        using namespace FragileBrutalism;

        if (trailCount == 0) return;

        // Draw oldest to newest
        for (int t = trailCount - 1; t >= 0; --t)
        {
            int bufferIndex = (trailWriteIndex - 1 - t + TRAIL_CAPACITY) % TRAIL_CAPACITY;
            const auto& snapshot = trailBuffer[bufferIndex];

            float age = (float)t / (float)TRAIL_CAPACITY;
            float opacity = 1.0f - (age * age); // Quadratic decay

            if (opacity < 0.05f) continue;

            // Draw right channel (orange faded)
            g.setColour(juce::Colour(kOrange).withAlpha(opacity * 0.4f));
            for (int i = 0; i < 6; ++i)
            {
                const auto& pole = snapshot.rightPoles[i];
                float x = cx + pole.x * radius;
                float y = cy - pole.y * radius; // Invert Y

                float size = (t == 0) ? 4.0f : 2.0f;
                g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
            }

            // Draw left channel (ink)
            g.setColour(juce::Colour(kInk).withAlpha(opacity));
            for (int i = 0; i < 6; ++i)
            {
                const auto& pole = snapshot.leftPoles[i];
                float x = cx + pole.x * radius;
                float y = cy - pole.y * radius; // Invert Y

                float size = (t == 0) ? 5.0f : 2.5f;

                // Current position: crosshair marker
                if (t == 0)
                {
                    g.drawLine(x - size, y, x + size, y, 1.5f);
                    g.drawLine(x, y - size, x, y + size, 1.5f);

                    // Highlight if near unit circle (resonance warning)
                    float r = std::sqrt(pole.x * pole.x + pole.y * pole.y);
                    if (r > 0.92f)
                    {
                        g.setColour(juce::Colour(kOrange).withAlpha(opacity));
                        g.drawEllipse(x - size * 1.5f, y - size * 1.5f,
                                     size * 3.0f, size * 3.0f, 1.0f);
                        g.setColour(juce::Colour(kInk).withAlpha(opacity));
                    }
                }
                else
                {
                    g.fillEllipse(x - size * 0.5f, y - size * 0.5f, size, size);
                }
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZPlaneRibbonDisplay)
};
