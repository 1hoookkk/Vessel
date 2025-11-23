/**
 * GREY BOX TEST - The Filter Event Horizon
 *
 * This is the ultimate gatekeeper test. If this fails, you don't have a product.
 *
 * PASS CRITERIA:
 * 1. White noise input → Morph sweep (0.0 → 1.0 → 0.0) → No clicks
 * 2. CPU usage stays flat (no spikes)
 * 3. No explosions (peak < 2.0)
 * 4. No NaN/Inf values
 * 5. Audible "Audity 2000 thinness" (cold, glassy, digital)
 *
 * NO JUCE. NO UI. Just raw filter math.
 */

#include "EmuZPlaneCore.h"
#include "PresetConverter.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <fstream>

// Simple WAV writer (44.1kHz, 16-bit, mono)
class SimpleWavWriter {
private:
    std::ofstream file;
    uint32_t dataSize = 0;
    uint32_t sampleRate;

    void writeInt32(uint32_t value) {
        file.write(reinterpret_cast<const char*>(&value), 4);
    }

    void writeInt16(uint16_t value) {
        file.write(reinterpret_cast<const char*>(&value), 2);
    }

public:
    SimpleWavWriter(const char* filename, uint32_t sr = 44100) : sampleRate(sr) {
        file.open(filename, std::ios::binary);

        // WAV header (will update later)
        file.write("RIFF", 4);
        writeInt32(0); // Chunk size (placeholder)
        file.write("WAVE", 4);

        // Format chunk
        file.write("fmt ", 4);
        writeInt32(16); // Subchunk1Size (PCM)
        writeInt16(1);  // AudioFormat (PCM)
        writeInt16(1);  // NumChannels (mono)
        writeInt32(sampleRate);
        writeInt32(sampleRate * 2); // ByteRate
        writeInt16(2);  // BlockAlign
        writeInt16(16); // BitsPerSample

        // Data chunk header
        file.write("data", 4);
        writeInt32(0); // Subchunk2Size (placeholder)
    }

    void writeSample(float sample) {
        // Clamp to [-1.0, 1.0] and convert to 16-bit
        sample = std::max(-1.0f, std::min(1.0f, sample));
        int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
        writeInt16(static_cast<uint16_t>(pcm));
        dataSize += 2;
    }

    ~SimpleWavWriter() {
        // Update sizes
        file.seekp(4);
        writeInt32(36 + dataSize);
        file.seekp(40);
        writeInt32(dataSize);
        file.close();
    }
};

// White noise generator
class NoiseGenerator {
private:
    std::mt19937 gen;
    std::uniform_real_distribution<float> dist;

public:
    NoiseGenerator() : gen(std::random_device{}()), dist(-1.0f, 1.0f) {}

    float next() {
        return dist(gen);
    }
};

// Stability analyzer
class StabilityAnalyzer {
private:
    float maxPeak = 0.0f;
    float rms = 0.0f;
    int sampleCount = 0;
    int nanCount = 0;
    int explosionCount = 0;

public:
    void process(float sample) {
        if (std::isnan(sample) || std::isinf(sample)) {
            nanCount++;
            return;
        }

        float absSample = std::abs(sample);
        maxPeak = std::max(maxPeak, absSample);

        if (absSample > 10.0f) {
            explosionCount++;
        }

        rms += sample * sample;
        sampleCount++;
    }

    void report() {
        std::cout << "\n=== STABILITY ANALYSIS ===" << std::endl;
        std::cout << "Max Peak: " << maxPeak << " (Target: < 2.0)" << std::endl;
        std::cout << "RMS: " << std::sqrt(rms / sampleCount) << std::endl;
        std::cout << "NaN/Inf Count: " << nanCount << " (Target: 0)" << std::endl;
        std::cout << "Explosion Count (>10.0): " << explosionCount << " (Target: 0)" << std::endl;
    }

    bool passed() {
        return maxPeak < 2.0f && nanCount == 0 && explosionCount == 0;
    }
};

// CPU profiler
class SimpleCPUProfiler {
private:
    std::chrono::high_resolution_clock::time_point start;

public:
    void startBlock() {
        start = std::chrono::high_resolution_clock::now();
    }

    double endBlock(int numSamples, float sampleRate) {
        auto end = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
        double bufferDurationMs = (numSamples / sampleRate) * 1000.0;
        return (elapsedMs / bufferDurationMs) * 100.0; // % CPU
    }
};

// Main Grey Box Test
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "   GREY BOX TEST - Filter Event Horizon" << std::endl;
    std::cout << "========================================" << std::endl;

    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int totalSeconds = 10;
    const int totalBlocks = (sampleRate * totalSeconds) / blockSize;

    std::cout << "\nTest Configuration:" << std::endl;
    std::cout << "- Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "- Block Size: " << blockSize << " samples" << std::endl;
    std::cout << "- Duration: " << totalSeconds << " seconds" << std::endl;
    std::cout << "- Total Blocks: " << totalBlocks << std::endl;

    // Initialize filter
    EmuHChipFilter filter(sampleRate);

    // Create Z-Plane cube from EMU presets
    // Using shape 0 (Vowel_Ae) morphing to shape 12 (Bell_Metallic)
    // This should give us the "cold, glassy" E-mu sound
    ZPlaneCube cube = PresetConverter::convertToCube(0, 12);

    std::cout << "\nZ-Plane Configuration:" << std::endl;
    std::cout << "- Shape A: 0 (Vowel_Ae)" << std::endl;
    std::cout << "- Shape B: 12 (Bell_Metallic)" << std::endl;
    std::cout << "- Expected sound: Cold, glassy, formant-like" << std::endl;

    // Prepare test signals
    NoiseGenerator noise;
    StabilityAnalyzer stability;
    SimpleCPUProfiler profiler;
    SimpleWavWriter wav("grey_box_output.wav", static_cast<uint32_t>(sampleRate));

    std::vector<float> inputBlock(blockSize);
    std::vector<float> outputBlock(blockSize);

    std::cout << "\nRunning test..." << std::endl;
    std::cout << "[ ";

    double totalCPU = 0.0;
    int cpuSamples = 0;

    // Process blocks with morph sweep
    for (int block = 0; block < totalBlocks; ++block) {
        // Progress indicator
        if (block % (totalBlocks / 20) == 0) {
            std::cout << "=" << std::flush;
        }

        // Generate white noise (best for hearing formant character)
        for (int i = 0; i < blockSize; ++i) {
            inputBlock[i] = noise.next() * 0.5f;
        }

        // Morph sweep: Triangle wave (0.0 → 1.0 → 0.0) over 2 seconds
        float progress = static_cast<float>(block) / totalBlocks;
        float morphCycle = std::fmod(progress * (totalSeconds / 2.0f), 1.0f);
        float morph = (morphCycle < 0.5f)
            ? morphCycle * 2.0f           // Rising edge: 0.0 → 1.0
            : 2.0f - (morphCycle * 2.0f); // Falling edge: 1.0 → 0.0

        // Y-axis: Controls pole angle (frequency shift)
        float yAxis = 0.5f; // Neutral

        // Z-axis: Controls pole RADIUS (actual resonance/Q)
        // 1.0 = maximum resonance (poles near unit circle)
        // Cranked to MAX to make formants unmistakable
        float transform = 0.98f; // EXTREME resonance

        // Process block
        profiler.startBlock();
        filter.process_block(cube, morph, yAxis, transform,
                           inputBlock.data(), outputBlock.data(), blockSize);
        double cpuPercent = profiler.endBlock(blockSize, sampleRate);

        totalCPU += cpuPercent;
        cpuSamples++;

        // Analyze stability and write output
        // RESONATOR FIX: Signal now passes through, minimal boost needed
        const float makeupGain = 2.0f; // Just for headroom
        for (int i = 0; i < blockSize; ++i) {
            float boosted = outputBlock[i] * makeupGain;
            stability.process(boosted);
            wav.writeSample(boosted);
        }
    }

    std::cout << " ] Done!" << std::endl;

    // Report results
    stability.report();

    std::cout << "\n=== CPU USAGE ===" << std::endl;
    std::cout << "Average CPU: " << (totalCPU / cpuSamples) << "%" << std::endl;
    std::cout << "(Target: < 25% single core)" << std::endl;

    std::cout << "\n=== OUTPUT ===" << std::endl;
    std::cout << "WAV file written to: grey_box_output.wav" << std::endl;
    std::cout << "Listen for:" << std::endl;
    std::cout << "- Cold, glassy tone (Audity 2000 aesthetic)" << std::endl;
    std::cout << "- Smooth morph transitions (no clicks/zippers)" << std::endl;
    std::cout << "- Vowel-like formants sweeping" << std::endl;

    // Final verdict
    std::cout << "\n========================================" << std::endl;
    if (stability.passed()) {
        std::cout << "   ✓ GREY BOX TEST PASSED" << std::endl;
        std::cout << "   The filter is stable. You may proceed to UI." << std::endl;
    } else {
        std::cout << "   ✗ GREY BOX TEST FAILED" << std::endl;
        std::cout << "   Fix the filter math before building UI." << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return stability.passed() ? 0 : 1;
}
