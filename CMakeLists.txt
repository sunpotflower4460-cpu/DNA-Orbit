cmake_minimum_required(VERSION 3.22)

project(DNAOrbit VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(DNA_ORBIT_BUILD_TESTS "Build the DNA Orbit unit test suite" ON)

# Local sanitizer builds (no CI service is used for this project - see
# docs/commercial-upgrade/ - so this is the only automated way to run
# ASan/UBSan). Usage:
#   cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DDNA_ORBIT_SANITIZERS="address,undefined"
#   cmake --build build-asan -j && ctest --test-dir build-asan --output-on-failure
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

# Local equivalent of the CI matrix's "warnings as errors" build (no CI
# service is used for this project - see docs/commercial-upgrade/ - so this
# is the only automated way to run it). Applied only to this project's own
# targets (see the DNAOrbit/DNAOrbitTests target-specific blocks below), not
# to JUCE's own sources, which this project does not control. Usage:
#   cmake -S . -B build-werror -DDNA_ORBIT_WARNINGS_AS_ERRORS=ON
#   cmake --build build-werror -j
option(DNA_ORBIT_WARNINGS_AS_ERRORS "Treat this project's own compiler warnings as errors (GCC/Clang only)" OFF)

# The UI text is Japanese, so the sources are UTF-8. GCC/Clang assume UTF-8
# already; MSVC needs to be told explicitly for both the source and execution
# character sets, otherwise the literals are mangled.
if (MSVC)
    add_compile_options(/utf-8)
endif()

# ---------------------------------------------------------------------------
# JUCE acquisition
#
# JUCE is fetched via CMake FetchContent and pinned to tag 8.0.15 so that
# builds are reproducible. If network access is unavailable, clone JUCE
# manually into a sibling "JUCE" directory next to this CMakeLists.txt (or
# anywhere) and set -DFETCHCONTENT_SOURCE_DIR_JUCE=<path-to-JUCE-checkout>
# when configuring, e.g.:
#
#   git clone --branch 8.0.15 --depth 1 https://github.com/juce-framework/JUCE.git
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=$(pwd)/JUCE
# ---------------------------------------------------------------------------
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
    # Must be TRUE for the editor's Ctrl+Z / Ctrl+Shift+Z Undo/Redo to work
    # in a plugin host at all. This maps to JucePlugin_EditorRequiresKeyboardFocus,
    # and when it is FALSE the wrapper adds the editor to the desktop with
    # ComponentPeer::windowIgnoresKeyPresses (see JUCE's
    # detail/juce_VSTWindowUtilities.h) - the window then never receives a
    # key event, so PluginEditor::keyPressed() is never called no matter what
    # setWantsKeyboardFocus() says on the component. It was FALSE when
    # Undo/Redo shipped, which meant the shortcuts only ever worked in the
    # Standalone build; see ADR-012.
    #
    # Cost of TRUE: while the plugin window has focus, keys it does not
    # consume may not reach the host's own shortcuts (spacebar transport is
    # the usual casualty). keyPressed() returns false for everything except
    # the Undo/Redo chords specifically so that hosts which do forward
    # unhandled keys still can - but whether a given host actually does needs
    # real-DAW checking (MANUAL_REQUIRED.md).
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(DNAOrbit PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Parameters.h
    Source/dsp/HelixEngine.cpp
    Source/dsp/HelixEngine.h
    Source/dsp/OrbitMath.h
    Source/dsp/OnePoleLowPass.h
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

if (DNA_ORBIT_WARNINGS_AS_ERRORS AND NOT MSVC)
    # Applied per-source-file (not via target_compile_options/
    # add_compile_options) so it only touches this project's own
    # translation units, never JUCE's vendored ones - juce_add_plugin
    # compiles both into the same DNAOrbit target, and this project does
    # not control JUCE's own warnings.
    get_target_property(_dnaOrbitSources DNAOrbit SOURCES)
    set_source_files_properties(${_dnaOrbitSources} PROPERTIES COMPILE_OPTIONS "-Werror")
endif()

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

# Developer tool: renders editor screenshots headlessly by driving the processor
# directly. Needed because a machine with no audio device never calls
# processBlock, which leaves the visualiser frozen in the Standalone build.
option(DNA_ORBIT_BUILD_TOOLS "Build the DNA Orbit screenshot tool" OFF)

if (DNA_ORBIT_BUILD_TOOLS)
    # Plain executable reusing the plugin's shared code, exactly like the test
    # target: juce_add_console_app would force JUCE_STANDALONE_APPLICATION=1 and
    # clash with the plugin's own definition.
    add_executable(DNAOrbitRenderShots Tools/RenderShots.cpp)
    target_include_directories(DNAOrbitRenderShots PRIVATE Source)
    target_compile_definitions(DNAOrbitRenderShots PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(DNAOrbitRenderShots PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    # Developer tool: prints Wet RMS/L-R balance/mono-sum/correlation for a
    # fixed set of input conditions across Stereo Preserve values, using the
    # real HelixEngine. Backs the measurements cited in
    # docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md.
    add_executable(DNAOrbitStereoPreserveAnalysis Tools/StereoPreserveAnalysis.cpp)
    target_include_directories(DNAOrbitStereoPreserveAnalysis PRIVATE Source)
    target_link_libraries(DNAOrbitStereoPreserveAnalysis PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    # Developer tool: deterministic DSP throughput benchmark. Absolute
    # numbers from this container are not trustworthy (see ADR-005); the
    # value is before/after comparison on one machine in one session.
    add_executable(DNAOrbitBenchmarkDSP Tools/BenchmarkDSP.cpp)
    target_include_directories(DNAOrbitBenchmarkDSP PRIVATE Source)
    target_link_libraries(DNAOrbitBenchmarkDSP PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    # Developer tool: measures how audible a parameter set actually is
    # (pan swing, width, level, mono fold-down), so preset intensity can be
    # tuned from measurements rather than guesswork.
    add_executable(DNAOrbitPresetIntensityAnalysis Tools/PresetIntensityAnalysis.cpp)
    target_include_directories(DNAOrbitPresetIntensityAnalysis PRIVATE Source)
    target_link_libraries(DNAOrbitPresetIntensityAnalysis PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    # Developer/demo tool: renders short WAV files (dry + processed through a
    # real factory preset) of a synthesized pad and a synthesized "vocal-ish"
    # tone, so the effect can actually be listened to. Not part of the
    # shipped plugin.
    add_executable(DNAOrbitRenderAudioSample Tools/RenderAudioSample.cpp)
    target_include_directories(DNAOrbitRenderAudioSample PRIVATE Source)
    target_link_libraries(DNAOrbitRenderAudioSample PRIVATE
        DNAOrbit
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
endif()
