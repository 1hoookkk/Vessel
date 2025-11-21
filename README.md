# VESSEL // CORIUM REACTOR

**Aesthetic:** Decommissioned Lab Equipment
**Architecture:** Physics-Driven Z-Plane Filter
**Target:** Trap Production with Heavy Inertia

---

## Concept

VESSEL is a Z-Plane filter where parameters are controlled by a **Physics Engine** (Smoothed Particle Hydrodynamics) rather than traditional LFOs. The user fights fluid mass to change the sound.

This is not a "tool." This is an **organism**.

---

## Architecture

### Core Components

#### 1. **MuseZPlaneEngine** (`source/DSP/MuseZPlaneEngine.h`)
- Unified wrapper for Z-Plane filter implementations
- Adapter pattern supporting Fast/Authentic modes via `std::variant`
- Thread-safe parameter setters
- Pole visualization interface for UI

**Key Methods:**
```cpp
void setMorph(float m);        // Morph position (0-1)
void setIntensity(float i);    // Resonance/Q
void setDrive(float d);        // Distortion
void process(float* L, float* R, int numSamples);
```

#### 2. **Corium PhysicsEngine** (`source/Physics/PhysicsEngine.h`)
- SPH (Smoothed Particle Hydrodynamics) with 100 particles
- Audio-reactive forces: repulsion, vorticity, cohesion
- Heavy inertia simulation (viscosity = 0.95)

**Physics Mappings:**
- `CentroidX` → Filter Morph
- `CentroidY` → Drive/Degrade
- `Dispersion` → Resonance (more spread = higher Q)

**Control Parameters:**
```cpp
void setSensitivity(float s);  // Audio reactivity (default: 1.0)
void setViscosity(float v);    // Drag (0.95 = heavy sludge)
void setAttraction(float a);   // User control strength (0.02 = fight)
```

#### 3. **MonitorView** (`source/UI/MonitorView.h`)
- 1970s Amber Monochrome Vector Display aesthetic
- 64x64 bitmap cache with metaball rendering
- Nearest-neighbor upscaling for visible pixels
- Scanline overlay for CRT authenticity

---

## Shape Modes: The Three Personalities

VESSEL features three distinct **Shape Modes** that completely alter the character of the filter, physics behavior, and visual display. Each mode is designed for a specific sonic aesthetic and automatically engages appropriate safety/vintage behaviors.

### LOW DAMP (Trap Anchor - Modern Safe)

**Character:** The modern, thick, underwater "Trap" filter.

**DSP Behavior:**
- 14-pole lowpass configuration
- Sub-Protect **FORCED ON** @ 180Hz (preserves 808 rumble)
- Smooth modulation (high resolution, no stepping)
- Adaptive gain compensation enabled (maintains volume)

**Physics Visual:**
- Heavy clustering (high cohesion)
- Particles stick together at bottom of display
- Standard amber color (#FF9900)
- Viscosity: 0.95 (very heavy drag)

**Use Case:** Safe for production work. Won't destroy your sub-bass. Classic Trap/Drake underwater effect.

---

### HIGH TEAR (Audity 2000 Ghost - Vintage Dangerous)

**Character:** Authentic E-mu Audity 2000 emulation. Thin, harsh, digital screaming.

**DSP Behavior:**
- 14-pole highpass with **procedural coefficient spreading**
- Sub-Protect **FORCED OFF** (phase destroys lows - intentional)
- Stepped modulation @ 64 samples (E-mu "zipper" effect for digital grit)
- Adaptive gain **DISABLED** (volume drops up to 60% as resonance increases)
- Resonance drop formula: `gain *= (1.0 - intensity * 0.6)`

**Procedural Coefficient Generation:**
```cpp
// Poles spread apart as morph increases ("tearing" the spectrum)
baseTheta = 0.15 + (morph * 0.4);  // High frequency range (1kHz-10kHz)
r = 0.88 + (intensity * 0.11);     // Pole radius (high Q)
// 7 conjugate pairs with alternating spread directions
// Zeros: 4 at DC (hard bass cut), 3 distributed for ripple
```

**Physics Visual:**
- Scattered particles (low cohesion)
- Hyper-reactive to audio (sensitivity: 1.5x)
- Red-amber warning color (#FF6600)
- Viscosity: 0.88 (lighter, chaotic)
- **WARNING OVERLAY:** "SUB PHASE BYPASSED" / "GAIN COMPENSATION DISABLED"

**Use Case:** Hi-hat shredding. Yeat/Carti style thin electric screech. **NOT safe for 808s** - will phase-warp and hollow out your low end.

---

### PHASE WARP (Alien - Hybrid)

**Character:** All-pass/comb mesh. Strange, hollow, phaser-like artifacts.

**DSP Behavior:**
- 14-stage allpass/comb configuration
- Sub-Protect **USER CHOICE** (respects global toggle)
- Fluid modulation (smooth)
- Adaptive gain enabled

**Physics Visual:**
- Concentric ring formation behavior
- Balanced reactivity (sensitivity: 0.8x - calmer)
- Cyan-tinted color (R:77, G:204, B:128)
- Viscosity: 0.92 (medium)

**Use Case:** Experimental textures. Liquid phase artifacts. Alien soundscapes.

---

## UI: Shape Mode Selector

**Location:** Title bar, right side

**Control:** Dropdown selector labeled "SIGNAL PATH:"
- LOW DAMP
- HIGH TEAR
- PHASE WARP

**Visual Feedback:**
- MonitorView changes color scheme per mode
- HIGH_TEAR displays warning text overlay
- Particle behavior adapts automatically

**Implementation:** source/PluginEditor.cpp:28

---

## DSP Signal Flow (Mode-Dependent)

```
Audio Input
    ↓
RMS Energy Calculation
    ↓
Shape Mode Configuration
    ├─ Detect current mode (LOW_DAMP/HIGH_TEAR/PHASE_WARP)
    ├─ Set behavior flags (sub-protect, stepped modulation, resonance drop)
    ├─ Configure filter engine (enable/disable adaptive gain)
    └─ Set physics visual profile (clustering, scatter, or ring)
    ↓
Physics Update (every 32 samples)
    ├─ Audio Energy → Vorticity (spin)
    ├─ User Target → Attraction Forces
    └─ Output: Centroid + Dispersion
    ↓
Modulation Routing
    ├─ HIGH_TEAR: Stepped morph (64-sample decimation)
    └─ Others: Smooth morph (high resolution)
    ↓
Parameter Mapping
    ├─ CentroidX → Filter Morph
    ├─ Dispersion → Filter Resonance
    └─ (Breach Mode) Dispersion → Drive
    ↓
Sub-Protect Logic (Mode-Dependent)
    ├─ LOW_DAMP: FORCED ON @ 180Hz
    │   ├─ Crossover split (Linkwitz-Riley)
    │   ├─ LOW: Pass through clean
    │   ├─ HIGH: Apply filter
    │   └─ Sum: Clean sub + Mangled highs
    ├─ HIGH_TEAR: FORCED OFF (full spectrum destruction)
    └─ PHASE_WARP: User choice
    ↓
Resonance Drop (HIGH_TEAR only)
    └─ Apply volume reduction: gain *= (1.0 - intensity * 0.6)
    ↓
Audio Output
```

**Key Decision Points:**
- source/PluginProcessor.cpp:141 - Shape mode switch
- source/PluginProcessor.cpp:187 - Stepped vs smooth modulation
- source/PluginProcessor.cpp:217 - Sub-protect application
- source/PluginProcessor.cpp:266 - Resonance drop

---

## Sub-Protect (Trap Production Safety)

**Problem:** Z-Plane filters can phase-warp sub-bass, ruining 808s.

**Solution:** Crossover at 180Hz
- **Below 180Hz:** Clean pass-through (preserve sub rumble)
- **Above 180Hz:** Full Corium destruction (texture without killing low end)

Controlled via `subProtectEnabled_` atomic bool (source/PluginProcessor.h:61)

---

## Breach Mode

When `isBreach_` is enabled:
- Dispersion drives Distortion instead of user parameter
- More particle spread = harder 808 destruction
- For when you want maximum chaos

---

## Build Instructions

### Prerequisites
- CMake 3.24+
- C++20 compiler (MSVC 2019+, GCC 11+, Clang 14+)
- JUCE 8.0.0 (automatically fetched)

### Windows (MSVC)
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

### macOS/Linux
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Build Outputs
- **VST3:** `build/Vessel_artefacts/Release/VST3/Vessel.vst3`
- **AU:** `build/Vessel_artefacts/Release/AU/Vessel.component` (macOS only)
- **Standalone:** `build/Vessel_artefacts/Release/Standalone/Vessel`

---

## Integration: Adding E-MU Filter Code

The current implementation uses **placeholder stubs** for the Z-Plane filter.

To integrate your authentic E-MU extraction:

1. **Place your filter code** in a new directory (e.g., `source/DSP/emu_extracted/`)

2. **Update includes** in `source/DSP/MuseZPlaneEngine.h:11-13`:
   ```cpp
   #include "emu_extracted/ZPlaneFilter_fast.h"
   #include "emu_extracted/AuthenticEMUZPlane.h"
   ```

3. **Remove placeholder stubs** (lines 21-45 in MuseZPlaneEngine.h)

4. **Verify adapters** match your actual class interfaces (lines 128-183)

---

## Physics Tuning Guide

### Heavy Sludge (Current Defaults)
```cpp
physicsEngine_.setViscosity(0.95f);      // High drag
physicsEngine_.setAttraction(0.02f);     // Weak pull (fight)
physicsEngine_.setSensitivity(1.0f);     // Full audio reactivity
```

### Water (Lighter Feel)
```cpp
physicsEngine_.setViscosity(0.80f);      // Less drag
physicsEngine_.setAttraction(0.08f);     // Stronger pull (easier)
```

### Spin/Vorticity
Controlled internally in `PhysicsEngine.h:53`:
```cpp
const float spin = energy * 2.5f;  // Increase for more swirl
```

---

## Thread Safety

### Audio Thread (CRITICAL)
- **NO allocations** in `processBlock`
- **NO locks** in physics update
- Sub-protect uses stack-allocated buffers
- All parameter updates via atomics

### Message Thread
- Parameter changes from UI
- Engine mode switching (Fast/Authentic)
- Safe via `std::atomic` and `std::variant::emplace`

---

## File Structure

```
Vessel/
├── CMakeLists.txt              # JUCE 8 + CMake config
├── CLAUDE.md                   # Project context
├── README.md                   # This file
├── .gitignore
└── source/
    ├── PluginProcessor.h/cpp   # Main audio processor
    ├── PluginEditor.h/cpp      # Main editor
    ├── DSP/
    │   └── MuseZPlaneEngine.h  # Unified filter wrapper
    ├── Physics/
    │   └── PhysicsEngine.h     # Corium SPH (header-only)
    └── UI/
        ├── MonitorView.h       # Amber CRT display
        └── MonitorView.cpp
```

---

## Next Steps

1. **Integrate E-MU filter code** (replace stubs in MuseZPlaneEngine.h)
2. **Build and test** with the Standalone app
3. **Tune physics constants** for desired feel
4. **Add parameter automation** (JUCE ValueTree)
5. **Implement preset system** (JSON serialization)

---

## Notes

- Physics updates every **32 samples** (optimal for 48kHz)
- MonitorView renders at **30 Hz** (intentional CRT persistence)
- Metaball threshold: `0.35f` (increase for tighter blobs)
- Amber color: RGB(255, 153, 0) with discrete brightness levels

---

**Serial Number:** CRM-1974-Z14
**Classification:** Experimental Audio Reactor
**Status:** Decommissioned (Reactivated for Production Use)
