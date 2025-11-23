# ARMAdillo Core Agent Roster
**Version:** 1.0
**Last Updated:** 2025-11-24
**Project:** HEAVY Plugin (H-Chip Emulation)

---

## Core Philosophy: Parallel-First Auditing

Agents are **concern-based** (not file-based). Each owns ONE orthogonal verification dimension.
When DSP code changes, invoke **all relevant agents in parallel** for fastest validation.

---

## Tier 1: Core Compliance Triad (Always Parallel)

These three agents form the **mandatory audit wave** for any DSP implementation:

### 1. `h-chip-spec-guardian` 🔴
**Concern:** Architectural compliance with H-Chip MASTER_SPEC
**Scope:**
- 7-biquad topology verification (TDF-II, not DF-I)
- 32-sample control rate enforcement
- Full biquads (poles + zeros), not all-pole resonators
- Series cascade structure

**Files:** `EmuZPlaneCore.*`, `PresetConverter.*`, `EMUAuthenticTables.h`
**Veto Power:** YES - Spec violations block everything
**Parallel with:** fixed-point, realtime, morph

**Trigger Examples:**
- New biquad stage implementation
- Topology refactoring
- Control-rate update changes
- "Does this match H-Chip architecture?"

---

### 2. `fixed-point-saturation-auditor` 🔵
**Concern:** Bit-depth contracts and saturation topology
**Scope:**
- int32 (Q31) signal path verification
- int16 (Q15) coefficient truncation
- int64 accumulator enforcement
- Inter-stage saturation placement (after ALL 7 stages)

**Files:** All DSP implementation files
**Veto Power:** YES - Math violations break authenticity
**Parallel with:** h-chip-spec, realtime, morph

**Trigger Examples:**
- Coefficient calculation changes
- New saturation logic
- Accumulator width modifications
- "Is the fixed-point math correct?"

---

### 3. `realtime-audio-guardian` 🟡
**Concern:** Real-time safety, performance, thread safety
**Scope:**
- Memory allocation detection (processBlock must be allocation-free)
- Mutex/lock detection (audio thread must be lock-free)
- Cache efficiency patterns
- Buffer layout optimization

**Files:** `PluginProcessor.cpp`, `processBlock()`, audio thread hot paths
**Veto Power:** NO - Performance is optimization priority, not blocking
**Parallel with:** h-chip-spec, fixed-point, morph

**Trigger Examples:**
- processBlock() modifications
- Buffer management changes
- Performance optimization attempts
- "Is this real-time safe?"

---

## Tier 2: Domain Specialists (Conditional Parallel)

### 4. `morph-preset-cartographer` 🟢
**Concern:** Preset geometry, 3D cube mapping, UI parameter flow
**Scope:**
- Shape-to-cube mapping pipeline
- 7th section pole synthesis
- X/Y/Z axis semantic correctness
- UI macro → k1/k2 translation

**Files:** `PresetConverter.*`, `EMUStaticShapeBank.*`, `ZPlaneCube/Frame/Section`
**Veto Power:** NO - Geometric integrity check
**Parallel with:** h-chip-spec, fixed-point, realtime

**Trigger Examples:**
- Preset bank modifications
- Trilinear interpolation changes
- UI parameter mapping updates
- "Does morphing work correctly?"

---

## Invocation Patterns

### Pattern A: DSP Implementation (Most Common)
**Trigger:** Modified `EmuZPlaneCore.cpp`, `EmuHChipFilter.h`, saturation logic

```
PARALLEL WAVE:
├─ h-chip-spec-guardian        (topology check)
├─ fixed-point-saturation-auditor  (math check)
└─ realtime-audio-guardian     (performance check)

CONFLICT RESOLUTION:
1. h-chip-spec > fixed-point > realtime
2. If realtime suggests SIMD but fixed-point warns of precision loss → fixed-point wins
3. All findings must reconcile before committing
```

---

### Pattern B: Preset/Morphing Work
**Trigger:** Modified `PresetConverter.*`, `EMUStaticShapeBank.*`

```
PARALLEL WAVE:
├─ morph-preset-cartographer   (primary: geometry)
├─ h-chip-spec-guardian        (verify coefficient generation)
└─ fixed-point-saturation-auditor (verify int16 truncation)

CONFLICT RESOLUTION:
1. h-chip-spec > morph > fixed-point
2. If morph finds mapping issue but h-chip-spec says spec is correct → h-chip-spec wins
```

---

### Pattern C: Performance Optimization
**Trigger:** Hot path optimization, SIMD, buffer layout changes

```
PARALLEL WAVE:
├─ realtime-audio-guardian     (primary: performance)
├─ fixed-point-saturation-auditor (ensure no float creep)
└─ h-chip-spec-guardian        (verify spec maintained)

CONFLICT RESOLUTION:
1. h-chip-spec > fixed-point > realtime
2. Optimizations CANNOT break authenticity
3. If SIMD/vectorization risks precision → reject optimization
```

---

### Pattern D: Bulk Refactoring (Nuclear Option)
**Trigger:** Major DSP refactoring across multiple files

```
PARALLEL WAVE (ALL AGENTS):
├─ h-chip-spec-guardian
├─ fixed-point-saturation-auditor
├─ realtime-audio-guardian
└─ morph-preset-cartographer

SEQUENTIAL REVIEW:
1. ALL agents must pass
2. h-chip-spec findings take absolute priority
3. Cross-validate findings (e.g., realtime vs fixed-point conflicts)
4. Generate unified fix plan
```

---

## Conflict Resolution Matrix

| Scenario | Winner | Rationale |
|----------|--------|-----------|
| realtime wants SIMD, fixed-point warns precision loss | **fixed-point** | Authenticity > performance |
| morph finds mapping issue, h-chip-spec says spec OK | **h-chip-spec** | Spec is ground truth |
| All pass, but output sounds wrong | **Escalate** | Human ear test required |
| fixed-point passes, h-chip-spec fails topology | **h-chip-spec** | Math is meaningless if topology is wrong |

---

## Usage Examples

### Example 1: Single DSP File Change
```
User: "I just implemented the 7-biquad cascade in EmuHChipFilter.h with inter-stage saturation"
Assistant: "Run Pattern A: DSP Implementation in parallel:"

**Parallel Invocation:**
- h-chip-spec-guardian (verify 7-section TDF-II topology)
- fixed-point-saturation-auditor (audit int32→int64→int32 path + saturation)
- realtime-audio-guardian (check for allocations/locks)
```

---

## Quick Reference: "Which Pattern?"

| Your Change | Pattern | Agents (Parallel) |
|-------------|---------|-------------------|
| New biquad implementation | Pattern A | spec + fixed-point + realtime |
| Coefficient calculation | Pattern A | spec + fixed-point + realtime |
| Saturation logic | Pattern A | spec + fixed-point + realtime |
| Preset bank update | Pattern B | morph + spec + fixed-point |
| UI parameter mapping | Pattern B | morph + spec |
| SIMD/optimization | Pattern C | realtime + fixed-point + spec |
| Major refactor | Pattern D | ALL AGENTS |

---

## Agent Status

- ✅ **h-chip-spec-guardian**: Production ready
- ✅ **fixed-point-saturation-auditor**: Production ready
- ✅ **realtime-audio-guardian**: Production ready
- ✅ **morph-preset-cartographer**: Production ready
