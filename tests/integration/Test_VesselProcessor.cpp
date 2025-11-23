#include <catch2/catch_test_macros.hpp>
#include "PluginProcessor.h"

TEST_CASE("HeavyProcessor Integration", "[integration][processor]") {
    // JUCE Message Manager is needed for some plugin components (like parameters)
    // We might need a ScopedJuceInitialiser if tests crash, but for now try direct.

    SECTION("Processor Instantiation") {
        HeavyProcessor processor;
        REQUIRE(processor.getName() == "HEAVY");
    }

    SECTION("Audio Block Processing (Silence)") {
        HeavyProcessor processor;
        
        // Prepare
        processor.prepareToPlay(48000.0, 512);
        
        // Process Silence
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        juce::MidiBuffer midi;
        
        // Should not crash
        REQUIRE_NOTHROW(processor.processBlock(buffer, midi));
        
        // Output should likely remain silent or near-silent (depending on noise floor)
        // Given the physics engine might add noise/dither, we just check for sanity (not NaN)
        float rms = buffer.getRMSLevel(0, 0, 512);
        REQUIRE(rms < 2.0f); // Shouldn't explode
        REQUIRE(!std::isnan(rms));
    }
}
