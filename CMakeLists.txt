cmake_minimum_required(VERSION 3.24)
project(HEAVY VERSION 1.0.0)

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

# --- Foleys GUI Magic ---
option(FOLEYS_BUILD_EXAMPLES "Build the examples" OFF)
option(FOLEYS_BUILD_TESTS "Build and run the unit tests" OFF)
add_subdirectory(foleys_gui_magic)

# --- HEAVY Core Library (for Testing) ---
add_library(Heavy_Core STATIC
    source/PluginProcessor.cpp
    source/PluginProcessor.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/HeavyConstants.h
    source/UI/HeavyComponents.h
    source/UI/RadarScope.h
    source/UI/HeavyBar.h
    source/DSP/MaterialPhysics.h
    source/DSP/ExciterEngine.h
    source/DSP/AuthenticEMUEngine.h
    source/DSP/EMUAuthenticTables.h
    source/DSP/IShapeBank.h
    source/DSP/IZPlaneEngine.h
    source/DSP/NonlinearStage.h
    source/DSP/EMUStaticShapeBank.h
    source/DSP/EMUStaticShapeBank.cpp
    source/DSP/ZPlaneCoefficientBank.cpp
    source/DSP/ZPoleMath.h
    source/DSP/ZPlaneCore.h
    source/DSP/ZPlaneCommon.h
    source/DSP/ZPlaneMath.h
    source/DSP/EmuZPlaneCore.cpp
    source/DSP/EmuZPlaneCore.h
    source/DSP/PresetConverter.cpp
    source/DSP/PresetConverter.h
    source/DSP/zplane_visualizer_math.cpp
    source/DSP/zplane_visualizer_math.h
)

target_include_directories(Heavy_Core PUBLIC source)

target_link_libraries(Heavy_Core PUBLIC
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics # Added for Editor components
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
    foleys_gui_magic
)

target_compile_definitions(Heavy_Core PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# --- Main Plugin Target ---
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

# Link the core library
target_link_libraries(HEAVY PRIVATE Heavy_Core)

# Tests
target_include_directories(HEAVY
    PUBLIC
        source
)

enable_testing()
add_subdirectory(tests)
