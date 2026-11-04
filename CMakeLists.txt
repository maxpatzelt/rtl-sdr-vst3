cmake_minimum_required(VERSION 3.15)

project(RTLSDRPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE setup - allow user to specify JUCE path
if(NOT DEFINED JUCE_PATH)
    set(JUCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE framework")
endif()

if(EXISTS "${JUCE_PATH}/modules")
    message(STATUS "Using JUCE from: ${JUCE_PATH}")
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Please set JUCE_PATH to your JUCE installation directory.")
endif()

# RTL-SDR configuration
option(BUILD_WITH_RTLSDR "Build with RTL-SDR hardware support" ON)

if(BUILD_WITH_RTLSDR)
    find_library(RTLSDR_LIBRARY NAMES rtlsdr librtlsdr)
    find_path(RTLSDR_INCLUDE_DIR rtl-sdr.h)

    if(RTLSDR_LIBRARY AND RTLSDR_INCLUDE_DIR)
        set(HAS_RTLSDR_VAR 1)
        message(STATUS "RTL-SDR found: ${RTLSDR_LIBRARY}")
    else()
        set(HAS_RTLSDR_VAR 0)
        message(WARNING "RTL-SDR library not found. Building in simulation mode.")
    endif()
else()
    set(HAS_RTLSDR_VAR 0)
    message(STATUS "Building WITHOUT RTL-SDR hardware support (simulation mode)")
endif()

# Plugin target
juce_add_plugin(RTLSDRPlugin
    COMPANY_NAME "AudioRF"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE ARFE
    PLUGIN_CODE Rsdr
    FORMATS VST3
    PRODUCT_NAME "RTL-SDR Radio"
    VST3_CATEGORIES "Generator"
)

target_sources(RTLSDRPlugin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/RTLSDRDevice.cpp
        Source/FMDemodulator.cpp
        Source/AMDemodulator.cpp
        Source/SSBDemodulator.cpp
        Source/AudioResampler.cpp
        Source/RingBuffer.cpp
        Source/SpectrumAnalyzer.cpp
        Source/WaterfallDisplay.cpp
        Source/SignalMeter.cpp
        Source/PresetManager.cpp
        Source/FrequencyHistory.cpp
)

target_compile_definitions(RTLSDRPlugin
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    PRIVATE
        HAS_RTLSDR=${HAS_RTLSDR_VAR}
)

target_link_libraries(RTLSDRPlugin
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Link RTL-SDR if found
if(HAS_RTLSDR_VAR)
    target_include_directories(RTLSDRPlugin PRIVATE ${RTLSDR_INCLUDE_DIR})
    target_link_libraries(RTLSDRPlugin PRIVATE ${RTLSDR_LIBRARY})
endif()
