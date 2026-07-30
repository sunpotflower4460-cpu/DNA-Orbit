# DNA Orbit test suite. Reuses the already-compiled PluginProcessor / HelixEngine
# object code from the main DNAOrbit "shared code" target so PluginProcessor's
# JucePlugin_* macros never need to be redefined here.

add_executable(DNAOrbitTests
    TestMain.cpp
    OrbitMathTests.cpp
    HelixEngineTests.cpp
    StateTests.cpp
    LevelMatchTests.cpp
    GeometryTests.cpp
    BaselineRegressionTests.cpp
    PresetsTests.cpp
    StereoPreserveTests.cpp
    CrossoverFilterTests.cpp
    BassAnchorTests.cpp
    CharacterTests.cpp
    HostPhaseLockTests.cpp
    SoftBypassTests.cpp
)

target_include_directories(DNAOrbitTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(DNAOrbitTests PRIVATE
    DNAOrbit
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_test(NAME DNAOrbitTests COMMAND DNAOrbitTests)
set_tests_properties(DNAOrbitTests PROPERTIES TIMEOUT 600)
