# ARMAdillo Core: Authentic H-Chip Emulation
**Project:** HEAVY Plugin (E-mu FX8010 "H-Chip" Emulator)
**Status:** Phase 1-2 ✅ Complete | Phase 3 ⏳ In Progress | Phase 4 🔄 Active

---

## Role & Objective

You are a **Digital Archaeologist and DSP Engineer**. Your goal is to create a bit-accurate emulation of the E-mu FX8010 "H-Chip" found in the Audity 2000. **Authenticity trumps "cleanliness."**

The DSP engine relies on Hardware Constraints to generate its character. Modern "best practices" do NOT apply here—you are preserving hardware behavior, not optimizing it.

---

## Core Hardware Constraints (IMMUTABLE REFERENCE)

This section defines the H-Chip specification. **Do not modify.** Agents validate against this ground truth.

### 1. The H-Chip Topology

- **Structure:** Series Cascade of 7 Biquad Sections (14 poles)
- **Form:** Transposed Direct Form II (TDF-II)
- **Components:** Full Biquads (Poles + Zeros). Do NOT use All-Pole Resonators.

### 2. The Data Path (The "Thinness")

- **Input/Output:** 32-bit Signed Integer (`int32_t`)
- **Coefficients:** 16-bit Signed Integer (`int16_t`)
  - **Constraint:** You MUST truncate calculated float coefficients to 16-bit precision before use. This quantization error is the source of the "Thin" sound.
- **Accumulator:** 64-bit Integer (`int64_t`)
  - **Constraint:** All multiply-accumulate operations occur in 64-bit space.

### 3. The Saturation Policy (The "Growl")

- **Rule:** Inter-Stage Saturation
- **Logic:** You must saturate the signal after every single biquad stage.
- **Mechanism:**
  ```cpp
  // The H-Chip Flow
  int64_t acc = (int64_t)b0 * x + z1;
  int32_t y = saturate_to_32bit(acc >> 15); // Shift Q46->Q31 THEN clip
  // Y becomes X for the next stage
  ```

### 4. Control Rate (The "Buzz")

- **Rule:** 32-Sample Lock
- **Constraint:** Filter coefficients update ONLY every 32 samples (~1.2kHz at 48kHz)
- **Forbidden:** Do NOT use linear interpolation or parameter smoothing on coefficients. The "stepping" artifacts are required.

---

## Implementation Status

### Phase 1: The Engine ✅ COMPLETE
**Actual Files:** `source/DSP/EmuHChipFilter.h`, `source/DSP/EmuZPlaneCore.{cpp,h}`

**What Was Implemented:**
- 7-stage TDF-II biquad cascade with int64 accumulators
- 32-sample control rate loop (no coefficient smoothing)
- Sysex→ZPlaneFrame decoder in PresetConverter
- Inter-stage saturation after every biquad
- Coefficient quantization: float→int16 Q15 truncation

**Current Behavior:**
- Signal path: int32 input → int64 accumulation → int32 output per stage
- Coefficients update every 32 samples, producing authentic 1.2kHz zipper artifacts
- All DSP operations use fixed-point integer math (no float in hot path)

**Recent Fixes:**
- **2025-11-24:** Fixed coefficient overflow at extreme parameters (b1 clamping)
- **2025-11-24:** Added thread-safe atomic parameter reads
- **2025-11-24:** Implemented coefficient scaling to prevent Q15 range violations

**Known Limitations:**
- No parameter smoothing (intentional—produces required 1.2kHz sidebands)
- Drive stage currently disabled (set to 0dB in code)

**Critical Files:**
- `source/DSP/EmuHChipFilter.h` - Filter kernel (process_block, update_coefficients)
- `source/DSP/EmuZPlaneCore.{cpp,h}` - ARMAdillo engine (interpolate_cube, decode_section)
- `source/DSP/PresetConverter.{cpp,h}` - Shape→Cube mapping

---

### Phase 2: The Interface ✅ COMPLETE
**Actual Files:** `source/PluginProcessor.{cpp,h}`, `source/PluginEditor.{cpp,h}`

**What Was Implemented:**
- Three UI macros: TENSION (frequency), HARDNESS (resonance), ALLOY (material path)
- Eight material types: Iron → Steel → Brass → Aluminum → Glass → Quartz → Carbon → Uranium
- Thread-safe parameter flow via WaitFreeTripleBuffer
- Auto-gain compensation system
- Basic pole visualization (oil drop renderer)

**Current Behavior:**
- Real-time parameter updates without blocking audio thread
- Material path morphs through 3D ZPlane cube coordinates
- All parameters respond to MIDI automation

**Recent Fixes:**
- **2025-11-24:** Atomic parameter reads prevent torn values
- **2025-11-24:** Reduced auto-gain makeup from 24dB→12dB
- **2025-11-24:** Added NaN/Inf stability checks with filter reset

**Known Limitations:**
- No visual feedback on drive saturation
- No per-stage visualization yet
- Material change check has unpredictable branch (minor performance impact)

**Critical Files:**
- `source/PluginProcessor.{cpp,h}` - Audio thread coordinator
- `source/PluginEditor.{cpp,h}` - UI components
- `source/VesselStyle.h` - Design system
- `source/Utility/WaitFreeTripleBuffer.h` - Lock-free parameter sync

---

### Phase 3: The "Glass Cube" Visualization ⏳ IN PROGRESS
**Status:** Basic JUCE Graphics implementation exists (not OpenGL)

**Current Implementation:**
- Low-res "oil drop" renderer (64x48 buffer, upscaled)
- Displays interpolated pole positions
- 30fps timer-based refresh

**Next Steps:**
- Evaluate performance requirements
- If optimization needed, create OpenGL Glass Engine using `glass-engine-builder` agent
- Add reactive glow effects based on output RMS

**Critical Files:**
- `source/PluginEditor.cpp:92-122` - Oil buffer rendering

---

### Phase 4: Verification 🔄 ACTIVE (Parallel Agent System)
**Status:** Agent system operational, tests pending

**Implemented Verification:**
- Parallel agent audit workflow (Pattern A/B/C/D)
- Four production agents: h-chip-spec-guardian, fixed-point-saturation-auditor, realtime-audio-guardian, morph-preset-cartographer
- Conflict resolution matrix (spec > fixed-point > realtime > morph)

**Pending Verification Tests:**

| Test | Status | Expected Result | Agent |
|------|--------|-----------------|-------|
| Null Test (F000 Preset) | 📋 Planned | Unity gain passthrough | h-chip-spec-guardian |
| Linearity Test (k1→Octaves) | 📋 Planned | Logarithmic frequency mapping | morph-preset-cartographer |
| Zipper Test (1.2kHz Sidebands) | 📋 Planned | Audible stepping on fast morphs | h-chip-spec-guardian |
| Coefficient Stability | ✅ Pass | No overflow at extremes (fixed 2025-11-24) | fixed-point-saturation-auditor |
| Thread Safety | ✅ Pass | No torn reads (fixed 2025-11-24) | realtime-audio-guardian |

---

## Verification Workflow

### Philosophy: Parallel-First Auditing

When DSP code changes, invoke **all relevant agents simultaneously** for fastest validation. Agents are organized by **concern** (not files).

**See `.claude/agent-roster.md` for detailed agent definitions, invocation patterns, and conflict resolution matrix.**

### Quick Reference: Which Pattern?

| Your Change | Pattern | Agents (Parallel) |
|-------------|---------|-------------------|
| New biquad implementation | Pattern A | h-chip-spec + fixed-point + realtime |
| Coefficient calculation | Pattern A | h-chip-spec + fixed-point + realtime |
| Saturation logic | Pattern A | h-chip-spec + fixed-point + realtime |
| Preset bank update | Pattern B | morph + h-chip-spec + fixed-point |
| UI parameter mapping | Pattern B | morph + h-chip-spec |
| SIMD/optimization | Pattern C | realtime + fixed-point + h-chip-spec |
| Major refactor | Pattern D | ALL AGENTS |

### Tier 1 Agents (Mandatory Checks)
- **h-chip-spec-guardian** 🔴 - Topology compliance (7 sections, TDF-II, series cascade)
- **fixed-point-saturation-auditor** 🔵 - Bit-depth contracts (int32→int64→int32, saturation)
- **realtime-audio-guardian** 🟡 - Audio thread safety (allocation-free, lock-free)

### Tier 2 Agents (Conditional Checks)
- **morph-preset-cartographer** 🟢 - Preset geometry and parameter mapping

### Conflict Resolution Priority
```
h-chip-spec > fixed-point > realtime > morph
```

When agents disagree: **spec compliance always wins**, then bit-depth, then performance.

---

## File Mapping Guide

### DSP Core
| Responsibility | File(s) |
|---|---|
| H-Chip kernel (7 biquads, TDF-II) | `source/DSP/EmuHChipFilter.h` |
| Engine coordinator (interpolation) | `source/DSP/EmuZPlaneCore.{cpp,h}` |
| Preset geometry & pole synthesis | `source/DSP/PresetConverter.{cpp,h}` |
| Static shape bank (8 material paths) | `source/DSP/EMUStaticShapeBank.{cpp,h}` |
| Coefficient tables | `source/DSP/EMUAuthenticTables.h` |
| Interface definitions | `source/DSP/IZPlaneEngine.h`, `source/DSP/ZPlaneCommon.h` |

### Audio Pipeline
| Responsibility | File(s) |
|---|---|
| Main processor (processBlock) | `source/PluginProcessor.{cpp,h}` |
| Material physics (exciter params) | `source/DSP/MaterialPhysics.h` |
| Exciter engines | `source/DSP/ExciterEngine.h` |
| Thread-safe parameter sync | `source/Utility/WaitFreeTripleBuffer.h` |
| Shared state structures | `source/SharedData.h` |

### UI Components
| Responsibility | File(s) |
|---|---|
| Main editor (oil drop renderer) | `source/PluginEditor.{cpp,h}` |
| Style/theming | `source/VesselStyle.h` |
| Constants | `source/HeavyConstants.h` |

### Visualization Math (Phase 3 Prep)
| Responsibility | File(s) |
|---|---|
| Z-Plane visualization helpers | `source/DSP/zplane_visualizer_math.{cpp,h}` |

### Notes
- No separate `AuthenticEMUEngine` file—logic distributed across `EmuHChipFilter` + `EmuZPlaneCore` + `PluginProcessor`
- All real-time DSP occurs in `PluginProcessor::processBlock` audio callback
- 32-sample coefficient update loop in `EmuHChipFilter.h:process_block` (lines 257-277)

---

## Coding Constraints & Discovered Gotchas

### Hard Rules (Violations Block Commits)

1. **NO Floating Point in DSP Loop**
   - DSP loop must use int casting overhead
   - Float calculations allowed ONLY for coefficient preparation
   - Pattern: `int64_t acc = (int64_t)int16_coeff * (int32_t)signal`

2. **NO Standard Libraries**
   - Do not use `juce::dsp::IIR` or similar filters
   - Write TDF-II cascade manually
   - Rationale: Standard libs don't expose inter-stage saturation

3. **Coefficient Truncation Mandatory**
   - All float→int16 conversions must **truncate** (not round)
   - Why: Rounding introduces precision drift that breaks authenticity
   - Pattern: `int16_t coeff = (int16_t)(floatVal * 32768.0f);`

4. **Thread Safety Requirements**
   - **Audio Thread:** Owns int32 filter state, reads coefficients
   - **UI Thread:** Owns float target parameters
   - **Sync Pattern:** NO mutexes on audio thread; use `std::atomic` or SPSC queue
   - All parameter reads must use `.load()` for atomic safety

---

### Discovered Pitfalls (Bugs Fixed During Phase 1-2)

| Issue | Root Cause | Fix | Impact |
|-------|------------|-----|--------|
| **Coefficient Overflow** | b1 = 2*R*cos(θ) exceeds [-1.0, 1.0) at extremes | Scale all coeffs if \|b1\| > 0.99 | Prevented random tone shifts at Quartz mode |
| **Parameter Race Conditions** | Non-atomic APVTS reads | Changed `*ptr` to `ptr->load()` | Eliminated torn reads causing glitches |
| **Denormal CPU Stalls** | Tiny filter states | Added `flush_denormal()` helper | Prevented audio dropouts |
| **Auto-Gain Runaway** | 24dB makeup too hot | Reduced to 12dB + tighter limits | Stable output levels |
| **NaN Propagation** | No infinity check | Added `std::isfinite()` guard | Filter recovers from instability |

**Key Lesson:** At extreme parameters (near-Nyquist frequency + near-unity resonance), even small coefficient errors cause massive frequency/Q shifts.

---

### Performance Targets

- ✅ No allocations in audio callback
- ✅ No mutex locks on audio thread
- ✅ Sequential access in cascade (cache-friendly)
- ⚠️ Unpredictable branch in material check (minor, acceptable)
- 📋 Future: Consider `alignas(64)` for HChipFilter struct (cache line optimization)

---

### Forbidden Patterns

❌ **DO NOT:**
- Add coefficient smoothing/interpolation
- Remove inter-stage saturation
- Use float accumulator
- Round coefficients (must truncate)
- Block audio thread with locks
- Optimize away integer math

✅ **DO:**
- Preserve 32-sample control rate stepping
- Keep inter-stage saturation harsh
- Use Q31/Q15 fixed-point throughout
- Truncate coefficients (authentic quantization)
- Pre-allocate all buffers in `prepareToPlay`

---

## Build Artifacts

**Release Build:**
- VST3: `build_clean/HEAVY_artefacts/Release/VST3/HEAVY.vst3`
- Standalone: `build_clean/HEAVY_artefacts/Release/Standalone/HEAVY.exe`

**Rebuild Command:**
```bash
cmake --build build_clean --config Release
```

---

## Agent System Reference

For detailed agent invocation patterns, conflict resolution, and usage examples:
👉 **See `.claude/agent-roster.md`**

Quick agent roster:
- `h-chip-spec-guardian` - MASTER_SPEC compliance (red)
- `fixed-point-saturation-auditor` - Bit-depth contracts (blue)
- `realtime-audio-guardian` - Performance & threading (yellow)
- `morph-preset-cartographer` - Preset geometry (green)

---

## Project History

**2025-11-24:**
- ✅ Completed Phase 1-2 implementation
- ✅ Created parallel agent verification system
- ✅ Fixed coefficient overflow bug (Quartz mode tone shifts)
- ✅ Fixed parameter race conditions (torn reads)
- ✅ Implemented denormal flushing + NaN recovery
- 🔄 Refactored CLAUDE.md to living spec format

**Next Session Goals:**
- Complete Phase 3 (evaluate OpenGL need)
- Implement Phase 4 verification tests
- Stress test all 8 materials at extreme settings
