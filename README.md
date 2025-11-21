# VESSEL

**A 14-Pole Morphing Z-Plane Filter**
**Project ARMAdillo**

--------

## Overview

VESSEL is a VST3/AU audio plugin that emulates the legendary E-mu H-Chip (FX8010)
Z-Plane filter architecture. It features a cascade of 7 biquad sections (14
poles total) with authentic fixed-point arithmetic and hardware-accurate
saturation, delivering the iconic "E-mu sound."

### Key Features

• **14-Pole Cascade:** 7 biquad sections in series for complex filtering
• **ARMAdillo Encoding:** Interpolation in k-space (Pitch/Resonance), not coefficient-space
• **Filter Cube:** Trilinear interpolation between 8 filter frames for smooth morphing
• **Fixed-Point Arithmetic:** 32-bit state/coefficients with 64-bit accumulation
• **Real-Time Safe:** Zero allocations in audio processing thread
• **3D Visualization:** "Lexicon-style" frequency response ribbons with depth

### Aesthetic: "Fragile Brutalism"

The UI embraces a minimalist, technical aesthetic inspired by haunted hardware:

• High contrast terminal green on deep black
• 1px thin lines, no gradients
• Monochromatic, minimal
• Ghostly depth effects in visualization

--------

## Architecture

### DSP Engine (`source/DSP/EmuZPlaneCore.h`)

The core DSP engine preserves the exact H-Chip mathematics:

```cpp
// Q31 Fixed-Point Format
using dsp_sample_t = int32_t;  // 1 sign bit, 0 integer bits, 31 fractional bits
using dsp_accum_t = int64_t;   // 64-bit accumulator for MAC operations

// Hardware-accurate saturation (branchless)
inline dsp_sample_t saturate(dsp_accum_t x);
```

Key Classes:

• `EmuHChipFilter`: The main filter engine with cascade processing
• `ARMAdilloEngine`: Trilinear interpolation and k-space decoding
• `ZPlaneCube`: 8-corner parameter space for morphing

### Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| **MORPH** | 0.0 - 1.0 | Interpolates the Z-Plane X-axis (filter character) |
| **FREQ** | 0.0 - 1.0 | Shifts the Y-axis (pitch tracking) |
| **TRANS** | 0.0 - 1.0 | Shifts the Z-axis (transform/resonance) |

### Thread Safety

VESSEL adheres to strict real-time safety protocols:

• **Audio Thread:** Uses `float*` raw pointers for zero-allocation processing
• **Message Thread:** Runs visualization calculations on a timer
• **Communication:** Triple-buffered state for lock-free visualization
• **No Allocations:** All buffers pre-allocated in constructor

--------

## Building

### Requirements

• CMake 3.24 or higher
• C++20 compiler (GCC 11+, Clang 14+, MSVC 2022+)
• JUCE 8.0 (automatically fetched)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/1hoookkk/Vessel.git
cd Vessel

# Create build directory
mkdir build && cd build

# Configure CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release
```

--------

## Usage

1. Load VESSEL in your DAW as a VST3 or AU plugin
2. Insert on a track (works best on melodic material or synths)
3. Adjust parameters:
   • **MORPH:** Sweep to explore different filter characters
   • **FREQ:** Modulate for pitch tracking effects
   • **TRANS:** Control resonance and transform characteristics
4. Watch the visualization: See the frequency response morph in real-time

--------

## Technical Details

### Fixed-Point Math

The Z-Plane core uses Q31 fixed-point arithmetic to precisely replicate the H-Chip's behavior:

```cpp
// Biquad processing (Direct Form I)
dsp_accum_t acc = 0;
acc += (dsp_accum_t)x * c.a0;      // Feedforward
acc += (dsp_accum_t)s.x1 * c.a1;
acc += (dsp_accum_t)s.x2 * c.a2;
acc += (dsp_accum_t)s.y1 * c.b1;   // Feedback
acc += (dsp_accum_t)s.y2 * c.b2;
acc >>= 31;                         // Q62 -> Q31
dsp_sample_t y = saturate(acc);    // Clamp to Q31 range
```

### Coefficient Decoding

The ARMAdillo engine decodes k-space parameters to biquad coefficients:

```cpp
// Resonance (k2) -> Pole Radius
float R = 0.0f + (0.98f * params.k2);
float b2 = -(R * R);

// Frequency (k1) -> Theta
float freqHz = 20.0f * std::pow(1000.0f, params.k1);
float theta = 2.0f * 3.14159f * freqHz / sampleRate;
float b1 = 2.0f * R * std::cos(theta);
```

--------

## License

This project is licensed under the GPLv3 license.

VESSEL uses the JUCE framework, which has its own licensing terms. Please review JUCE's license at [juce.com](https://juce.com/legal).

--------

## Credits

**Z-Plane Implementation Lead:** Project ARMAdillo
**Based on:** E-mu Systems H-Chip/FX8010 Architecture
**Framework:** JUCE 8

Special thanks to:
• Dave Rossum (Rossum Electro-Music)
• E-mu Systems
• Ross Bencina (Real-time Audio Programming)