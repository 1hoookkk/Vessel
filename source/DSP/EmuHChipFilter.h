#pragma once

#include <cstdint>
#include <algorithm>
#include <cmath>
#include "EmuZPlaneCore.h" // For ZPlaneFrame, ARMAdilloEngine

/**
 * E-MU FX8010 "H-CHIP" EMULATION KERNEL
 * TARGET: AUDITY 2000 / PROTEUS 2000 FILTER ARCHITECTURE
 * 
 * ARCHAEOLOGICAL NOTES:
 * - Signal Path: 32-bit Fixed Point (Q31)
 * - Accumulator: 64-bit (Simulating H-Chip 67-bit acc)
 * - Coeffs: 16-bit Fixed Point (Q15) - TRUNCATED
 * - Topology: 7-Stage Cascade Transposed Direct Form II
 * - Artifacts: 32-sample control step (zipper), Inter-stage saturation (growl)
 */

// -- 1. DATA STRUCTURES --

// State variables for TDF-II
struct BiquadState {
    int32_t z1; // Q31
    int32_t z2; // Q31
};

// Coefficients in Q15 format (1 sign, 0 int, 15 frac)
struct HChipCoeffs { // Renamed to avoid conflict if BiquadCoeffs is in EmuZPlaneCore
    int16_t b0;
    int16_t b1;
    int16_t b2;
    int16_t a1;
    int16_t a2;
};

// Main Filter Class State
struct HChipFilter {
    static const int NUM_STAGES = 7;
    
    BiquadState states[NUM_STAGES];
    HChipCoeffs coeffs[NUM_STAGES];
    
    // Control Rate Counter
    int32_t controlCounter; // Counts 0..31
    
    HChipFilter() {
        controlCounter = 0;
        for(int i=0; i<NUM_STAGES; i++) {
            states[i] = {0, 0};
            coeffs[i] = {0, 0, 0, 0, 0};
        }
    }
};

// -- 2. HELPER FUNCTIONS (THE "GRIT" GENERATORS) --

// CONVERSION: Float (-1.0 to 1.0) -> Int32 (Q31)
inline int32_t float_to_q31(float x) {
    if (x > 1.0f) x = 1.0f;
    if (x < -1.0f) x = -1.0f;
    return (int32_t)(x * 2147483647.0f);
}

// CONVERSION: Int32 (Q31) -> Float
inline float q31_to_float(int32_t x) {
    return (float)x * (1.0f / 2147483648.0f);
}

// ARTIFACT GENERATOR: Float -> Int16 (Q15) Truncation
// HALF-SCALE TRICK: Biquad coeffs can go to ±1.9, but Q15 only stores ±1.0
// We store 0.5x the value, then shift 1 less bit during MAC to compensate
inline int16_t float_to_q15_truncated(float x) {
    float scaled = (x * 0.5f) * 32768.0f; // Scale by 0.5 to fit ±1.9 into ±1.0 range
    if (scaled > 32767.0f) scaled = 32767.0f;
    if (scaled < -32768.0f) scaled = -32768.0f;
    return (int16_t)scaled;
}

// SATURATION: The "Growl"
inline int32_t hard_clip_q31(int64_t x) {
    if (x > 2147483647LL) return 2147483647;
    if (x < -2147483648LL) return -2147483648;
    return (int32_t)x;
}

// -- 3. COEFFICIENT CALCULATION --
// CONNECTS THE "BRAIN" (ARMAdillo) TO THE "BODY" (H-Chip)
inline void update_coefficients(HChipFilter* filter, const ZPlaneFrame& frame, float sampleRate) {
    
    // We loop through the 7 sections coming from your Z-Plane Morph
    for (int s = 0; s < HChipFilter::NUM_STAGES; ++s) {
        
        // 1. Ask ARMAdillo to calculate the High-Res Float Coefficients
        auto floatCoeffs = ARMAdilloEngine::decode_section(frame.sections[s], sampleRate);

        // 2. THE "GRIT" STEP: Truncate High-Res Floats to 16-bit Hardware Coeffs
        // This is where the "Thinness" happens.
        // Note: ARMAdillo returns 'emu::BiquadCoeffs' (or similar) with Q31 or float?
        // Let's check ARMAdilloEngine::decode_section return type.
        // In EmuZPlaneCore.h, it returns BiquadCoeffs which has dsp_sample_t (int32).
        // Wait, the existing ARMAdilloEngine::decode_section returns int32 fixed point BiquadCoeffs.
        // The user's snippet assumes it returns floats or we need to convert.
        // "auto floatCoeffs = ARMAdilloEngine::decode_section..."
        // The user's snippet says "Truncate High-Res Floats to 16-bit Hardware Coeffs".
        
        // IF ARMAdillo returns int32, we should scale/truncate to int16.
        // In EmuZPlaneCore.h, BiquadCoeffs members are dsp_sample_t (int32).
        // They are Q31.
        // To convert Q31 (int32) to Q15 (int16), we shift right by 16.
        
        // BUT the user snippet logic shows: `float_to_q15_truncated(floatCoeffs.b0)`.
        // This implies `floatCoeffs` members are floats.
        // I might need to update ARMAdilloEngine to return floats, OR adapt here.
        // The prompt says "Swap out the 'Basic Lowpass' logic for the 'ARMAdillo Decoder' logic shown above."
        // And the "above" logic uses `float_to_q15_truncated`.
        
        // Let's look at `EmuZPlaneCore.h` again. `decode_section` returns `BiquadCoeffs` (int32).
        // I should probably cast the int32 Q31 back to float to follow the user's snippet strictly, 
        // OR (better) just shift the bits if they are already fixed point.
        // However, the user snippet explicitly wants "Truncation" behavior of float->int16.
        // If I have Q31, `x >> 16` is truncation.
        
        // However, I should check if `ARMAdilloEngine` output is indeed what I think it is.
        // `EmuZPlaneCore.cpp` shows `decode_section` converting `float_to_fixed`.
        
        // I'll assume for this file I should adapt to what `ARMAdilloEngine` gives me.
        // `floatCoeffs` is `emu::BiquadCoeffs` (Q31).
        // So I'll convert Q31 to float first to use the user's helper, or just shift.
        // `fixed_to_float` is available in `EmuZPlaneCore.h`.
        
        // Actually, looking at the user's snippet again:
        // `filter->coeffs[s].b0 = float_to_q15_truncated(floatCoeffs.b0);`
        // This strongly suggests `floatCoeffs` has float members.
        
        // To be safe and follow instructions "Swap out ... logic shown above":
        // I will use `fixed_to_float` on the result of `decode_section` if it returns fixed point.
        
        // Using fully qualified access just in case.
        auto fixedCoeffs = ARMAdilloEngine::decode_section(frame.sections[s], sampleRate);
        
        // Convert to float (recovering the high res value) then re-truncate to Q15 per instructions
        // (This simulates the specific artifact pipeline requested)
        filter->coeffs[s].b0 = float_to_q15_truncated(fixed_to_float(fixedCoeffs.b1)); // Note mapping: b0 in snippet vs b1 in Emu?
        // Wait, EmuZPlaneCore.h `BiquadCoeffs` has `b1, b2, a0, a1, a2`.
        // The user snippet `BiquadCoeffs` struct has `b0, b1, b2, a1, a2`.
        // Standard biquad: y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2.
        // Emu H-Chip often uses a0 as input gain? Or is it TDF-II?
        // User snippet TDF-II: y = b0*x + z1 ...
        // EmuZPlaneCore `process_biquad`: `acc += x * c.a0 ... + y1 * c.b1 ...`
        // There is a naming mismatch.
        // EmuZPlaneCore `BiquadCoeffs`:
        // b1 (Feedback 1), b2 (Feedback 2), a0, a1, a2 (Feedforward).
        // User Snippet `HChipFilter`:
        // b0, b1, b2 (Feedforward?), a1, a2 (Feedback?)
        
        // In user snippet `process_block`:
        // Term: b0 * x[n] -> Feedforward
        // Equation 2: z1 = b1*x - a1*y + z2 -> b1 is Feedforward, a1 is Feedback
        // Equation 3: z2 = b2*x - a2*y -> b2 is Feedforward, a2 is Feedback
        
        // So User Snippet: b0, b1, b2 are Feedforward. a1, a2 are Feedback.
        
        // In `EmuZPlaneCore`:
        // `process_biquad`:
        // acc += x * c.a0 (Feedforward 0)
        // acc += x1 * c.a1 (Feedforward 1)
        // acc += x2 * c.a2 (Feedforward 2)
        // acc += y1 * c.b1 (Feedback 1)
        // acc += y2 * c.b2 (Feedback 2)
        
        // So Mapping:
        // User b0 -> Emu a0
        // User b1 -> Emu a1
        // User b2 -> Emu a2
        // User a1 -> Emu -b1 (Note sign convention)
        // User a2 -> Emu -b2
        
        // User snippet: `z1 = b1*x - a1*y`. Emu: `acc += y1 * c.b1`.
        // If Emu `b1` is positive coeff in sum, and loop is `y[n] = ... + y[n-1]*b1`, then `y[n] - b1*y[n-1] = ...` -> pole at b1.
        // User snippet: `y_clipped` is output. `z1 = ... - a1*y`.
        // So user `a1` is subtracted.
        
        // Let's assume `ARMAdilloEngine::decode_section` produces Emu-compatible coeffs (feedforward a, feedback b).
        // I need to map them to the `HChipCoeffs` struct of the User Snippet.
        
        // Emu `a0` -> User `b0`
        // Emu `a1` -> User `b1`
        // Emu `a2` -> User `b2`
        // Emu `b1` -> User `a1` (Negated?)
        // Emu `b2` -> User `a2` (Negated?)
        
        // Let's check `decode_section` in `EmuZPlaneCore.cpp`.
        // c.a0 = float_to_fixed(a0_real) (Feedforward)
        // c.b1 = float_to_fixed(b1_real) (Feedback)
        // b1_real = 2 * R * cos(theta).
        // Standard difference equation: y[n] = x[n] + b1*y[n-1] + b2*y[n-2] (if b1, b2 are on right side)
        // Transfer function H(z) = 1 / (1 - b1 z^-1 - b2 z^-2).
        // Denominator 1 - b1...
        // User snippet: z1 = ... - a1*y.
        // Implies denominator 1 + a1 z^-1 ...
        // So User `a1` = - Emu `b1`.
        
        // Wait, `b1_real` in `decode_section` is `2*R*cos`. This is usually positive for low freq.
        // If Transfer function is 1 / (1 - 2Rcos z^-1 ...), then coeff is +2Rcos.
        // User snippet subtracts `a1`. So `a1` should be `-2Rcos` to match `+` in TF?
        // Or if user `a1` is positive, then `-a1` is negative.
        // Let's look at User Snippet `update_coefficients` (the deleted part):
        // `a1_f = -2.0f * cosW`.
        // `filter->coeffs[i].a1 = ... a1_f ...`
        // So User `a1` is typically negative (-2cos).
        // Snippet uses `- a1*y`. So `- (-2cos) * y` = `+ 2cos * y`.
        // This matches `+ b1 * y` where `b1 = 2cos`.
        
        // So User `a1` should be `- Emu b1`.
        
        // Let's verify Emu `b2`. `b2_real = -(R*R)`. Negative.
        // User snippet `a2_f = 1.0f - alpha` (Wait, that was Lowpass example).
        // Let's check `EmuZPlaneCore` `decode_section` again.
        // `b2_real = -R*R`.
        // User snippet `z2 = ... - a2*y`.
        // To get `- R^2 * y`, `a2` must be `+ R^2`.
        // So User `a2` = `- Emu b2`.
        
        // MAPPING:
        // User b0 = Emu a0
        // User b1 = Emu a1
        // User b2 = Emu a2
        // User a1 = - Emu b1
        // User a2 = - Emu b2
        
        // But I should check `EmuHChipFilter::process_biquad` in `EmuZPlaneCore.h` to see how Emu coeffs are used.
        // `acc += s.y1 * c.b1;`
        // It ADDS `b1 * y1`.
        // User snippet SUBTRACTS `a1 * y`.
        // So `a1` must be `-b1`.
        
        filter->coeffs[s].b0 = float_to_q15_truncated(fixed_to_float(fixedCoeffs.a0));
        filter->coeffs[s].b1 = float_to_q15_truncated(fixed_to_float(fixedCoeffs.a1));
        filter->coeffs[s].b2 = float_to_q15_truncated(fixed_to_float(fixedCoeffs.a2));
        filter->coeffs[s].a1 = float_to_q15_truncated(-fixed_to_float(fixedCoeffs.b1));
        filter->coeffs[s].a2 = float_to_q15_truncated(-fixed_to_float(fixedCoeffs.b2));
    }
}

// -- 4. DSP KERNEL (THE SOURCE OF TRUTH) --

// Add 'targetFrame' and 'sampleRate' inputs
inline void process_block(HChipFilter* filter, float* data, int numSamples, const ZPlaneFrame& targetFrame, float sampleRate) {
    
    for (int n = 0; n < numSamples; ++n) {
        
        // A. CONTROL RATE ARTIFACTS (Zipper Noise)
        // ---------------------------------------------------------
        // Logic: Only update coefficients every 32 samples.
        // This creates 1.2kHz stepping sidebands.
        if (filter->controlCounter == 0) {
             update_coefficients(filter, targetFrame, sampleRate);
        }
        
        // Modulo 32 counter
        filter->controlCounter = (filter->controlCounter + 1) & 31;


        // B. INPUT CONVERSION
        // ---------------------------------------------------------
        // Enter the fixed point domain.
        int32_t signal = float_to_q31(data[n]);


        // C. 7-STAGE CASCADE LOOP
        // ---------------------------------------------------------
        for (int s = 0; s < HChipFilter::NUM_STAGES; ++s) {
            
            // 1. Load Coefficients & State
            HChipCoeffs* c = &filter->coeffs[s];
            BiquadState* st = &filter->states[s];

            // 2. ARITHMETIC LOGIC (H-Chip Emulation)
            // TDF-II Equation 1: y[n] = b0*x[n] + z1[n-1]
            // Math: Q15 * Q31 = Q46. Need int64 accumulator.

            int64_t acc;

            // Term: b0 * x[n]
            acc = (int64_t)c->b0 * (int64_t)signal;

            // Add State z1 (Shifted to match Q46 accumulator alignment)
            // Note: State is stored in Q31. To add to Q46 accumulator,
            // we conceptually treat state as already aligned or shift the product down.
            // HALF-SCALE COMPENSATION: Coeffs are stored at 0.5x, so shift by 14 instead of 15
            // This gives us 2x gain to restore the original magnitude

            int32_t term_b0 = (int32_t)(acc >> 14); // Bit Shift Right 14 (was 15)

            // y_raw is the potential output before clipping
            int64_t y_raw = (int64_t)term_b0 + (int64_t)st->z1;

            // 3. INTER-STAGE SATURATION (The "Growl")
            // Logic: Hard clip internal node 'y'.
            // If Resonance is high, this node clips, creating odd harmonics.
            int32_t y_clipped = hard_clip_q31(y_raw);

            // 4. STATE UPDATES (TDF-II)

            // Equation 2: z1[n] = b1*x[n] - a1*y[n] + z2[n-1]
            // We use y_clipped for feedback math to ensure the "Distorted Formant" behavior

            int64_t term_b1 = ((int64_t)c->b1 * (int64_t)signal) >> 14;
            int64_t term_a1 = ((int64_t)c->a1 * (int64_t)y_clipped) >> 14;

            // Update z1
            st->z1 = hard_clip_q31(term_b1 - term_a1 + st->z2);

            // Equation 3: z2[n] = b2*x[n] - a2*y[n]

            int64_t term_b2 = ((int64_t)c->b2 * (int64_t)signal) >> 14;
            int64_t term_a2 = ((int64_t)c->a2 * (int64_t)y_clipped) >> 14;
            
            // Update z2
            st->z2 = hard_clip_q31(term_b2 - term_a2);

            // 5. PROPAGATION
            // The output of this stage is the clipped signal.
            // This becomes the 'x' (signal) for the next stage.
            signal = y_clipped;
        }


        // D. OUTPUT CONVERSION
        // ---------------------------------------------------------
        // Exit fixed point domain.
        data[n] = q31_to_float(signal);
    }
}
