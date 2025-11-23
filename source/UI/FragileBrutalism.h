/*
  ==============================================================================
    VESSEL // Fragile Brutalism Design System
    Minimal token system: Paper/Ink/Orange aesthetic.
    Zero-cost mechanical stress system for audio-reactive UI drift.
  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <random>

namespace FragileBrutalism
{
    // ═══════════════════════════════════════════════════════════════════════
    // COLOR TOKENS
    // ═══════════════════════════════════════════════════════════════════════
    constexpr juce::uint32 kPaper      = 0xFFF2F2F0;  // Background (paper white)
    constexpr juce::uint32 kInk        = 0xFF0C0C0C;  // Primary (black ink)
    constexpr juce::uint32 kOrange     = 0xFFFF4400;  // Accent (safety orange, clipping only)
    constexpr juce::uint32 kInkFaded   = 0xFF808080;  // Dimmed labels
    constexpr juce::uint32 kVoid       = 0xFF080808;  // Visualizer background (void tear)

    // ═══════════════════════════════════════════════════════════════════════
    // SPACING TOKENS (Grid-based, pixel-perfect)
    // ═══════════════════════════════════════════════════════════════════════
    constexpr int kGridUnit      = 4;
    constexpr int kBorderWidth   = 1;
    constexpr int kPadSmall      = kGridUnit * 2;   // 8px
    constexpr int kPadMedium     = kGridUnit * 4;   // 16px
    constexpr int kPadLarge      = kGridUnit * 6;   // 24px

    // ═══════════════════════════════════════════════════════════════════════
    // TYPOGRAPHY
    // ═══════════════════════════════════════════════════════════════════════
    inline juce::Font getMonoFont(float size, bool bold = false)
    {
        int style = bold ? juce::Font::bold : juce::Font::plain;
        juce::Font mono(juce::FontOptions("Courier New", size, style));
        // Fallback chain
        if (!mono.getTypefaceName().containsIgnoreCase("Courier"))
            mono = juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, style));
        return mono;
    }

    // Force-block typography: distribute chars across width for schematic spacing
    inline void drawForceBlockText(juce::Graphics& g, const juce::String& text,
                                   juce::Rectangle<int> bounds, float size)
    {
        if (text.isEmpty()) return;

        juce::Font f = getMonoFont(size, true);
        g.setFont(f);

        int charCount = text.length();
        if (charCount == 1) {
            g.drawText(text, bounds, juce::Justification::centred);
            return;
        }

        float totalWidth = (float)bounds.getWidth();
        float spacing = totalWidth / (float)(charCount - 1);
        float y = bounds.getCentreY();

        for (int i = 0; i < charCount; ++i) {
            juce::String ch = text.substring(i, i + 1);
            float x = bounds.getX() + (i * spacing);
            g.drawText(ch, (int)x - 10, (int)y - (int)(size/2), 20, (int)size,
                      juce::Justification::centred);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // MECHANICAL RATTLE SYSTEM
    // Audio-reactive Brownian drift with transient snap-back.
    // No allocations, pure state machine.
    // ═══════════════════════════════════════════════════════════════════════
    class MechanicalRattle
    {
    public:
        MechanicalRattle()
        {
            // Seed Brownian RNG with time + address (unique per instance)
            auto seed = (uint32_t)(juce::Time::currentTimeMillis() ^ (uint64_t)this);
            rng.seed(seed);
        }

        // Call from timer with current RMS [0..1] and transient spike [0..1]
        void update(float rms, float transient)
        {
            stress_ = juce::jmin(1.0f, rms * 3.0f); // Scale RMS to stress

            // Brownian motion (not random jitter)
            float brownianStep = normalDist(rng) * 0.3f; // Gaussian
            driftX_ += brownianStep * stress_;
            driftY_ += brownianStep * stress_ * 0.7f; // Vertical less

            // Decay drift over time (spring-back)
            driftX_ *= 0.92f;
            driftY_ *= 0.92f;

            // Transient snap
            if (transient > 0.5f) {
                float snapMag = (transient - 0.5f) * 2.0f;
                driftX_ += normalDist(rng) * snapMag * 2.0f;
                driftY_ += normalDist(rng) * snapMag * 1.5f;
            }

            // Clamp drift
            driftX_ = juce::jlimit(-3.0f, 3.0f, driftX_);
            driftY_ = juce::jlimit(-2.0f, 2.0f, driftY_);
        }

        // Get current offset in pixels
        juce::Point<float> getOffset() const { return {driftX_, driftY_}; }
        float getStress() const { return stress_; }

    private:
        float stress_ = 0.0f;
        float driftX_ = 0.0f;
        float driftY_ = 0.0f;

        std::mt19937 rng;
        std::normal_distribution<float> normalDist{0.0f, 1.0f};
    };

    // ═══════════════════════════════════════════════════════════════════════
    // PHOSPHOR PERSISTENCE HELPER
    // Image buffer approach: draw new frame, overlay previous at 90% opacity.
    // ═══════════════════════════════════════════════════════════════════════
    class PhosphorBuffer
    {
    public:
        PhosphorBuffer() = default;

        void resize(int w, int h)
        {
            if (w == width_ && h == height_) return;
            width_ = w;
            height_ = h;
            buffer_ = juce::Image(juce::Image::ARGB, w, h, true);
        }

        // Draw current frame to buffer with persistence
        void drawWithPersistence(juce::Graphics& targetG,
                                std::function<void(juce::Graphics&)> drawFunc,
                                float persistence = 0.90f)
        {
            if (!buffer_.isValid()) return;

            // Create graphics context for buffer
            juce::Graphics bufferG(buffer_);

            // Decay previous frame
            bufferG.setOpacity(persistence);
            bufferG.drawImageAt(buffer_, 0, 0);

            // Draw new content at full opacity
            bufferG.setOpacity(1.0f);
            drawFunc(bufferG);

            // Composite to target
            targetG.drawImageAt(buffer_, 0, 0);
        }

        void clear()
        {
            if (buffer_.isValid())
                buffer_.clear(buffer_.getBounds(), juce::Colours::transparentBlack);
        }

    private:
        juce::Image buffer_;
        int width_ = 0;
        int height_ = 0;
    };

} // namespace FragileBrutalism
