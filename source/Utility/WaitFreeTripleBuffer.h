#pragma once
#include <atomic>
#include <array>

/*
  ==============================================================================
    Wait-Free Triple Buffer
    Pattern: Single Producer (Audio), Single Consumer (UI)
    
    Safety:
    - Producer never blocks (Wait-Free).
    - Consumer never blocks (Wait-Free).
    - No locks, mutexes, or allocations.
    
    Logic:
    - 3 Buffers: Back (Writing), Front (Reading), Middle (Exchange).
    - Producer writes to Back, then atomic_swaps with Middle.
    - Consumer checks flag, atomic_swaps Middle with Front.
  ==============================================================================
*/

template <typename T>
class WaitFreeTripleBuffer {
public:
    WaitFreeTripleBuffer() {
        newContentAvailable.store(false);
        
        // Initial ownership
        indexBack = 0;
        indexMiddle = 1;
        indexFront = 2;
        
        // Ensure default construction of buffers
    }

    // -------------------------------------------------------------------------
    // PRODUCER API (Audio Thread)
    // -------------------------------------------------------------------------
    
    // Get a reference to the writable back buffer.
    // Does NOT publish. Use push() to publish.
    T& getBackBuffer() {
        return buffers[indexBack];
    }

    // Publish the current Back buffer content to the Middle slot.
    void push() {
        // 1. Swap Back and Middle indices.
        // 'memory_order_acq_rel' ensures:
        // - Release: The write to buffers[indexBack] is visible to Consumer.
        // - Acquire: We see the latest indexMiddle value.
        indexBack = indexMiddle.exchange(indexBack, std::memory_order_acq_rel);
        
        // 2. Signal availability.
        newContentAvailable.store(true, std::memory_order_release);
    }
    
    // Convenience: Write and Push
    void push(const T& state) {
        buffers[indexBack] = state;
        push();
    }

    // -------------------------------------------------------------------------
    // CONSUMER API (UI Thread)
    // -------------------------------------------------------------------------
    
    // Attempt to retrieve the latest state.
    // Returns true if new data was retrieved.
    bool pull(T& outState) {
        // 1. Check for new content
        if (newContentAvailable.load(std::memory_order_acquire)) {
            
            // 2. Steal the Middle buffer (which has the latest data)
            // The old Front becomes the new Middle (recycling the slot)
            indexFront = indexMiddle.exchange(indexFront, std::memory_order_acq_rel);
            
            // 3. Reset flag
            // We took the data. 
            newContentAvailable.store(false, std::memory_order_relaxed);
            
            // 4. Copy data to output
            outState = buffers[indexFront];
            return true;
        }
        
        // No new data, but we can still read the *current* Front buffer if needed
        // (Though typically we only want to repaint on new data)
        return false;
    }

    // Get the last valid state (even if not new)
    const T& getCurrentState() const {
        return buffers[indexFront];
    }

private:
    std::array<T, 3> buffers;
    
    int indexBack;           // Owned by Producer
    int indexFront;          // Owned by Consumer
    std::atomic<int> indexMiddle; // Shared Exchange
    
    std::atomic<bool> newContentAvailable;
};
