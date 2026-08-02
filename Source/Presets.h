#pragma once

#include <cmath>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"

/**
 * Factory presets define the complete audio state so applying one never
 * depends on values left behind by a previous preset or host automation.
 */
namespace dnaorbit::presets
{
    struct Preset
    {
        const char* name;
        float rateHz;
        bool  sync;
        int   division;
        float radius, depth, symmetry, twist, core;
        bool  nullCore;
        float mix;
        float output;
        bool  autoGain;
        float stereoPreserve;
        bool  softBypass;
    };

    inline const Preset presets[] = {
        { "ボーカルを広げる", 0.10f, false, 2,  75.0f, 45.0f, 100.0f, 4.0f, 10.0f, false, 30.0f, 0.0f, true, 70.0f, false },
        { "パッドを回す",     0.18f, false, 2, 100.0f, 65.0f, 100.0f, 7.0f, 10.0f, false, 45.0f, 0.0f, true, 70.0f, false },
        { "ギターに揺らぎ",   0.08f, false, 2,  80.0f, 60.0f,  88.0f, 6.0f, 15.0f, false, 40.0f, 0.0f, true, 70.0f, false },
        { "シンセを速く回す", 0.60f, false, 2,  90.0f, 70.0f, 100.0f, 8.0f,  0.0f, false, 40.0f, 0.0f, true, 70.0f, false },
        { "実験:中心を消す", 0.04f, false, 2, 100.0f, 50.0f, 100.0f, 8.0f,  0.0f, true,  30.0f, 0.0f, true, 70.0f, false },
    };

    inline constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

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
        set (params::softBypassID, preset.softBypass ? 1.0f : 0.0f);
    }

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
            && isOn (params::softBypassID, preset.softBypass);
    }
}
