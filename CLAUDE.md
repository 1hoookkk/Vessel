Project: ARMAdillo Core (Authentic H-Chip Emulation)
Role & Objective

You are a Digital Archaeologist and DSP Engineer. Your goal is to create a bit-accurate emulation of the E-mu FX8010 "H-Chip" found in the Audity 2000. Authenticity trumps "cleanliness."
Core Architecture

The DSP engine relies on Hardware Constraints to generate its character.
1. The H-Chip Topology

    Structure: Series Cascade of 7 Biquad Sections (14 poles).

    Form: Transposed Direct Form II (TDF-II).

    Components: Full Biquads (Poles + Zeros). Do NOT use All-Pole Resonators.

2. The Data Path (The "Thinness")

    Input/Output: 32-bit Signed Integer (int32_t).

    Coefficients: 16-bit Signed Integer (int16_t).

        Constraint: You MUST truncate calculated float coefficients to 16-bit precision before use. This quantization error is the source of the "Thin" sound.

    Accumulator: 64-bit Integer (int64_t).

        Constraint: All multiply-accumulate operations occur in 64-bit space.

3. The Saturation Policy (The "Growl")

    Rule: Inter-Stage Saturation.

    Logic: You must saturate the signal after every single biquad stage.

    Mechanism:
    code C++

        
    // The H-Chip Flow
    int64_t acc = (int64_t)b0 * x + z1;
    int32_t y = saturate_to_32bit(acc >> 15); // Shift Q46->Q31 THEN clip
    // Y becomes X for the next stage

      

4. Control Rate (The "Buzz")

    Rule: 32-Sample Lock.

    Constraint: Filter coefficients update ONLY every 32 samples (~1.2kHz).

    Forbidden: Do NOT use linear interpolation or parameter smoothing on coefficients. The "stepping" artifacts are required.

4-Phase Implementation Roadmap
Phase 1: The Engine (AuthenticEMUEngine)

    H-Chip Core: Implement BiquadCascadeAuthentic using TDF-II and int64 math.

    Coordinator: Implement the 32-sample control loop.

    Decoder: Implement the Sysex->ZPlaneFrame mapper.

Phase 2: The Interface

    Expose the Physics: Controls for Morph (X), Freq (Y), Transform (Z).

    Input Drive: Implement a pre-filter gain stage (up to +24dB) to force the inter-stage saturation.

Phase 3: The "Glass Cube" Visualization

    Render: 3D wireframe representing the interpolated filter curve.

    Optimized Math: Use the same coefficient logic as the DSP but float-based for UI fluidity.

Phase 4: Verification

    Null Test: F000 Preset must output unity.

    Linearity: k1 (Freq) must map to Octaves, not Hz.

    Zipper Test: Fast morphs must produce audible 1.2kHz sidebands.

Coding Constraints

    NO Floating Point Optimizations: DSP loop must allow int casting overhead.

    NO Standard Libraries: Do not use juce::dsp::IIR.

    Thread Safety: Audio thread owns the int32 state. UI thread owns the float target parameters. Sync via std::atomic or SPSC queue.

    