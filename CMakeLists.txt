cmake_minimum_required(VERSION 3.24)

project(VESSEL VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory (assumes JUCE is in libs/JUCE or similar)
# For production, use CPM or FetchContent to fetch JUCE
add_subdirectory(libs/JUCE EXCLUDE_FROM_ALL)

# VST3 SDK for formats
juce_set_vst2_sdk_path(~/SDKs/VST_SDK/VST2_SDK)

# Plugin configuration
set(FORMATS AU VST3 Standalone)

# Source files
set(SOURCE_FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# Create the plugin target
juce_add_plugin(VESSEL
    COMPANY_NAME "ARMAdillo"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Armd
    PLUGIN_CODE Vzpf
    FORMATS ${FORMATS}
    PRODUCT_NAME "VESSEL"
    VST3_CATEGORIES Fx Filter
    AU_MAIN_TYPE kAudioUnitType_Effect
)

# Generate JuceHeader.h
juce_generate_juce_header(VESSEL)

# Source files
target_sources(VESSEL PRIVATE ${SOURCE_FILES})

# Compiler definitions
target_compile_definitions(VESSEL
    PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

# Link JUCE modules
target_link_libraries(VESSEL
    PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(VESSEL
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/Structure
)
