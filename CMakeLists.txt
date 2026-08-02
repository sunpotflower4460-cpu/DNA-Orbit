cmake_minimum_required(VERSION 3.22)

project(DNAOrbit VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DNA_ORBIT_BUILD_TESTS "Build the DNA Orbit unit test suite" ON)

set(DNA_ORBIT_SANITIZERS "" CACHE STRING
    "Comma-separated sanitizers to build with, e.g. 'address,undefined' (GCC/Clang only)")

if (NOT DNA_ORBIT_SANITIZERS STREQUAL "")
    if (MSVC)
        message(WARNING "DNA_ORBIT_SANITIZERS is not supported with MSVC; ignoring.")
    else()
        add_compile_options(-fsanitize=${DNA_ORBIT_SANITIZERS} -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=${DNA_ORBIT_SANITIZERS})
    endif()
endif()

if (MSVC)
    add_compile_options(/utf-8)
endif()

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.15
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(DNAOrbit
    PRODUCT_NAME                "DNA Orbit"
    PLUGIN_MANUFACTURER_CODE    FpSt
    PLUGIN_CODE                 DnaO
    COMPANY_NAME                "Flower Pot Studio"
    BUNDLE_ID                   "com.flowerpotstudio.dnaorbit"
    FORMATS                     Standalone VST3 AU
    VST3_CATEGORIES             Fx Spatial
    AU_MAIN_TYPE                kAudioUnitType_Effect
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(DNAOrbit PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.h
    Source/Presets.h
    Source/dsp/HelixEngine.cpp
    Source/dsp/HelixEngine.h
    Source/dsp/OrbitMath.h
    Source/dsp/OnePoleLowPass.h
    Source/dsp/LinkwitzRileyCrossover.h
    Source/dsp/StereoUtilities.h
    Source/ui/HelixView3D.cpp
    Source/ui/HelixView3D.h
    Source/ui/HelixHistory.h
    Source/ui/Projection3D.h
    Source/ui/SphereSprite.h
    Source/ui/DnaLookAndFeel.cpp
    Source/ui/DnaLookAndFeel.h
)

target_include_directories(DNAOrbit PRIVATE Source)

target_compile_definitions(DNAOrbit PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
)

target_link_libraries(DNAOrbit PUBLIC
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if (DNA_ORBIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

option(DNA_ORBIT_BUILD_TOOLS "Build the DNA Orbit screenshot tool" OFF)

if (DNA_ORBIT_BUILD_TOOLS)
    add_executable(DNAOrbitRenderShots Tools/RenderShots.cpp)
    target_include_directories(DNAOrbitRenderShots PRIVATE Source)
    target_compile_definitions(DNAOrbitRenderShots PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(DNAOrbitRenderShots PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
