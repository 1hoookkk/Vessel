/**
 * ZPlaneDisplay.h
 * Z-Plane Visualizer Component
 *
 * This component renders the "Lexicon-style" 3D frequency response ribbons
 * using the "Fragile Brutalism" aesthetic:
 *   - High contrast monochromatic (terminal green on deep black)
 *   - 1px thin lines
 *   - Ghostly depth effect with low opacity
 *   - Technical/haunted hardware appearance
 *
 * Performance:
 *   - Geometry calculation runs on a Timer (message thread)
 *   - NOT called from audio thread
 *   - Uses lock-free FIFO to receive parameter updates from processor
 */

#pragma once

#include <JuceHeader.h>
#include "../Structure/DSP/ZPlaneCore.h"
#include "PluginProcessor.h"
#include <vector>
#include <complex>
#include <cmath>

//==============================================================================
// 3D Point for Visualization
//==============================================================================

struct VisPoint3D {
    float x;  // Frequency (Normalized 0.0 - 1.0)
    float y;  // Magnitude (dB or Linear, normalized)
    float z;  // Morph Depth (0.0 - 1.0)

    VisPoint3D() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
    VisPoint3D(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
};

//==============================================================================
// Z-Plane Ribbon Geometry Generator
//==============================================================================

class ZPlaneRibbonGeometry
{
public:
    static constexpr int RESOLUTION = 128;      // Points per ribbon line
    static constexpr int SLICE_COUNT = 8;       // How many ribbons (depth)

    /**
     * Calculates the frequency response H(z) for a single biquad at a specific frequency omega.
     * H(z) = (a0 + a1*z^-1 + a2*z^-2) / (1 - b1*z^-1 - b2*z^-2)
     */
    static std::complex<float> getBiquadResponse(const vessel::dsp::BiquadCoeffs& c,
                                                 float omega) noexcept
    {
        // Convert fixed-point coefficients to float for visualization
        float a0 = vessel::dsp::fixed_to_float(c.a0);
        float a1 = vessel::dsp::fixed_to_float(c.a1);
        float a2 = vessel::dsp::fixed_to_float(c.a2);
        float b1 = vessel::dsp::fixed_to_float(c.b1);
        float b2 = vessel::dsp::fixed_to_float(c.b2);

        // z^-1 = e^(-j*omega) = cos(omega) - j*sin(omega)
        std::complex<float> z_1(std::cos(omega), -std::sin(omega));
        std::complex<float> z_2(std::cos(2.0f * omega), -std::sin(2.0f * omega));

        std::complex<float> numerator = a0 + a1 * z_1 + a2 * z_2;

        // Denominator: 1 - (b1*z^-1 + b2*z^-2)
        std::complex<float> denominator(1.0f, 0.0f);
        denominator -= b1 * z_1;
        denominator -= b2 * z_2;

        // Safety for instability
        if (std::abs(denominator) < 1e-9f)
            return std::complex<float>(0.0f, 0.0f);

        return numerator / denominator;
    }

    /**
     * Generates the geometry data for the "Lexicon-style" 3D ribbons.
     *
     * @param cube The ZPlaneCube data structure
     * @param freqAxis Current Y-axis modulation (Pitch tracking)
     * @param transformZ Current Z-axis modulation (Transform)
     * @param sampleRate Current sample rate for frequency calculations
     * @return Vector of ribbons, each ribbon is a vector of 3D points
     */
    static std::vector<std::vector<VisPoint3D>> generateRibbons(
        const vessel::dsp::ZPlaneCube& cube,
        float freqAxis,
        float transformZ,
        float sampleRate = 48000.0f)
    {
        std::vector<std::vector<VisPoint3D>> ribbons;
        ribbons.reserve(SLICE_COUNT);

        // Loop through Z-Depth (The "Ribbons" going back into the screen)
        for (int i = 0; i < SLICE_COUNT; ++i)
        {
            float morphX = static_cast<float>(i) / static_cast<float>(SLICE_COUNT - 1);

            std::vector<VisPoint3D> currentRibbon;
            currentRibbon.reserve(RESOLUTION);

            // Interpolate the cube at this morph position
            vessel::dsp::ZPlaneFrame frame =
                vessel::dsp::ARMAdilloEngine::interpolate_cube(cube, morphX, freqAxis, transformZ);

            // Decode to coefficients
            std::array<vessel::dsp::BiquadCoeffs, 7> coeffs;
            for (int s = 0; s < 7; ++s)
            {
                coeffs[s] = vessel::dsp::ARMAdilloEngine::decode_section(
                    frame.sections[s], sampleRate);
            }

            // Loop through Frequency (X-Axis)
            for (int j = 0; j < RESOLUTION; ++j)
            {
                // Logarithmic frequency mapping (20Hz - 20kHz)
                float t = static_cast<float>(j) / static_cast<float>(RESOLUTION - 1);
                float freqHz = 20.0f * std::pow(1000.0f, t);
                float omega = 2.0f * 3.14159265358979323846f * freqHz / sampleRate;

                // Calculate Complex Magnitude (cascade all 7 filters)
                std::complex<float> totalResponse(1.0f, 0.0f);

                for (const auto& coeff : coeffs)
                {
                    totalResponse *= getBiquadResponse(coeff, omega);
                }

                float magnitude = std::abs(totalResponse);

                // Convert to Decibels for Y-height
                float db = 20.0f * std::log10(magnitude + 1e-9f);

                // Normalize for screen drawing (-60dB to +24dB range -> 0.0-1.0)
                float plotY = (db + 60.0f) / 84.0f;
                plotY = std::max(0.0f, std::min(1.0f, plotY));

                // Store the point
                currentRibbon.emplace_back(t, plotY, morphX);
            }

            ribbons.push_back(std::move(currentRibbon));
        }

        return ribbons;
    }
};

//==============================================================================
// Z-Plane Display Component
//==============================================================================

class ZPlaneDisplay : public juce::Component,
                      private juce::Timer
{
public:
    explicit ZPlaneDisplay(VesselAudioProcessor& proc)
        : processor(proc)
    {
        // Start timer for visualization updates (30 FPS)
        startTimerHz(30);

        // Initialize with default cube
        currentCube = vessel::dsp::ZPlaneCubeFactory::createDemoCube();
    }

    ~ZPlaneDisplay() override
    {
        stopTimer();
    }

    void paint(juce::Graphics& g) override
    {
        // Background: Deep black
        g.fillAll(juce::Colour(0xff000000));

        // Get bounds for drawing
        auto bounds = getLocalBounds().toFloat();
        float width = bounds.getWidth();
        float height = bounds.getHeight();

        // Draw ribbons from back to front for proper depth
        for (int ribbonIdx = 0; ribbonIdx < static_cast<int>(cachedRibbons.size()); ++ribbonIdx)
        {
            const auto& ribbon = cachedRibbons[ribbonIdx];

            if (ribbon.empty())
                continue;

            // Calculate opacity: Back ribbons are more transparent
            float depthAlpha = 0.15f + (0.85f * static_cast<float>(ribbonIdx) /
                                       static_cast<float>(cachedRibbons.size() - 1));

            // Terminal green with depth-based alpha
            juce::Colour ribbonColour = juce::Colour(0xff00ff41).withAlpha(depthAlpha);

            // Create path for this ribbon
            juce::Path ribbonPath;

            bool firstPoint = true;
            for (const auto& point : ribbon)
            {
                // Map normalized coordinates to screen space
                float screenX = bounds.getX() + point.x * width;
                float screenY = bounds.getBottom() - point.y * height; // Flip Y

                if (firstPoint)
                {
                    ribbonPath.startNewSubPath(screenX, screenY);
                    firstPoint = false;
                }
                else
                {
                    ribbonPath.lineTo(screenX, screenY);
                }
            }

            // Draw the ribbon with 1px stroke (Fragile Brutalism)
            g.setColour(ribbonColour);
            g.strokePath(ribbonPath, juce::PathStrokeType(1.0f));
        }

        // Draw frequency grid (subtle)
        drawFrequencyGrid(g, bounds);
    }

    void resized() override
    {
        // Regenerate ribbons on resize
        regenerateRibbons();
    }

private:
    VesselAudioProcessor& processor;

    // Cached ribbon geometry
    std::vector<std::vector<VisPoint3D>> cachedRibbons;

    // Current parameter state
    vessel::dsp::ZPlaneCube currentCube;
    float currentFreq = 0.5f;
    float currentTrans = 0.5f;

    void timerCallback() override
    {
        // Check for new visualization data from audio thread
        VisualizationSnapshot snapshot;
        if (processor.getVisualizationSnapshot(snapshot))
        {
            currentFreq = snapshot.freq;
            currentTrans = snapshot.trans;

            // Regenerate ribbons with new parameters
            regenerateRibbons();
        }

        // Repaint the component
        repaint();
    }

    void regenerateRibbons()
    {
        // This runs on the message thread, NOT the audio thread
        cachedRibbons = ZPlaneRibbonGeometry::generateRibbons(
            currentCube,
            currentFreq,
            currentTrans,
            static_cast<float>(processor.getSampleRate())
        );
    }

    void drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        // Draw subtle frequency markers (20Hz, 200Hz, 2kHz, 20kHz)
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.1f));

        const float frequencies[] = { 20.0f, 200.0f, 2000.0f, 20000.0f };
        const char* labels[] = { "20", "200", "2k", "20k" };

        for (int i = 0; i < 4; ++i)
        {
            // Calculate normalized position (log scale)
            float t = std::log10(frequencies[i] / 20.0f) / std::log10(1000.0f);
            float x = bounds.getX() + t * bounds.getWidth();

            // Draw vertical line
            g.drawVerticalLine(static_cast<int>(x), bounds.getY(), bounds.getBottom());

            // Draw label (if there's space)
            g.drawText(labels[i],
                      static_cast<int>(x) - 20, static_cast<int>(bounds.getBottom()) - 20,
                      40, 20,
                      juce::Justification::centred);
        }

        // Draw 0dB line
        float zeroDbY = bounds.getBottom() - (60.0f / 84.0f) * bounds.getHeight();
        g.setColour(juce::Colour(0xff00ff41).withAlpha(0.2f));
        g.drawHorizontalLine(static_cast<int>(zeroDbY), bounds.getX(), bounds.getRight());
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZPlaneDisplay)
};
