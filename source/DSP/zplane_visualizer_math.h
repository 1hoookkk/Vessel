#pragma once
#include <vector>
#include <complex>
#include <functional>
#include <cmath>
#include "EmuZPlaneCore.h" // Needed for ZPlaneCube

// Data Structures for Visualization
struct VisCoeffs {
    float b1, b2;
    float a0, a1, a2;
};

struct VisPoint3D {
    float x; // Frequency (Normalized 0.0 - 1.0 or Log)
    float y; // Magnitude (dB or Linear Gain)
    float z; // Morph Depth (0.0 - 1.0)
};

class ZPlaneVisualizer {
public:
    // Math Helpers
    static std::complex<float> get_biquad_response(const BiquadCoeffs& c, float omega);

    // Generators
    static std::vector<std::vector<VisPoint3D>> generate_ribbons(
        const ZPlaneCube& cube,
        float freq_axis_y, 
        float transform_z,
        float sample_rate_hz
    );

    static std::vector<std::vector<VisPoint3D>> generate_perspective_grid(int freq_lines = 6, int depth_lines = 6);

private:
    static const int RESOLUTION = 128;
    static const int SLICE_COUNT = 8;
};
