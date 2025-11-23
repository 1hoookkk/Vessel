cmake_minimum_required(VERSION 3.24)
project(HEAVY VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

# 2. HEAVY Core Library 
# (Strict H-Chip Architecture - No Modern Physics/Exciter files)
add_subdirectory(foleys_gui_magic)

add_library(Heavy_Core STATIC
    source/PluginProcessor.cpp
    source/PluginProcessor.h
    source/Components/VesselXYPad.cpp
    source/Components/VesselXYPad.h
    source/Components/VesselGlassCube.cpp
    source/Components/VesselGlassCube.h
    
    # The Digital Archaeology Core (H-Chip Emulation)
    source/DSP/EmuHChipFilter.h
    source/DSP/EmuZPlaneCore.h
    source/DSP/EmuZPlaneCore.cpp
    source/DSP/PresetConverter.h
    source/DSP/PresetConverter.cpp
    source/DSP/EMUStaticShapeBank.h
    source/DSP/EMUStaticShapeBank.cpp
    source/DSP/EMUAuthenticTables.h
    source/DSP/IZPlaneEngine.h

    # Shared Data & Utilities
    source/HeavyConstants.h
    source/SharedData.h
    source/Utility/WaitFreeTripleBuffer.h

    # UI Components
    source/VesselStyle.h

    # Utilities (Visualizer helper math)
    source/DSP/zplane_visualizer_math.cpp
    source/DSP/zplane_visualizer_math.h
)

# Includes
target_include_directories(Heavy_Core PUBLIC source)

# Dependencies
target_link_libraries(Heavy_Core PUBLIC
    foleys_gui_magic
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_graphics 
    juce::juce_recommended_config_flags
)

# Compiler Definitions
target_compile_definitions(Heavy_Core PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# 3. Main Plugin Target
set(FORMATS AU VST3 Standalone)

juce_add_plugin(HEAVY
    COMPANY_NAME "HeavyAudio"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Hvya
    PLUGIN_CODE Hvyp
    FORMATS ${FORMATS}
    PRODUCT_NAME "HEAVY"
)

# Link
target_link_libraries(HEAVY PRIVATE Heavy_Core)

# REQUIRED: Fixes JuceHeader linkage for the plugin target too
juce_generate_juce_header(HEAVY)