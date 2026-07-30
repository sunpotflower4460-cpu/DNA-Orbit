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
    };

    inline const Preset presets[] = {
        { "ボーカルを広げる", 0.10f, false, 2,  75.0f, 45.0f, 100.0f, 4.0f, 10.0f, false, 30.0f, 0.0f, true, 70.0f, 120.0f, 0 },
        { "パッドを回す",     0.18f, false, 2, 100.0f, 65.0f, 100.0f, 7.0f, 10.0f, false, 45.0f, 0.0f, true, 70.0f, 120.0f, 0 },
        { "ギターに揺らぎ",   0.08f, false, 2,  80.0f, 60.0f,  88.0f, 6.0f, 15.0f, false, 40.0f, 0.0f, true, 70.0f, 120.0f, 0 },
        { "シンセを速く回す", 0.60f, false, 2,  90.0f, 70.0f, 100.0f, 8.0f,  0.0f, false, 40.0f, 0.0f, true, 70.0f, 120.0f, 0 },
        { "実験:中心を消す", 0.04f, false, 2, 100.0f, 50.0f, 100.0f, 8.0f,  0.0f, true,  30.0f, 0.0f, true, 70.0f, 120.0f, 0 },
    };

    inline constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

    /**
     * Sets every one of the 12 parameters from the preset, so the resulting
     * sound never depends on state the preset didn't explicitly specify.
     */
    inline void apply (juce::AudioProcessorValueTreeState& apvts, const Preset& preset)
    {
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
            && isClose (params::characterID, (float) preset.character, 0.5f);
    }
}
