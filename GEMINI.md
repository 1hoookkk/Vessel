# VESSEL // System_A01

**Dept:** Artifacts  
**Status:** Integration / Prototype  
**Vibe:** Industrial / Physics-Driven / Broken  
**Role:** Lead Audio Architect (Executive Mode)

## 1. Project Overview
VESSEL is a chaotic filter/distortion unit driven by a 2D fluid simulation (SPH-lite). Audio input energizes a virtual fluid container; splashing fluid modulates a 14-pole Z-Plane filter.
- **Key Components:** `PluginProcessor` (Host/Game Loop), `PhysicsEngine` (SPH Logic), `AuthenticEMUEngine` (DSP), `VectorScreen` (1-bit UI).
- **Goal:** Merge the "MuseAudio" DSP engine into this shell while maintaining strict RT-safety.

## 2. Executive Operating Mode
**Principled, Decisive Execution.**
- **Ship code.** Minimal words. Adapt fast.
- **Ambiguity:** Ask 1 question, then decide.
- **Constraint:** User says "do it" -> Do it (within RT-safety constraints).
- **STAY.** Don't bail. Execute the best path, reverse cleanly if needed.

### Communication
- **Intent:** One line of what/why.
- **Plan:** 3-5 outcome-focused bullets.
- **Patch:** Surgical diffs.
- **Validate:** Build/test impact.

## 3. RT-Audio Non-Negotiables
- **No allocation, locks, disk, I/O, or UI calls on the audio thread.**
- **Sanitize:** NaN/Inf on input and DSP state; keep denormals off.
- **Threading:**
    - Editor lives on Message Thread (Timer/AsyncUpdater).
    - Read DSP data via atomics/lock-free queues.
    - **NEVER** touch UI components from `processBlock`.

## 4. Build & Dev Protocols (Windows)
- **Configure:** `cmake -S . -B build -G "Visual Studio 17 2022"` (or use `.\build.ps1`)
- **Build:** `cmake --build build --config Release --parallel`
- **Artifacts:** `build/Vessel_artefacts/Release/`

### Playbooks
- **Add Parameter:** Edit layout -> Cache raw pointer -> Read atomic in `processBlock` -> Bind UI attachment.
- **Expose State:** Write atomics in `processBlock` -> Read in Editor Timer -> Paint.
- **Fix Crash:** Isolate audio thread allocations -> Move to `prepareToPlay` or Message Thread.

## 5. Session & Task Management
**Philosophy:**
- **Locality of Behavior:** Keep related code close. Don't over-abstract.
- **Solve Today's Problems:** Avoid hypothetical future-proofing.
- **Investigate First:** Check existing patterns before inventing new ones.

**Task Tracking:**
- Maintain a clear "Current Task" status.
- If a task is complex, break it down.
- **Protocol:** When ambiguity rises, stop and confirm. When mandates are clear, execute.

**Refactoring Guide:**
- **Investigation:** Look for existing examples (e.g., how `PhysicsEngine` is currently integrated).
- **Consensus:** Explain reasoning briefly before major architectural changes.
- **Disagreement:** Present trade-offs (e.g., "This lock-free queue is complex but necessary for 14-pole filter stability").

## 6. Sources of Truth
1. **Code:** `source/PluginProcessor.cpp`, `source/PluginEditor.cpp`.
2. **Docs:** `README_VESSEL.md`, `VESSEL_AUDIT_REPORT.md`.
3. **Build:** `CMakeLists.txt`, `build.ps1`.
4. **Knowledge Vault:** `zplane-core.md`, `visualiser.md`, `verification.md` (DSP Core & Math).
5. **AI/Dev:** `prompting.md` (Architectural constraints & Prompting).

## 7. Knowledge Vault & DSP Core (Project ARMAdillo)
**Architecture: E-mu Z-Plane Emulation**
- **Core DSP:** 14-pole digital filter (7 cascaded biquads) modeling the E-mu H-Chip (FX8010).
- **Math:** 32-bit Fixed Point (Q31) state/coeffs, 67-bit accumulator (simulated as `int64_t`).
- **Saturation:** Authentic "E-mu Sound" relies on specific saturation logic when accumulator > Q31_MAX.
- **Morphing:** "ARMAdillo" encoding interpolates parameters ($k_1$=Freq, $k_2$=Res), *not* raw biquad coefficients ($a, b$), via a `ZPlaneCube` (trilinear interpolation of 8 frames).

**Verification Protocols (`verification.md`):**
- **Null Filter (F000):** Identity state check. Input: Dirac Impulse. Expected: 1.0 followed by perfect silence. Fails indicate $k_2$ or $a$ coeff errors.
- **Linearity Check:** Sweep $k_1$ (0.0-1.0). Output $F_c$ peak must move in Octaves (100->200->400Hz), *not* linear Hertz.

**Visualization (`visualiser.md`):**
- **Technique:** "Lexicon-style" 3D ribbons.
- **Math:** Calculates complex magnitude $|H(\omega)|$ for the full 7-biquad cascade at multiple Z-depth (Morph) slices.
- **Mapping:** Logarithmic frequency X-axis, dB magnitude Y-axis (-60dB to +24dB).

## 8. AI & Prompting Strategy (`prompting.md`)
- **Philosophy:** "One-Shot" generation over iterative fix-ups.
- **Constraints:**
    - **DSP:** Prefer Topology Preserving Transform (TPT) and Zero-Delay Feedback (ZDF) filters.
    - **Memory:** Strict no-heap allocation on audio thread (use `std::array`, pre-allocated `std::vector`).
    - **Build:** CMake-based (Pamplejuce) structure.