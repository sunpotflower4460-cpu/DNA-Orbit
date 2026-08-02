#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * Central definition of every DNA Orbit parameter. Parameter IDs are part of
 * the host/session contract and must never be renamed after release.
 */
namespace dnaorbit::params
{
    inline constexpr const char* rateID       = "rate";
    inline constexpr const char* syncID       = "sync";
    inline constexpr const char* divisionID   = "division";
    inline constexpr const char* radiusID     = "radius";
    inline constexpr const char* depthID      = "depth";
    inline constexpr const char* symmetryID   = "symmetry";
    inline constexpr const char* twistID      = "twist";
    inline constexpr const char* coreID       = "core";
    inline constexpr const char* nullCoreID   = "nullCore";
    inline constexpr const char* mixID        = "mix";
    inline constexpr const char* outputID     = "output";
    inline constexpr const char* autoGainID   = "autoGain";
    inline constexpr const char* stereoPreserveID = "stereoPreserve";
    inline constexpr const char* softBypassID = "softBypass";
    inline constexpr const char* bassAnchorID = "bassAnchorHz";
    inline constexpr const char* characterID  = "character";
    inline constexpr const char* phaseModeID  = "phaseMode";
    inline constexpr const char* startPhaseID = "startPhase";
    inline constexpr const char* directionID  = "direction";

    /**
     * State schema:
     *   1 - original Mid-only Wet path.
     *   2 - Stereo Preserve stationary Side bed.
     *   3 - Bass Anchor, Character and deterministic transport phase controls.
     *
     * Missing schema-3 parameters in an older save are migrated to values that
     * preserve the previous sound: Bass Anchor 20 Hz (effectively off), Natural
     * Character, Free phase. Fresh instances use the product defaults below.
     */
    inline constexpr const char* schemaVersionPropertyID = "dnaOrbitSchemaVersion";
    inline constexpr int currentStateSchemaVersion = 3;
    inline constexpr int legacyUnversionedSchema = 1;
    inline constexpr int stereoPreserveIntroducedInSchema = 2;
    inline constexpr int musicalDspIntroducedInSchema = 3;

    inline constexpr const char* uiStateNodeID          = "uiState";
    inline constexpr const char* editorPagePropertyID   = "editorPage";
    inline constexpr const char* editorWidthPropertyID  = "editorWidth";
    inline constexpr const char* editorHeightPropertyID = "editorHeight";

    inline constexpr float rateMinHz = 0.02f;
    inline constexpr float rateMaxHz = 4.0f;
    inline constexpr float rateDefaultHz = 0.12f;

    inline constexpr float stereoPreserveDefaultPercent = 70.0f;
    inline constexpr float stereoPreserveLegacyPercent = 0.0f;
    inline constexpr float bassAnchorDefaultHz = 120.0f;
    inline constexpr float bassAnchorLegacyHz = 20.0f;

    enum PhaseMode
    {
        phaseFree = 0,
        phaseRetrigger,
        phaseHostLock
    };

    enum Direction
    {
        clockwise = 0,
        counterClockwise
    };

    enum Character
    {
        characterNatural = 0,
        characterVivid,
        characterDeep
    };

    inline const juce::StringArray syncDivisionChoices {
        "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8"
    };

    inline const juce::StringArray characterChoices { "Natural", "Vivid", "Deep" };
    inline const juce::StringArray phaseModeChoices { "Free", "Retrigger", "Host Lock" };
    inline const juce::StringArray directionChoices { "CW", "CCW" };

    inline double quarterNotesPerBar (int numerator, int denominator) noexcept
    {
        const int safeNumerator = numerator > 0 ? numerator : 4;
        const int safeDenominator = denominator > 0 ? denominator : 4;
        return (double) safeNumerator * 4.0 / (double) safeDenominator;
    }

    /** Number of quarter notes in one full orbit cycle. */
    inline double divisionIndexToBeats (int index, int numerator = 4, int denominator = 4) noexcept
    {
        const double bar = quarterNotesPerBar (numerator, denominator);
        switch (index)
        {
            case 0: return 4.0 * bar;
            case 1: return 2.0 * bar;
            case 2: return bar;
            case 3: return 2.0;  // half note
            case 4: return 1.0;  // quarter note
            case 5: return 0.5;  // eighth note
            default: return bar;
        }
    }

    inline double syncedRateHz (double bpm, int divisionIndex,
                                int numerator = 4, int denominator = 4) noexcept
    {
        const double beatsPerCycle = divisionIndexToBeats (divisionIndex, numerator, denominator);
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

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { rateID, 1 }, "Rate",
            juce::NormalisableRange<float> (rateMinHz, rateMaxHz, 0.0001f, 0.35f), rateDefaultHz,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float hz, int)
                {
                    if (hz <= 0.0f)
                        return juce::String ("-");
                    const float secondsPerTurn = 1.0f / hz;
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

        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { autoGainID, 1 }, "Auto Gain", true));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { stereoPreserveID, 1 }, "Stereo Preserve",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), stereoPreserveDefaultPercent,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { softBypassID, 1 }, "Soft Bypass", false));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bassAnchorID, 1 }, "Bass Anchor",
            juce::NormalisableRange<float> (20.0f, 500.0f, 0.1f, 0.35f), bassAnchorDefaultHz,
            juce::AudioParameterFloatAttributes()
                .withLabel ("Hz")
                .withStringFromValueFunction ([] (float hz, int)
                {
                    return hz <= 20.1f ? juce::String ("Off") : juce::String (hz, hz < 100.0f ? 1 : 0) + " Hz";
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    return text.containsIgnoreCase ("off") ? 20.0f : text.getFloatValue();
                })));

        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { characterID, 1 }, "Character", characterChoices, characterNatural));

        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { phaseModeID, 1 }, "Phase Mode", phaseModeChoices, phaseHostLock));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { startPhaseID, 1 }, "Start Phase",
            juce::NormalisableRange<float> (0.0f, 360.0f, 0.1f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel (juce::String::fromUTF8 ("°"))));

        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { directionID, 1 }, "Direction", directionChoices, clockwise));

        return { paramList.begin(), paramList.end() };
    }
}
