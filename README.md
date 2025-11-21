# VESSEL

**A 14-Pole Morphing Z-Plane Filter**
*Project ARMAdillo*

---

## Overview

VESSEL is a VST3/AU audio plugin that emulates the legendary E-mu H-Chip (FX8010) Z-Plane filter architecture. It features a cascade of 7 biquad sections (14 poles total) with authentic fixed-point arithmetic and hardware-accurate saturation, delivering the iconic "E-mu sound."

### Key Features

- **14-Pole Cascade**: 7 biquad sections in series for complex filtering
- **ARMAdillo Encoding**: Interpolation in k-space (Pitch/Resonance), not coefficient-space
- **Filter Cube**: Trilinear interpolation between 8 filter frames for smooth morphing
- **Fixed-Point Arithmetic**: 32-bit state/coefficients with 64-bit accumulation
- **Real-Time Safe**: Zero allocations in audio processing thread
- **3D Visualization**: "Lexicon-style" frequency response ribbons with depth

### Aesthetic: "Fragile Brutalism"

The UI embraces a minimalist, technical aesthetic inspired by haunted hardware:
- High contrast terminal green on deep black
- 1px thin lines, no gradients
- Monochromatic, minimal
- Ghostly depth effects in visualization

---

## Architecture

### DSP Engine (`Structure/DSP/ZPlaneCore.h`)

The core DSP engine preserves the exact H-Chip mathematics:

```cpp
// Q31 Fixed-Point Format
using dsp_sample_t = int32_t;  // 1 sign bit, 0 integer bits, 31 fractional bits
using dsp_accum_t = int64_t;   // 64-bit accumulator for MAC operations

// Hardware-accurate saturation (branchless)
inline dsp_sample_t saturate(dsp_accum_t x);
```

**Key Classes:**
- `EmuHChipFilter`: The main filter engine with cascade processing
- `ARMAdilloEngine`: Trilinear interpolation and k-space decoding
- `ZPlaneCube`: 8-corner parameter space for morphing

### Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| **MORPH** | 0.0 - 1.0 | Interpolates the Z-Plane X-axis (filter character) |
| **FREQ** | 0.0 - 1.0 | Shifts the Y-axis (pitch tracking) |
| **TRANS** | 0.0 - 1.0 | Shifts the Z-axis (transform/resonance) |

### Thread Safety

VESSEL adheres to strict real-time safety protocols:

- **Audio Thread**: Uses `std::atomic<float>*` for lock-free parameter access
- **Message Thread**: Runs visualization calculations on a timer
- **Communication**: `juce::AbstractFifo` for lock-free data transfer
- **No Allocations**: All buffers pre-allocated in constructor

---

## Building

### Requirements

- **CMake** 3.24 or higher
- **C++23** compiler (GCC 11+, Clang 14+, MSVC 2022+)
- **JUCE** 8.0 or higher
- VST3 SDK (optional, for VST3 format)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/Vessel.git
cd Vessel

# Initialize JUCE submodule (if not using CPM)
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Built plugins will be in:
# - build/Vessel_artefacts/Release/VST3/
# - build/Vessel_artefacts/Release/AU/
```

### Platform-Specific Notes

**macOS:**
```bash
# AU and VST3 will be automatically copied to:
# ~/Library/Audio/Plug-Ins/Components/ (AU)
# ~/Library/Audio/Plug-Ins/VST3/ (VST3)
```

**Windows:**
```bash
# VST3 will be copied to:
# C:\Program Files\Common Files\VST3\
```

**Linux:**
```bash
# VST3 will be copied to:
# ~/.vst3/
```

---

## Usage

1. **Load VESSEL** in your DAW as a VST3 or AU plugin
2. **Insert on a track** (works best on melodic material or synths)
3. **Adjust parameters**:
   - **MORPH**: Sweep to explore different filter characters
   - **FREQ**: Modulate for pitch tracking effects
   - **TRANS**: Control resonance and transform characteristics
4. **Watch the visualization**: See the frequency response morph in real-time

### Tips

- **Automate MORPH**: Create evolving, dynamic filter sweeps
- **Combine with LFOs**: Modulate FREQ and TRANS for movement
- **Push TRANS high**: Explore screaming resonances (be careful with levels!)
- **Layer multiple instances**: Create complex, multi-dimensional filtering

---

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
float theta = 2.0f * π * freqHz / sampleRate;
float b1 = 2.0f * R * std::cos(theta);
```

### Visualization

The 3D ribbon display calculates frequency response in real-time:

1. **Trilinear Interpolation**: Generate frame at current MORPH/FREQ/TRANS
2. **Decode Coefficients**: Convert to biquad coefficients
3. **Frequency Response**: Calculate |H(ω)| for 128 points (20Hz - 20kHz)
4. **Render Ribbons**: Draw 8 depth slices with opacity falloff

---

## Project Structure

```
Vessel/
├── CMakeLists.txt           # Main CMake configuration
├── Source/                  # Plugin source code
│   ├── PluginProcessor.h/cpp    # Main audio processor
│   ├── PluginEditor.h/cpp       # GUI and look-and-feel
│   └── ZPlaneDisplay.h          # 3D visualization component
├── Structure/               # DSP architecture
│   └── DSP/
│       └── ZPlaneCore.h         # Fixed-point filter engine
├── libs/                    # Third-party libraries
│   └── JUCE/                    # JUCE framework (submodule)
└── README.md
```

---

## License

This project is licensed under the **GPLv3** license. See [LICENSE](LICENSE) for details.

VESSEL uses the JUCE framework, which has its own licensing terms. Please review JUCE's license at [juce.com](https://juce.com/legal).

---

## Credits

**Z-Plane Implementation Lead**: ARMAdillo Project
**Based on**: E-mu Systems H-Chip/FX8010 Architecture
**Framework**: JUCE 8

Special thanks to:
- Dave Rossum (Rossum Electro-Music) for pioneering work on digital filter topology
- E-mu Systems for the legendary Z-Plane filter architecture
- Ross Bencina for real-time audio safety protocols

---

## References

- [E-mu Z-Plane Synthesis](https://en.wikipedia.org/wiki/E-mu_Systems#Z-Plane_synthesis)
- [Dave Rossum Patents](https://patents.google.com/?inventor=Dave+Rossum)
- [Real-Time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)
- [JUCE Framework](https://juce.com/)

---

## Contributing

Contributions are welcome! Please open an issue or pull request on GitHub.

### Development Guidelines

1. **No allocations in `processBlock()`** - Ever.
2. **Preserve fixed-point math** - Don't convert to floating-point biquads
3. **Namespace discipline** - No `using namespace juce;` in headers
4. **Test on all platforms** - macOS, Windows, Linux
5. **Follow the aesthetic** - "Fragile Brutalism" is law

---

**Made with obsessive attention to detail.**
*"The math must be exact."*
