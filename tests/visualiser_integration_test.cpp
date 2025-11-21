/**
 * Phase 3.1 Integration Test (Prototype)
 * Verifies the wiring between ZPlaneVisualizer (Visualiser.md) and EmuZPlaneCore
 */

#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <functional>
#include <iomanip>

// Include the Core DSP (Phase 1.1)
#include "../source/DSP/EmuZPlaneCore.h"
// (In a real build, we'd link against EmuZPlaneCore.cpp, here we assume it's linked)

// =========================================================
// ZPlaneVisualizer (Ported from visualiser.md for testing)
// =========================================================

struct VisCoeffs {
    float b1, b2;
    float a0, a1, a2;
};

struct VisPoint3D {
    float x, y, z;
};

class ZPlaneVisualizer {
private:
    static const int RESOLUTION = 128;
    static const int SLICE_COUNT = 8;
    static inline float sSampleRate = 48000.0f;
    static inline std::function<std::vector<VisCoeffs>(float, float, float)> sCoeffProvider = nullptr;

public:
    static void set_sample_rate(float sample_rate_hz) { sSampleRate = sample_rate_hz; }
    static float get_sample_rate() { return sSampleRate; }

    static void set_coeff_provider(const std::function<std::vector<VisCoeffs>(float, float, float)>& provider) {
        sCoeffProvider = provider;
    }

    static std::complex<float> get_biquad_response(const VisCoeffs& c, float omega) {
        std::complex<float> z_1 = std::polar(1.0f, -omega);
        std::complex<float> z_2 = std::polar(1.0f, -2.0f * omega);
        std::complex<float> num = c.a0 + c.a1 * z_1 + c.a2 * z_2;
        std::complex<float> den = 1.0f - (c.b1 * z_1 + c.b2 * z_2);
        if (std::abs(den) < 1e-9f) return {0,0};
        return num / den;
    }

    static std::vector<std::vector<VisPoint3D>> generate_ribbons(
        float freq_axis_y, 
        float transform_z,
        float sample_rate_hz = sSampleRate
    ) {
        std::vector<std::vector<VisPoint3D>> ribbons;
        for (int i = 0; i < SLICE_COUNT; ++i) {
            float morph_x = (float)i / (float)(SLICE_COUNT - 1);
            std::vector<VisPoint3D> current_ribbon;
            current_ribbon.reserve(RESOLUTION);

            std::vector<VisCoeffs> sections;
            if (sCoeffProvider) {
                sections = sCoeffProvider(morph_x, freq_axis_y, transform_z);
            } else {
                sections = std::vector<VisCoeffs>(7, {0,0,1,0,0});
            }

            for (int j = 0; j < RESOLUTION; ++j) {
                float t = (float)j / (float)(RESOLUTION - 1);
                float freqHz = 20.0f * std::pow(1000.0f, t);
                float omega = 2.0f * 3.14159f * freqHz / sample_rate_hz;

                std::complex<float> total_response(1.0f, 0.0f);
                for (const auto& sec : sections) {
                    total_response *= get_biquad_response(sec, omega);
                }

                float magnitude = std::abs(total_response);
                float db = 20.0f * std::log10(magnitude + 1e-9f);
                float plot_y = (db + 60.0f) / 84.0f;
                plot_y = std::max(0.0f, std::min(1.0f, plot_y));

                current_ribbon.push_back({ t, plot_y, morph_x });
            }
            ribbons.push_back(current_ribbon);
        }
        return ribbons;
    }
    
    static std::vector<std::vector<VisPoint3D>> generate_perspective_grid(int freq_lines = 6, int depth_lines = 6) {
        std::vector<std::vector<VisPoint3D>> grid;
        for (int i = 0; i < freq_lines; ++i) {
            float t = (float)i / (float)(freq_lines - 1);
            grid.push_back({{t, 0, 0}, {t, 0, 1}});
        }
        for (int d = 0; d < depth_lines; ++d) {
            float z = (float)d / (float)(depth_lines - 1);
            grid.push_back({{0, 0, z}, {1, 0, z}});
        }
        return grid;
    }
};

// =========================================================
// Test Main
// =========================================================

int main() {
    std::cout << "=== ZPlane Visualizer Wiring Test ===" << std::endl;

    // 1. Setup Core Data (Dummy Cube)
    ZPlaneCube testCube;
    for(int i=0; i<8; ++i) {
        for(int s=0; s<7; ++s) {
            // Set some resonant filter params
            testCube.corners[i].sections[s] = {0.5f, 0.8f, 1.0f, 0};
        }
    }

    // 2. Wire the Provider!
    ZPlaneVisualizer::set_coeff_provider([&testCube](float x, float y, float z) -> std::vector<VisCoeffs> {
        // A. Interpolate
        ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(testCube, x, y, z);
        
        std::vector<VisCoeffs> result;
        result.reserve(7);

        // B. Decode & Convert (Fixed -> Float)
        float sr = ZPlaneVisualizer::get_sample_rate();
        for(int s=0; s<7; ++s) {
            BiquadCoeffs fixed = ARMAdilloEngine::decode_section(frame.sections[s], sr);
            
            VisCoeffs floatCoeffs;
            floatCoeffs.b1 = fixed_to_float(fixed.b1);
            floatCoeffs.b2 = fixed_to_float(fixed.b2);
            floatCoeffs.a0 = fixed_to_float(fixed.a0);
            floatCoeffs.a1 = fixed_to_float(fixed.a1);
            floatCoeffs.a2 = fixed_to_float(fixed.a2);
            
            result.push_back(floatCoeffs);
        }
        return result;
    });

    // 3. Set Sample Rate
    ZPlaneVisualizer::set_sample_rate(48000.0f);

    // 4. Generate Ribbons
    std::cout << "Generating ribbons with wired provider..." << std::endl;
    auto ribbons = ZPlaneVisualizer::generate_ribbons(0.0f, 0.0f);

    std::cout << "Ribbon Count: " << ribbons.size() << " (Expected 8)" << std::endl;
    if (ribbons.empty()) return 1;

    std::cout << "Points per ribbon: " << ribbons[0].size() << " (Expected 128)" << std::endl;
    
    // Check middle point magnitude
    auto pt = ribbons[0][64]; // Middle frequency
    std::cout << "Mid-Freq Magnitude: " << pt.y << " (Normalized 0-1)" << std::endl;

    // 5. Generate Grid
    auto grid = ZPlaneVisualizer::generate_perspective_grid();
    std::cout << "Grid Lines: " << grid.size() << std::endl;

    std::cout << "Wiring Verification Complete." << std::endl;
    return 0;
}
