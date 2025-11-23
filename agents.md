# agents.md — Unified Agent Playbook

## Purpose
Single-source guide for running the VESSEL verification agents. Blends the hardware-spec focus from `CLAUDE.md` with the execution discipline from `GEMINI.md`, and aligns with `.claude/agent-roster.md`.

## Sources of Truth
- **MASTER_SPEC:** `CLAUDE.md` (H-Chip topology, fixed-point contracts, 32-sample control rate, inter-stage saturation, coefficient truncation).
- **Execution Protocol:** `GEMINI.md` (real-time safety, “ship code” cadence, one-line intent + tight plans).
- **Roster Details:** `.claude/agent-roster.md` (roles, triggers, conflict priorities).

## Agent Roster & Mandates
- **h-chip-spec-guardian** (veto) — Enforce 7-stage TDF-II cascade, full biquads, 32-sample coefficient lock, topology compliance. Spec is ground truth.
- **fixed-point-saturation-auditor** (veto) — Q31 signal path, Q15 coeff truncation (no rounding), int64 accumulators, inter-stage saturation after each biquad.
- **realtime-audio-guardian** (advisory) — No allocations/locks/disk/UI on audio thread; cache-friendly hot paths; atomics or lock-free queues only.
- **morph-preset-cartographer** (advisory) — Preset geometry, Z-plane cube mapping, macro → k-space translation, shape bank integrity.

**Conflict priority:** h-chip-spec > fixed-point > realtime > morph. Authenticity beats performance; performance beats convenience.

## Invocation Patterns (run agents in parallel per pattern)
- **Pattern A — DSP implementation:** Changes to `EmuHChipFilter`, `EmuZPlaneCore`, saturation or control-rate logic. Agents: spec + fixed-point + realtime.
- **Pattern B — Presets/morphing:** `PresetConverter`, `EMUStaticShapeBank`, cube mapping. Agents: morph + spec + fixed-point.
- **Pattern C — Performance:** SIMD/hot-path tweaks, buffer layout. Agents: realtime + fixed-point + spec.
- **Pattern D — Major refactor:** Cross-file DSP refactors. Agents: ALL. Everyone must pass; resolve using conflict priority.

## Operating Rules (must-haves before agents sign off)
- Signal path: int32 I/O, int64 MAC, int16 coeffs (truncated), inter-stage saturation every biquad.
- Control rate: coefficients update every 32 samples; no smoothing/interpolation.
- Real-time safety: zero allocations/locks in `processBlock`; sanitize NaN/Inf; keep denormals flushed; UI stays off the audio thread.
- Morphing: ARMAdillo k-space interpolation via 8-corner cube; do not morph raw biquad coeffs.

## Execution Protocol (use GEMINI “Executive Mode” tone)
1. **Intent:** One line on what/why the change is.
2. **Plan:** 3–5 bullets, outcome-focused (which agents/pattern, what checks).
3. **Patch:** Surgical diff; keep DSP math integer and spec-faithful.
4. **Validate:** State which agents ran, their findings, and any follow-up (tests pending or passed).

## Quick Reference
- If topology changes even slightly → Pattern A.
- If presets/cube mapping change → Pattern B.
- If performance/SIMD is touched → Pattern C.
- If more than one of the above at once → Pattern D.

