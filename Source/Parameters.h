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
    inline constexpr const char* stereoPreserveID = "stereoPreserve";
    inline constexpr const char* bassAnchorHzID   = "bassAnchorHz";
    inline constexpr const char* characterID      = "character";
    inline constexpr const char* phaseModeID      = "phaseMode";
    inline constexpr const char* startPhaseID     = "startPhase";
    inline constexpr const char* directionID      = "direction";
    inline constexpr const char* softBypassID     = "softBypass";
    inline constexpr const char* monoPreviewID    = "monoPreview";

    /**
     * State schema version, stored as a property on apvts.state (alongside
     * editorPage/Width/Height) rather than as a parameter, since it is not an
     * audio-controllable value.
     *
     *   1 - baseline: the 12 parameters above, Wet built from a mono (M-only)
     *       downmix, no stereo-preserving source model.
     *   2 - adds Stereo Preserve (stereoPreserveID): the two strands and
     *       Core stay fed from a shared Mid downmix (as in schema 1) at any
     *       value, but the input's Side content is additionally injected as
     *       a separate, non-orbiting "bed" scaled by this parameter, fixing
     *       anti-phase stereo input collapsing Wet to silence without
     *       coupling either strand's loudness to the input's L/R balance
     *       (see HelixEngine::process() and
     *       docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md).
     *       A schema-1 save has no stereoPreserve PARAM node at all (it
     *       didn't exist yet), so it is force-set to 0 on load to exactly
     *       reproduce the schema-1 sound (see PluginProcessor::setStateInformation
     *       and Tests/BaselineRegressionTests.cpp). A genuinely fresh
     *       instance (nothing loaded) gets the parameter's declared default
     *       instead.
     *   3 - adds Bass Anchor (bassAnchorHzID): a Linkwitz-Riley crossover
     *       splits input into a low band (kept as a direct, non-orbiting
     *       stereo-image-preserving anchor) and a high band (which alone
     *       feeds the strands/Core/Stereo-Preserve-bed machinery). At the
     *       parameter's range minimum (20Hz) the crossover is fully bypassed
     *       in the DSP, not just a near-zero split, which is what makes 20Hz
     *       the exact schema-2-and-earlier-compatible value. A schema-1/2
     *       save has no bassAnchorHz PARAM node, so it is force-set to 20Hz
     *       (Off) on load; a fresh instance gets the declared default.
     *
     * Bump this whenever a new schema version changes how a *missing* schema
     * property (i.e. a project saved by an older build) should be
     * interpreted - not for ordinary new parameters, which APVTS already
     * defaults safely on its own. See PluginProcessor::setStateInformation.
     */
    inline constexpr const char* schemaVersionPropertyID = "dnaOrbitSchemaVersion";
    inline constexpr int currentStateSchemaVersion = 3;

    /**
     * A save with no schemaVersion attribute at all predates the property
     * itself (it did not exist before schema 1), so it is schema 1 by
     * definition - a fixed historical fact, NOT "whatever the current
     * version happens to be". Do not use currentStateSchemaVersion as that
     * fallback: it will keep incrementing, but a missing attribute always
     * means schema 1.
     */
    inline constexpr int legacyUnversionedSchema = 1;

    /** Schema version at which stereoPreserveID first existed; see above. */
    inline constexpr int stereoPreserveIntroducedInSchema = 2;

    /** Schema version at which bassAnchorHzID first existed; see above. */
    inline constexpr int bassAnchorIntroducedInSchema = 3;

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

    // A fresh instance defaults to 70%: the centre-at-zero geometry is
    // identical at every Stereo Preserve value (it depends only on the two
    // strands staying antipodal, not on what feeds them), so there is no
    // physical trade-off in picking a higher default - it just better
    // expresses "two distinct strands" and incidentally fixes anti-phase
    // stereo input collapsing Wet to silence. A project saved before this
    // parameter existed (schema 1) is force-set to 0 instead, to exactly
    // reproduce its original sound - see currentStateSchemaVersion above.
    inline constexpr float stereoPreserveDefaultPercent = 70.0f;
    inline constexpr float stereoPreserveLegacyPercent = 0.0f;

    inline constexpr float bassAnchorMinHz = 20.0f;
    inline constexpr float bassAnchorMaxHz = 500.0f;
    inline constexpr float bassAnchorDefaultHz = 120.0f;
    // Also the exact DSP-bypass value - see HelixEngine::process().
    inline constexpr float bassAnchorLegacyHz = bassAnchorMinHz;

    inline const juce::StringArray syncDivisionChoices {
        "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8"
    };

    /**
     * How many bars each division choice spans, independent of time
     * signature (a "bar" always means one measure, whatever its length).
     */
    inline double divisionIndexToBars (int index) noexcept
    {
        switch (index)
        {
            case 0: return 4.0;   // 4 bars
            case 1: return 2.0;   // 2 bars
            case 2: return 1.0;   // 1 bar
            case 3: return 0.5;   // 1/2 bar
            case 4: return 0.25;  // 1/4 bar
            case 5: return 0.125; // 1/8 bar
            default: return 1.0;
        }
    }

    /**
     * Number of quarter-note beats per one full orbit cycle. quarterNotesPerBar
     * defaults to 4/4 for any caller that does not have host time-signature
     * info; passing the host's actual numerator*4/denominator makes "N bars"
     * mean the same wall-clock duration under any time signature (see
     * docs/commercial-upgrade/02_DSP再設計仕様書.md §5.4).
     */
    inline double divisionIndexToBeats (int index, double quarterNotesPerBar = 4.0) noexcept
    {
        if (quarterNotesPerBar <= 0.0)
            quarterNotesPerBar = 4.0;
        return divisionIndexToBars (index) * quarterNotesPerBar;
    }

    /** Converts a host tempo (BPM) and division index into an orbit rate in Hz. */
    inline double syncedRateHz (double bpm, int divisionIndex, double quarterNotesPerBar = 4.0) noexcept
    {
        const double beatsPerCycle = divisionIndexToBeats (divisionIndex, quarterNotesPerBar);
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

        // Both strands and Core always stay fed from a shared Mid downmix at
        // any value (so their energies stay exactly balanced regardless of
        // input); this controls how much of the input's Side content is
        // added back as a separate, non-orbiting width "bed". See
        // HelixEngine::process(), the schema-version doc comment above, and
        // docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md.
        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { stereoPreserveID, 1 }, "Stereo Preserve",
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), stereoPreserveDefaultPercent,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        // At the range minimum (20Hz) the crossover is fully bypassed in the
        // DSP (see HelixEngine::process()), not merely a near-zero split -
        // that is what makes 20Hz both the displayed "Off" state and the
        // exact schema-2-and-earlier-compatible value.
        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { bassAnchorHzID, 1 }, "Bass Anchor",
            juce::NormalisableRange<float> (bassAnchorMinHz, bassAnchorMaxHz, 0.01f, 0.35f),
            bassAnchorDefaultHz,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float hz, int)
                {
                    if (hz <= bassAnchorMinHz + 0.05f)
                        return juce::String ("Off");
                    return juce::String ((int) std::round (hz)) + juce::String (juce::CharPointer_UTF8 ("Hz"));
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    if (text.trim().equalsIgnoreCase ("Off"))
                        return bassAnchorMinHz;
                    return juce::jlimit (bassAnchorMinHz, bassAnchorMaxHz, text.getFloatValue());
                })));

        // Natural (index 0) is exactly this engine's fixed back-attenuation/
        // cutoff/delay constants from before Character existed - the
        // default reproduces every existing project's sound unchanged, so
        // this parameter needed no schema-version bump. See
        // HelixEngine::applyParameters() and
        // docs/commercial-upgrade/decisions/ADR-006-character.md.
        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { characterID, 1 }, "Character",
            juce::StringArray { "Natural", "Vivid", "Deep" }, 0));

        // Free (index 0) is exactly this engine's original continuous,
        // rate-integrated phase - no PPQ dependency, no retrigger-on-play -
        // so, like Character, this needed no schema-version bump: the
        // default reproduces every existing project's behaviour unchanged.
        // Host Lock computes phase directly from the host's PPQ position
        // (drift-free, reproducible across replays/offline bounce);
        // Retrigger resets to Start Phase only when playback starts. See
        // HelixEngine::process() and
        // docs/commercial-upgrade/decisions/ADR-007-host-phase-lock.md.
        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { phaseModeID, 1 }, "Phase Mode",
            juce::StringArray { "Free", "Retrigger", "Host Lock" }, 0));

        paramList.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { startPhaseID, 1 }, "Start Phase",
            juce::NormalisableRange<float> (0.0f, 360.0f, 0.01f), 0.0f,
            juce::AudioParameterFloatAttributes().withLabel (juce::String (juce::CharPointer_UTF8 ("°")))));

        paramList.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { directionID, 1 }, "Direction",
            juce::StringArray { "CW", "CCW" }, 0));

        // In-plugin Soft Bypass, independent of the host's own Bypass. Off
        // (the default) is exactly this engine's normal processing - no
        // schema bump needed. When on, HelixEngine::process() still runs the
        // full effect chain every block (so the orbit phase, filters, and
        // smoothers never freeze - see ADR-008) and crossfades the final
        // output to the dry input over ~30ms, avoiding both the freeze
        // problem ADR-001 fixed for host Bypass and any dependency on how
        // well a given host automates/announces its own Bypass.
        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { softBypassID, 1 }, "Soft Bypass", false));

        // Monitoring-only utility: folds the finished output (post Soft
        // Bypass, so it previews whatever is actually being heard) down to
        // mono, for checking mono-compatibility. Off (default) leaves
        // output untouched - no schema bump needed. See ADR-009.
        paramList.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { monoPreviewID, 1 }, "Mono Preview", false));

        return { paramList.begin(), paramList.end() };
    }
}
