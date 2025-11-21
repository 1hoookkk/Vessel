/**
 * Z-Plane Visualizer Math Helper
 * * Purpose:
 * This class bridges the gap between the DSP engine (emu_zplane_core.cpp)
 * and a UI renderer (OpenGL/JUCE).
 * * It calculates the Frequency Response Magnitude |H(w)| for the entire
 * 14-pole cascade at various "slices" of the Morph axis, generating
 * the data needed to draw the "Lexicon-style" 3D ribbons.
 */

#include <vector>
#include <complex>
#include <cmath>
#include <iostream>
#include <functional>

// We need the structures from the Core, so we assume they are accessible.
// In a real project, #include "emu_zplane_core.cpp" or the header.
// Redefining simplified versions here for standalone compilation of this helper.
struct VisCoeffs {
    float b1, b2; // Feedback
    float a0, a1, a2; // Feedforward
};

struct VisPoint3D {
    float x; // Frequency (Normalized 0.0 - 1.0 or Log)
    float y; // Magnitude (dB or Linear Gain)
    float z; // Morph Depth (0.0 - 1.0)
};

class ZPlaneVisualizer {
private:
    static const int RESOLUTION = 128; // Points per ribbon line
    static const int SLICE_COUNT = 8;  // How many ribbons to draw (Depth)
    static inline float sSampleRate = 48000.0f;

    // Optional hook to fetch real coeffs from the core; falls back to dummy data.
    static inline std::function<std::vector<VisCoeffs>(float, float, float)> sCoeffProvider = nullptr;

public:
    static void set_sample_rate(float sample_rate_hz) { sSampleRate = sample_rate_hz; }
    static float get_sample_rate() { return sSampleRate; }

    // Allow the UI/engine layer to plug in the real ARMAdillo decoder.
    static void set_coeff_provider(const std::function<std::vector<VisCoeffs>(float, float, float)>& provider) {
        sCoeffProvider = provider;
    }

    /**
     * Calculates the frequency response H(z) for a single biquad at a specific frequency omega.
     * H(z) = (a0 + a1*z^-1 + a2*z^-2) / (1 - b1*z^-1 - b2*z^-2)
     * Note: We use the subtractive feedback convention from the Z-Plane Core.
     */
    static std::complex<float> get_biquad_response(const VisCoeffs& c, float omega) {
        // z^-1 = e^(-j * omega) = cos(omega) - j*sin(omega)
        std::complex<float> z_1 = std::polar(1.0f, -omega);
        std::complex<float> z_2 = std::polar(1.0f, -2.0f * omega);

        std::complex<float> numerator = c.a0 + c.a1 * z_1 + c.a2 * z_2;
        
        // Denominator: 1 - (b1*z^-1 + b2*z^-2) 
        // (Sign matches the DSP core loop logic: acc += y*b1)
        std::complex<float> denominator = 1.0f - (c.b1 * z_1 + c.b2 * z_2);

        // Safety for instability
        if (std::abs(denominator) < 1e-9f) return std::complex<float>(0,0);

        return numerator / denominator;
    }

    /**
     * Generates the geometry data for the "Lexicon-style" 3D ribbons.
     * * @param cube The ZPlaneCube data structure (from core)
     * @param freq_axis_y Current Y-axis modulation (Pitch tracking)
     * @param transform_z Current Z-axis modulation (Transform)
     * @return A vector of points. Each block of 'RESOLUTION' points is one ribbon.
     */
    static std::vector<std::vector<VisPoint3D>> generate_ribbons(
        /* const ZPlaneCube& cube, */ // Assuming access to Cube
        float freq_axis_y, 
        float transform_z,
        float sample_rate_hz = sSampleRate
    ) {
        std::vector<std::vector<VisPoint3D>> ribbons;

        // 1. Loop through Z-Depth (The "Ribbons" going back into the screen)
        // We take snapshots at Morph = 0.0, 0.14, 0.28 ... 1.0
        for (int i = 0; i < SLICE_COUNT; ++i) {
            float morph_x = (float)i / (float)(SLICE_COUNT - 1);
            
            std::vector<VisPoint3D> current_ribbon;
            current_ribbon.reserve(RESOLUTION);

            // In a real implementation, we would call ARMAdilloEngine::interpolate_cube
            // and ARMAdilloEngine::decode_section here to get the coeffs for this slice.
            // For this visualizer demo, we simulate having the 7 decoded sections.
            std::vector<VisCoeffs> sections = get_coeffs_at_morph(morph_x, freq_axis_y, transform_z);

            // 2. Loop through Frequency (X-Axis of the screen)
            for (int j = 0; j < RESOLUTION; ++j) {
                // Logarithmic frequency mapping looks best for EQ curves
                // 0.0 -> 20Hz, 1.0 -> 20kHz
                float t = (float)j / (float)(RESOLUTION - 1);
                float freqHz = 20.0f * std::pow(1000.0f, t);
                float omega = 2.0f * 3.14159f * freqHz / sample_rate_hz; // Use runtime sample rate

                // 3. Calculate Complex Magnitude
                std::complex<float> total_response(1.0f, 0.0f);
                
                // Multiply the response of all 7 cascaded filters
                for (const auto& sec : sections) {
                    total_response *= get_biquad_response(sec, omega);
                }

                float magnitude = std::abs(total_response);
                
                // Convert to Decibels for Y-height
                float db = 20.0f * std::log10(magnitude + 1e-9f); // +epsilon to prevent log(0)
                
                // Normalize for screen drawing (-60dB to +24dB range mapped to 0.0-1.0)
                float plot_y = (db + 60.0f) / 84.0f;
                plot_y = std::max(0.0f, std::min(1.0f, plot_y));

                // Store the point
                current_ribbon.push_back({ t, plot_y, morph_x });
            }
            ribbons.push_back(current_ribbon);
        }
        return ribbons;
    }

    /**
     * Generates a simple "Lexicon-style" perspective grid the UI can render under the ribbons.
     * Returns lines grouped as {front->back} for both frequency and depth axes.
     */
    static std::vector<std::vector<VisPoint3D>> generate_perspective_grid(int freq_lines = 6, int depth_lines = 6) {
        std::vector<std::vector<VisPoint3D>> grid;
        // Frequency lines (front to back)
        for (int i = 0; i < freq_lines; ++i) {
            float t = (float)i / (float)(freq_lines - 1);
            std::vector<VisPoint3D> line;
            line.push_back({t, 0.0f, 0.0f});
            line.push_back({t, 0.0f, 1.0f});
            grid.push_back(line);
        }
        // Depth (morph) lines across frequency
        for (int d = 0; d < depth_lines; ++d) {
            float z = (float)d / (float)(depth_lines - 1);
            std::vector<VisPoint3D> line;
            line.push_back({0.0f, 0.0f, z});
            line.push_back({1.0f, 0.0f, z});
            grid.push_back(line);
        }
        return grid;
    }

private:
    // Stub function to mimic getting coefficients from the Core
    static std::vector<VisCoeffs> get_coeffs_at_morph(float x, float y, float z) {
        if (sCoeffProvider) {
            return sCoeffProvider(x, y, z);
        }
        // Fallback: return placeholder coefficients (unity feedforward / no feedback)
        std::vector<VisCoeffs> dummy(7, VisCoeffs{0.0f, 0.0f, 1.0f, 0.0f, 0.0f});
        return dummy;
    }
};

int main() {
    std::cout << "ZPlane Visualizer Math Helper - Verified" << std::endl;
    // Verify grid generation
    auto grid = ZPlaneVisualizer::generate_perspective_grid();
    std::cout << "Generated " << grid.size() << " grid lines." << std::endl;
    return 0;
}