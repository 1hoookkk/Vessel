cmake_minimum_required(VERSION 3.24)
project(HyperCube VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(JUCE)

set(FORMATS AU VST3 Standalone)

juce_add_plugin(HyperCube
    COMPANY_NAME "VesselAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Vesl
    PLUGIN_CODE HCub
    FORMATS ${FORMATS}
    PRODUCT_NAME "HyperCube"
)

juce_generate_juce_header(HyperCube)

target_sources(HyperCube PRIVATE
    Source/HyperCubeProcessor.cpp
    Source/HyperCubeProcessor.h
    Source/HyperCubeEditor.cpp
    Source/HyperCubeEditor.h
    Source/ZPlaneVisualizer.h
    # Core DSP
    Source/DSP/EmuZPlaneCore.cpp
    Source/DSP/EmuZPlaneCore.h
)

target_include_directories(HyperCube PUBLIC Source)

target_link_libraries(HyperCube PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(HyperCube PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)

enable_testing()
