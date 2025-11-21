#include "zplane_visualizer_math.h"
#include "EmuZPlaneCore.h" // Ensure we access ARMAdilloEngine
#include <iostream>
#include <algorithm>

std::complex<float> ZPlaneVisualizer::get_biquad_response(const BiquadCoeffs& c, float omega) {
    // z^-1 = e^(-j * omega) = cos(omega) - j*sin(omega)
    std::complex<float> z_1 = std::polar(1.0f, -omega);
    std::complex<float> z_2 = std::polar(1.0f, -2.0f * omega);

    // Convert Fixed Point to Float for visualization
    float a0 = fixed_to_float(c.a0);
    float a1 = fixed_to_float(c.a1);
    float a2 = fixed_to_float(c.a2);
    float b1 = fixed_to_float(c.b1);
    float b2 = fixed_to_float(c.b2);

    std::complex<float> numerator = a0 + a1 * z_1 + a2 * z_2;
    std::complex<float> denominator = 1.0f - (b1 * z_1 + b2 * z_2);

    if (std::abs(denominator) < 1e-9f) return std::complex<float>(0,0);

    return numerator / denominator;
}

std::vector<std::vector<VisPoint3D>> ZPlaneVisualizer::generate_ribbons(
    const ZPlaneCube& cube,
    float freq_axis_y, 
    float transform_z,
    float sample_rate_hz
) {
    std::vector<std::vector<VisPoint3D>> ribbons;

    for (int i = 0; i < SLICE_COUNT; ++i) {
        float morph_x = (float)i / (float)(SLICE_COUNT - 1);
        
        std::vector<VisPoint3D> current_ribbon;
        current_ribbon.reserve(RESOLUTION);

        // Interpolate Frame using ARMAdillo
        ZPlaneFrame frame = ARMAdilloEngine::interpolate_cube(cube, morph_x, freq_axis_y, transform_z);
        
        // Decode Coeffs
        std::vector<BiquadCoeffs> sections;
        sections.reserve(ZPlaneFrame::SECTION_COUNT);
        for(int s=0; s<ZPlaneFrame::SECTION_COUNT; ++s) {
            sections.push_back(ARMAdilloEngine::decode_section(frame.sections[s], sample_rate_hz));
        }

        for (int j = 0; j < RESOLUTION; ++j) {
            // Logarithmic frequency mapping
            float t = (float)j / (float)(RESOLUTION - 1);
            float freqHz = 20.0f * std::pow(1000.0f, t);
            float omega = 2.0f * 3.14159265f * freqHz / sample_rate_hz;

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

std::vector<std::vector<VisPoint3D>> ZPlaneVisualizer::generate_perspective_grid(int freq_lines, int depth_lines) {
    std::vector<std::vector<VisPoint3D>> grid;
    for (int i = 0; i < freq_lines; ++i) {
        float t = (float)i / (float)(freq_lines - 1);
        std::vector<VisPoint3D> line;
        line.push_back({t, 0.0f, 0.0f});
        line.push_back({t, 0.0f, 1.0f});
        grid.push_back(line);
    }
    for (int d = 0; d < depth_lines; ++d) {
        float z = (float)d / (float)(depth_lines - 1);
        std::vector<VisPoint3D> line;
        line.push_back({0.0f, 0.0f, z});
        line.push_back({1.0f, 0.0f, z});
        grid.push_back(line);
    }
    return grid;
}
