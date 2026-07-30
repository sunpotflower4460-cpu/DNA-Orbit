#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Central definition of every DNA Orbit parameter: IDs (never renamed once
 * shipped), ranges, defaults, and the APVTS layout factory. Also holds the
 * Sync Division -> beats-per-cycle mapping used for tempo sync.
 */
namespace dnaorbit::params
{
    inline constexpr const char* rateID     = "rate";
    inline constexpr const char* syncID     = "sync";
    inline constexpr const char* divisionID = "division";
    inline constexpr const char* radiusID   = "radius";
    inline constexpr const char* depthID    = "depth";
    inline constexpr const char* symmetryID = "symmetry";
    inline constexpr const char* twistID    = "twist";
    inline constexpr const char* coreID     = "core";
    inline constexpr const char* nullCoreID = "nullCore";
    inline constexpr const char* mixID      = "mix";
    inline constexpr const char* outputID   = "output";
    inline constexpr const char* autoGainID = "autoGain";

    /**
     * State schema version, stored as a property on apvts.state (alongside
     * editorPage/Width/Height) rather than as a parameter, since it is not an
     * audio-controllable value.
     *
     *   1 - baseline: the 12 parameters above, Wet built from a mono (M-only)
     *       downmix, no stereo-preserving source model.
     *
     * Bump this whenever a new schema version changes how a *missing* schema
     * property (i.e. a project saved by an older build) should be
     * interpreted - not for ordinary new parameters, which APVTS already
     * defaults safely on its own. See PluginProcessor::setStateInformation.
     */
    inline constexpr const char* schemaVersionPropertyID = "dnaOrbitSchemaVersion";
    inline constexpr int currentStateSchemaVersion = 1;

    /**
     * Editor-only state (which tab is showing, window size) lives in its own
     * child node under apvts.state rather than as flat root properties, so it
     * stays clearly separate from anything that affects the sound. Every
     * project saved before this existed has these three as flat root
     * properties instead - see PluginProcessor::setStateInformation for the
     * one-time migration that moves them into this node.
     */
    inline constexpr const char* uiStateNodeID        = "uiState";
    inline constexpr const char* editorPagePropertyID   = "editorPage";
    inline constexpr const char* editorWidthPropertyID  = "editorWidth";
    inline constexpr const char* editorHeightPropertyID = "editorHeight";

    inline constexpr float rateMinHz = 0.02f;
    inline constexpr float rateMaxHz = 4.0f;
    inline constexpr float rateDefaultHz = 0.12f;

    inline const juce::StringArray syncDivisionChoices {
        "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8"
    };

    /** Number of quarter-note beats per one full orbit cycle, indexed as above (assumes 4/4). */
    inline double divisionIndexToBeats (int index) noexcept
    {
        switch (index)
        {
            case 0: return 16.0; // 4 bars
            case 1: return 8.0;  // 2 bars
            case 2: return 4.0;  // 1 bar
            case 3: return 2.0;  // 1/2
            case 4: return 1.0;  // 1/4
            case 5: return 0.5; // 1/8
            default: return 4.0;
        }
    }

    /** Converts a host tempo (BPM) and division index into an orbit rate in Hz. */
    inline double syncedRateHz (double bpm, int divisionIndex) noexcept
    {
        const double beatsPerCycle = divisionIndexToBeats (divisionIndex);
        if (bpm <= 0.0 || beatsPerCycle <= 0.0)
            return rateDefaultHz;
        const double secondsPerCycle = beatsPerCycle * (60.0 / bpm);
        if (secondsPerCycle <= 0.0)
            return rateDefaultHz;
        return juce::jlimit ((double) rateMinHz, (double) rateMaxHz, 1.0 / secondsPerCycle);
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> paramList;

        // Rate is displayed as seconds-per-revolution rather than Hz: for a slow
        // orbit LFO "8.3 s per turn" is far more intuitive than "0.12 Hz".
        // The ID, range and skew are unchanged - only the display text differs.
        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { rateID, 1 }, "Rate",
            juce::NormalisableRange<float> (rateMinHz, rateMaxHz, 0.0001f, 0.35f), rateDefaultHz,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float hz, int)
                {
                    if (hz <= 0.0f)
                        return juce::String ("-");
                    const float secondsPerTurn = 1.0f / hz;
                    // juce::String(const char*) parses as ASCII, so wrap UTF-8 explicitly.
                    return juce::String (secondsPerTurn, secondsPerTurn < 10.0f ? 2 : 1)
                         + juce::String (juce::CharPointer_UTF8 ("秒/周"));
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    const float seconds = text.getFloatValue();
                    if (seconds <= 0.0f)
                        return rateDefaultHz;
                    return juce::jlimit (rateMinHz, rateMaxHz, 1.0f / seconds);
                })));

        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { syncID, 1 }, "Sync", false));

        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { divisionID, 1 }, "Sync Division", syncDivisionChoices, 2));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { radiusID, 1 }, "Radius",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 80.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { depthID, 1 }, "Depth",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 55.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { symmetryID, 1 }, "Symmetry",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { twistID, 1 }, "Twist",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.01f), 5.0f,
            juce::AudioParameterFloatAttributes().withLabel ("ms")));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { coreID, 1 }, "Core",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { nullCoreID, 1 }, "Null Core", false));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { mixID, 1 }, "Mix",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 35.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { outputID, 1 }, "Output",
            juce::NormalisableRange<float> (-12.0f, 6.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        // Level-matches the Wet signal to Dry so moving Mix does not change the
        // perceived loudness. Defaults to on; can be switched off to get the
        // raw, uncompensated wet level.
        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { autoGainID, 1 }, "Auto Gain", true));

        return { paramList.begin(), paramList.end() };
    }
}
