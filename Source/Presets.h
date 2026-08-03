#pragma once

#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

/**
 * Factory presets defined as a point in the FULL parameter space (not just
 * the handful of knobs shown on the Basic tab), so applying one is
 * deterministic: the result depends only on which preset was picked, never on
 * whatever Sync/Division/Output/Auto Gain/Stereo Preserve/Bass Anchor
 * happened to be left at beforehand.
 *
 * Kept independent of PluginEditor so it can be unit-tested without a GUI.
 */
namespace dnaorbit::presets
{
    struct Preset
    {
        const char* name; // UTF-8; wrap with dnaorbit::ui::jp() before display.
        float rateHz;
        bool  sync;
        int   division;
        float radius, depth, symmetry, twist, core;
        bool  nullCore;
        float mix;
        float output;
        bool  autoGain;
        // Matches the shipped product default (Parameters.h) for every
        // factory preset; not perceptually re-tuned per preset since that
        // needs real listening (see MANUAL_REQUIRED.md), not a guess.
        float stereoPreserve;
        float bassAnchorHz;
        // 0 = Natural (reproduces this engine's pre-Character sound
        // exactly), 1 = Vivid, 2 = Deep. Every current factory preset keeps
        // Natural; not perceptually re-tuned per preset without listening.
        int character;
        // 0 = Free, 1 = Retrigger, 2 = Host Lock. Every current factory
        // preset keeps Free (this engine's original phase behaviour);
        // Host Lock needs host transport to demonstrate, so it is left as
        // an explicit user choice rather than a preset default.
        int   phaseMode;
        float startPhaseDeg;
        bool  clockwise;
        // Every factory preset applies with Soft Bypass off and Mono
        // Preview off. Picking a preset is meant to audition its sound,
        // never to silently leave you bypassed or monitoring in mono.
        bool  softBypass;
        bool  monoPreview;
    };

    // Intensity re-tuned from measurements (Tools/PresetIntensityAnalysis.cpp)
    // after the original values proved too timid to hear: "ボーカルを広げる"
    // in particular sat at Mix 30% / Core 10%, measuring a width
    // (1 - L/R correlation) of just 0.045 - very nearly mono. Mix is the
    // dominant lever (width rises monotonically 0.078 -> 0.485 across the
    // Mix range) and Core works against it (0.378 at Core 0% down to 0.102
    // at Core 50%), so both moved. Depth and Twist barely affect width at
    // all; Rate does not affect any time-averaged metric, but does decide
    // whether a listener perceives a whole revolution inside a phrase, so
    // the slowest presets were nudged up. Character stays Natural
    // throughout: it changes tone rather than intensity, and unlike these
    // values it has no measured backing yet (see ADR-006 and
    // MANUAL_REQUIRED.md).
    inline const Preset presets[] = {
        { "ボーカルを広げる", 0.13f, false, 2,  85.0f, 65.0f, 100.0f, 5.0f,  5.0f, false, 62.0f, 0.0f, true, 70.0f, 120.0f, 0, 0, 0.0f, true, false, false },
        { "パッドを回す",     0.18f, false, 2, 100.0f, 85.0f, 100.0f, 8.0f,  5.0f, false, 70.0f, 0.0f, true, 70.0f, 120.0f, 0, 0, 0.0f, true, false, false },
        { "ギターに揺らぎ",   0.11f, false, 2,  90.0f, 75.0f,  88.0f, 7.0f, 10.0f, false, 60.0f, 0.0f, true, 70.0f, 120.0f, 0, 0, 0.0f, true, false, false },
        { "シンセを速く回す", 0.60f, false, 2, 100.0f, 85.0f, 100.0f, 9.0f,  0.0f, false, 65.0f, 0.0f, true, 70.0f, 120.0f, 0, 0, 0.0f, true, false, false },
        // NULL CORE removes the Wet signal's Mid content entirely, which
        // Auto Gain's makeup formula does not model (it assumes a normal
        // strand+core reconstruction) - measured ~2.9dB quiet in stereo at
        // this Mix, and much quieter still once folded to mono (the entire
        // point of this preset). A flat output trim was tried to close the
        // stereo gap, but since it boosts the mono fold-down by the exact
        // same amount, it directly undoes the mono-cancellation the preset
        // exists to demonstrate - so the ~2.9dB stereo dip is left as an
        // honest, measured consequence of removing Mid content, not masked.
        { "実験:中心を消す", 0.06f, false, 2, 100.0f, 70.0f, 100.0f, 8.0f,  0.0f, true,  60.0f, 0.0f, true, 70.0f, 120.0f, 0, 0, 0.0f, true, false, false },
    };

    inline constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

    /**
     * Sets every parameter from the preset, so the resulting sound never
     * depends on state the preset didn't explicitly specify.
     *
     * If undoManagerForOneStep is non-null, every parameter this preset
     * touches is grouped into a single Undo step (a new transaction is
     * opened once, before any parameter changes) - without this, undoing a
     * preset application would require one Ctrl+Z per parameter, since
     * APVTS otherwise starts a fresh transaction boundary only when told to.
     * Left null (the default) for GUI-independent unit testing, where no
     * UndoManager exists at all.
     */
    inline void apply (juce::AudioProcessorValueTreeState& apvts, const Preset& preset,
                        juce::UndoManager* undoManagerForOneStep = nullptr)
    {
        if (undoManagerForOneStep != nullptr)
            undoManagerForOneStep->beginNewTransaction (juce::String (juce::CharPointer_UTF8 (preset.name)));

        auto set = [&apvts] (const char* id, float actualValue)
        {
            if (auto* parameter = apvts.getParameter (id))
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
        };

        set (params::rateID, preset.rateHz);
        set (params::syncID, preset.sync ? 1.0f : 0.0f);
        set (params::divisionID, (float) preset.division);
        set (params::radiusID, preset.radius);
        set (params::depthID, preset.depth);
        set (params::symmetryID, preset.symmetry);
        set (params::twistID, preset.twist);
        set (params::coreID, preset.core);
        set (params::nullCoreID, preset.nullCore ? 1.0f : 0.0f);
        set (params::mixID, preset.mix);
        set (params::outputID, preset.output);
        set (params::autoGainID, preset.autoGain ? 1.0f : 0.0f);
        set (params::stereoPreserveID, preset.stereoPreserve);
        set (params::bassAnchorHzID, preset.bassAnchorHz);
        set (params::characterID, (float) preset.character);
        set (params::phaseModeID, (float) preset.phaseMode);
        set (params::startPhaseID, preset.startPhaseDeg);
        set (params::directionID, preset.clockwise ? 0.0f : 1.0f);
        set (params::softBypassID, preset.softBypass ? 1.0f : 0.0f);
        set (params::monoPreviewID, preset.monoPreview ? 1.0f : 0.0f);
    }

    /**
     * True if every parameter currently sits at the preset's stored value
     * (within a small tolerance for float round-trip through the
     * normalised 0-1 range). Used to drive a "Modified" indicator: once the
     * user nudges anything after picking a preset, this goes false.
     */
    inline bool matchesCurrentState (const juce::AudioProcessorValueTreeState& apvts, const Preset& preset) noexcept
    {
        auto isClose = [&apvts] (const char* id, float actual, float tolerance = 0.05f)
        {
            auto* raw = apvts.getRawParameterValue (id);
            return raw != nullptr && std::abs (raw->load() - actual) <= tolerance;
        };
        auto isOn = [&apvts] (const char* id, bool actual)
        {
            auto* raw = apvts.getRawParameterValue (id);
            return raw != nullptr && (raw->load() > 0.5f) == actual;
        };

        return isClose (params::rateID, preset.rateHz)
            && isOn (params::syncID, preset.sync)
            && isClose (params::divisionID, (float) preset.division, 0.5f)
            && isClose (params::radiusID, preset.radius)
            && isClose (params::depthID, preset.depth)
            && isClose (params::symmetryID, preset.symmetry)
            && isClose (params::twistID, preset.twist)
            && isClose (params::coreID, preset.core)
            && isOn (params::nullCoreID, preset.nullCore)
            && isClose (params::mixID, preset.mix)
            && isClose (params::outputID, preset.output)
            && isOn (params::autoGainID, preset.autoGain)
            && isClose (params::stereoPreserveID, preset.stereoPreserve)
            && isClose (params::bassAnchorHzID, preset.bassAnchorHz, 1.0f)
            && isClose (params::characterID, (float) preset.character, 0.5f)
            && isClose (params::phaseModeID, (float) preset.phaseMode, 0.5f)
            && isClose (params::startPhaseID, preset.startPhaseDeg, 1.0f)
            && isOn (params::directionID, ! preset.clockwise) // choice index 1 (CCW) == "on"
            && isOn (params::softBypassID, preset.softBypass)
            && isOn (params::monoPreviewID, preset.monoPreview);
    }
}
