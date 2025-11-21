# VESSEL // System_A01

**Dept:** Artifacts  
**Status:** Prototype  
**Vibe:** Clinical / Physics-Driven / Precise

VESSEL is a chaotic filter/distortion unit driven by a 2D fluid simulation (SPH-lite). The audio input energizes a virtual fluid container. As the fluid splashes and swirls, it modulates the filter cutoff and resonance.

## The Interface
- **Screen:** A high-contrast "Lab-Clean" display showing dark fluid particles on a gallery white field.
- **Gravity X/Y:** Tilts the container, sloshing the fluid around.
- **Mode:**
    1.  **LOW_DAMP:** Lowpass filter. Sub-bass protected. Fluid dispersion adds resonance.
    2.  **HIGH_TEAR:** Highpass filter. High resonance. Screams when fluid explodes.
    3.  **PHASE_WARP:** Bandpass filter. Sweeps rapidly based on fluid velocity.

## How to Build (Windows)
1. Ensure you have **Visual Studio 2022** (with C++ desktop dev) and **CMake** installed.
2. Right-click `build.ps1` and select "Run with PowerShell" (or run `.\build.ps1` in terminal).
3. The VST3 plugin will be in `build/Vessel_artefacts/Release/VST3/`.

## Vibe Tuning
If the physics feels too "clean", edit `source/PhysicsEngine.h`:
- Increase `kCount` for more particles (CPU heavy).
- Decrease `kDamping` (e.g. to `0.98f`) to make the fluid slippery and never stop moving.
- Increase `vortexSpin` to make it spin faster when audio hits hard.
