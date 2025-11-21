#pragma once
#include <complex>
#include <cmath>
#include <algorithm>
#include <juce_dsp/juce_dsp.h>

namespace zpm
{
    constexpr float kRefFs = 48000.0f;
    constexpr float kPi    = 3.14159265358979323846f;
    constexpr float kTwoPi = 6.28318530717958647692f;

    inline float wrapAngle (float a)
    {
        while (a >  kPi) a -= kTwoPi;
        while (a < -kPi) a += kTwoPi;
        return a;
    }
    inline float interpAngleShortest (float a, float b, float t)
    {
        return a + t * wrapAngle (b - a);
    }

    // Proper bilinear transform: z@48k -> s -> z@fs
    // This preserves formant frequencies correctly across sample rates
    inline std::complex<float> remap48kToFs (std::complex<float> z48, float fs)
    {
        using C = std::complex<float>;
        const C one { 1.0f, 0.0f };

        // Step 1: z48 -> s domain using inverse bilinear transform
        // s = 2*fs_ref * (z - 1) / (z + 1)
        const auto s = 2.0f * kRefFs * (z48 - one) / (z48 + one);

        // Step 2: s -> z@fs using forward bilinear transform
        // z = (2*fs + s) / (2*fs - s)
        const auto zfs = (2.0f * fs + s) / (2.0f * fs - s);

        return zfs;
    }

    // Helper: convert r,theta pair with proper SR mapping
    inline std::pair<float,float> remapPolar48kToFs(float r48, float theta48, float fs)
    {
        if (fs == kRefFs) return {r48, theta48}; // No remapping needed

        const auto z48 = std::polar(r48, theta48);
        const auto zfs = remap48kToFs(z48, fs);

        const float rfs = std::clamp(std::abs(zfs), 0.10f, 0.9995f);
        const float thetafs = std::arg(zfs);

        return {rfs, wrapAngle(thetafs)};
    }

    // Log-space geodesic interpolation for stable morphing
    // Interpolates in log-domain for radius and ensures angle continuity
    inline std::pair<float,float> interpolatePoleLogSpace(
        float rA, float thetaA,
        float rB, float thetaB,
        float t,
        bool unwrapAngle = true)
    {
        // Ensure radii are valid (strictly inside unit circle)
        rA = std::clamp(rA, 0.10f, 0.9995f);
        rB = std::clamp(rB, 0.10f, 0.9995f);

        // Geodesic interpolation in log space (guarantees |p(t)| < 1)
        const float ln_rA = std::log(rA);
        const float ln_rB = std::log(rB);
        const float ln_r = (1.0f - t) * ln_rA + t * ln_rB;
        const float r = std::exp(ln_r);

        // Angle interpolation with optional unwrapping for continuity
        float theta;
        if (unwrapAngle)
        {
            theta = interpAngleShortest(thetaA, thetaB, t);
        }
        else
        {
            theta = thetaA + t * (thetaB - thetaA);
        }

        return {r, wrapAngle(theta)};
    }

    // Block-based pole ramping for efficient coefficient updates
    // Processes entire blocks of samples with coefficient interpolation
    struct BlockPoleRamp
    {
        static constexpr int RAMP_SUBDIVISIONS = 8; // Update coefficients 8 times per block

        struct CoefficientRamp
        {
            float startCoeff, endCoeff;
            float currentCoeff;
            float increment;

            void prepare(float start, float end, int blockSize)
            {
                startCoeff = start;
                endCoeff = end;
                currentCoeff = start;
                increment = (end - start) / static_cast<float>(RAMP_SUBDIVISIONS);
            }

            void advanceSegment() noexcept
            {
                currentCoeff += increment;
                currentCoeff = std::clamp(currentCoeff, startCoeff, endCoeff);
            }
        };

        CoefficientRamp a1Ramp, a2Ramp, b0Ramp;
        int currentSegment = 0;
        int samplesUntilNextUpdate = 0;

        // Initialize block ramp from start to end positions
        inline void prepareBlock(float r0, float theta0, float r1, float theta1, int blockSize)
        {
            // Ensure valid positions
            r0 = std::clamp(r0, 0.10f, 0.9995f);
            r1 = std::clamp(r1, 0.10f, 0.9995f);

            // Calculate start coefficients
            const float a1_0 = -2.0f * r0 * std::cos(theta0);
            const float a2_0 = r0 * r0;
            const float b0_0 = (1.0f - r0) * 0.5f;

            // Calculate end coefficients
            const float a1_1 = -2.0f * r1 * std::cos(theta1);
            const float a2_1 = r1 * r1;
            const float b0_1 = (1.0f - r1) * 0.5f;

            // Prepare coefficient ramps
            a1Ramp.prepare(a1_0, a1_1, blockSize);
            a2Ramp.prepare(a2_0, a2_1, blockSize);
            b0Ramp.prepare(b0_0, b0_1, blockSize);

            currentSegment = 0;
            samplesUntilNextUpdate = blockSize / RAMP_SUBDIVISIONS;
        }

        // Get current coefficients
        inline void getCurrentCoeffs(float& a1, float& a2, float& b0) const noexcept
        {
            a1 = a1Ramp.currentCoeff;
            a2 = a2Ramp.currentCoeff;
            b0 = b0Ramp.currentCoeff;
        }

        // Update coefficients for next segment (call every N samples)
        inline bool updateSegment() noexcept
        {
            if (--samplesUntilNextUpdate <= 0)
            {
                if (currentSegment < RAMP_SUBDIVISIONS - 1)
                {
                    a1Ramp.advanceSegment();
                    a2Ramp.advanceSegment();
                    b0Ramp.advanceSegment();
                    currentSegment++;
                    samplesUntilNextUpdate = 64 / RAMP_SUBDIVISIONS; // Assuming 64 sample blocks
                    return true;
                }
            }
            return false;
        }
    };

    // Q-factor calculation from pole parameters
    inline float computeQ(float r, float theta)
    {
        if (r <= 0.0f || std::abs(theta) < 1e-6f) return 0.0f;
        const float bandwidth = 2.0f * (1.0f - r);
        return std::abs(theta) / (bandwidth + 1e-10f);
    }

    // Section assignment: Sort poles by Q-factor (low to high)
    // This places high-Q poles at the end of the cascade, reducing clipping risk
    template<int N = 6>
    struct SectionAssignment
    {
        struct PoleData
        {
            float r, theta;
            float q;
            int originalIndex;
        };

        // Sort poles by Q-factor and return reordered indices
        static void assignSections(const float* radii, const float* angles, int indices[N])
        {
            PoleData poles[N];

            // Calculate Q for each pole
            for (int i = 0; i < N; ++i)
            {
                poles[i].r = radii[i];
                poles[i].theta = angles[i];
                poles[i].q = computeQ(radii[i], angles[i]);
                poles[i].originalIndex = i;
            }

            // Sort by Q (ascending: low-Q first, high-Q last)
            std::sort(poles, poles + N, [](const PoleData& a, const PoleData& b) {
                return a.q < b.q;
            });

            // Extract sorted indices
            for (int i = 0; i < N; ++i)
            {
                indices[i] = poles[i].originalIndex;
            }
        }

        // In-place reorder of pole data arrays
        static void reorderPoles(float radii[N], float angles[N])
        {
            int indices[N];
            assignSections(radii, angles, indices);

            float tempR[N], tempTh[N];
            for (int i = 0; i < N; ++i)
            {
                tempR[i] = radii[indices[i]];
                tempTh[i] = angles[indices[i]];
            }

            for (int i = 0; i < N; ++i)
            {
                radii[i] = tempR[i];
                angles[i] = tempTh[i];
            }
        }
    };
}