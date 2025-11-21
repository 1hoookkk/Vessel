#include <catch2/catch_test_macros.hpp>
#include "../../source/Utility/WaitFreeTripleBuffer.h"
#include <thread>
#include <atomic>
#include <vector>

struct TestState {
    int id;
    float value;
};

TEST_CASE("WaitFreeTripleBuffer Basic Flow", "[TripleBuffer]") {
    WaitFreeTripleBuffer<TestState> buffer;

    SECTION("Initialization") {
        TestState state;
        REQUIRE_FALSE(buffer.pull(state)); // Should be empty/no new data
    }

    SECTION("Single Push and Pull") {
        // Producer
        {
            auto& back = buffer.getBackBuffer();
            back.id = 1;
            back.value = 1.0f;
            buffer.push();
        }

        // Consumer
        TestState out;
        bool hasNew = buffer.pull(out);
        REQUIRE(hasNew == true);
        REQUIRE(out.id == 1);
        REQUIRE(out.value == 1.0f);

        // Pull again - should be no new data
        bool hasMore = buffer.pull(out);
        REQUIRE(hasMore == false);
    }
}

TEST_CASE("WaitFreeTripleBuffer Overwrite Behavior", "[TripleBuffer]") {
    WaitFreeTripleBuffer<TestState> buffer;

    // Push 1
    {
        auto& back = buffer.getBackBuffer();
        back.id = 1;
        buffer.push();
    }

    // Push 2 (Consumer hasn't read yet, should overwrite/skip 1)
    {
        auto& back = buffer.getBackBuffer();
        back.id = 2;
        buffer.push();
    }

    TestState out;
    bool hasNew = buffer.pull(out);
    REQUIRE(hasNew == true);
    REQUIRE(out.id == 2); // Should get the latest
}

TEST_CASE("WaitFreeTripleBuffer Threaded Stress", "[TripleBuffer]") {
    WaitFreeTripleBuffer<TestState> buffer;
    std::atomic<bool> running{true};
    
    // Consumer Thread
    std::thread consumer([&]() {
        TestState lastState{0, 0.0f};
        while (running) {
            TestState current;
            if (buffer.pull(current)) {
                REQUIRE(current.id > lastState.id); // IDs must be increasing
                lastState = current;
            }
        }
    });

    // Producer Thread
    for (int i = 1; i <= 1000; ++i) {
        auto& back = buffer.getBackBuffer();
        back.id = i;
        back.value = (float)i;
        buffer.push();
        // Spin a bit to let consumer catch some
        // std::this_thread::sleep_for(std::chrono::microseconds(1)); 
    }

    running = false;
    consumer.join();
}
