#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include "Presets.h"
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    HelixEngine::Parameters toEngineParameters (const dnaorbit::presets::Preset& preset)
    {
        HelixEngine::Parameters p;
        p.rateHz           = preset.rateHz;
        p.radius01         = preset.radius / 100.0f;
        p.depth01          = preset.depth / 100.0f;
        p.symmetry01       = preset.symmetry / 100.0f;
        p.twistMs          = preset.twist;
        p.core01           = preset.core / 100.0f;
        p.nullCore         = preset.nullCore;
        p.mix01            = preset.mix / 100.0f;
        p.outputDb         = preset.output;
        p.autoGain         = preset.autoGain;
        p.stereoPreserve01 = preset.stereoPreserve / 100.0f;
        p.bassAnchorHz     = preset.bassAnchorHz;
        p.character        = preset.character;
        p.phaseMode        = preset.phaseMode;
        p.startPhaseDeg    = preset.startPhaseDeg;
        p.clockwise        = preset.clockwise;
        p.softBypass       = preset.softBypass;
        p.monoPreview      = preset.monoPreview;
        return p;
    }

    class PresetIntensityTests : public juce::UnitTest
    {
    public:
        PresetIntensityTests() : juce::UnitTest ("PresetIntensity", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Every factory preset produces a measurably audible stereo width, not a near-mono result");
            {
                // Regression guard for the "presets feel too subtle" report
                // that led to ADR-010's re-tuning: this pins a minimum
                // width (1 - L/R correlation) for every factory preset on a
                // mono-centred four-note pad, so a future edit cannot
                // silently walk Mix/Core back down to a barely-audible
                // level the way the original shipped values did (measured
                // 0.045 for "ボーカルを広げる" before ADR-010 - see
                // Tools/PresetIntensityAnalysis.cpp for the full sweep this
                // threshold is based on).
                constexpr double sampleRate = 48000.0;
                constexpr int blockSize = 512;
                constexpr double durationSeconds = 12.0;
                constexpr double minimumWidth = 0.15;

                const int numSamples = (int) (durationSeconds * sampleRate);
                const double freqs[] = { 261.63, 329.63, 392.00, 523.25 };

                for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
                {
                    const auto& preset = dnaorbit::presets::presets[i];

                    juce::AudioBuffer<float> source (2, numSamples);
                    {
                        auto* l = source.getWritePointer (0);
                        auto* r = source.getWritePointer (1);
                        for (int n = 0; n < numSamples; ++n)
                        {
                            double s = 0.0;
                            for (double f : freqs)
                                s += 0.09 * std::sin (juce::MathConstants<double>::twoPi * f * (double) n / sampleRate);
                            l[n] = (float) s;
                            r[n] = (float) s;
                        }
                    }

                    HelixEngine engine;
                    engine.prepare (sampleRate, blockSize, 2);
                    auto params = toEngineParameters (preset);
                    engine.primeParameters (params);

                    juce::AudioBuffer<float> wet (2, numSamples);
                    for (int start = 0; start < numSamples; start += blockSize)
                    {
                        const int n = std::min (blockSize, numSamples - start);
                        juce::AudioBuffer<float> block (2, n);
                        block.copyFrom (0, 0, source, 0, start, n);
                        block.copyFrom (1, 0, source, 1, start, n);
                        engine.process (block, 2);
                        wet.copyFrom (0, start, block, 0, 0, n);
                        wet.copyFrom (1, start, block, 1, 0, n);
                    }

                    // Skip the first second so smoothers/filters have settled.
                    const int analysisStart = (int) sampleRate;
                    const auto* wl = wet.getReadPointer (0);
                    const auto* wr = wet.getReadPointer (1);

                    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
                    for (int n = analysisStart; n < numSamples; ++n)
                    {
                        sumLL += (double) wl[n] * wl[n];
                        sumRR += (double) wr[n] * wr[n];
                        sumLR += (double) wl[n] * wr[n];
                    }

                    const double denom = std::sqrt (sumLL * sumRR);
                    const double correlation = denom > 1.0e-12 ? juce::jlimit (-1.0, 1.0, sumLR / denom) : 1.0;
                    const double width = 1.0 - correlation;

                    const juce::String presetName (juce::CharPointer_UTF8 (preset.name));
                    expectGreaterThan (width, minimumWidth,
                        "Preset \"" + presetName + "\" measured width " + juce::String (width, 3)
                            + " - too close to mono to be an audible effect");
                }
            }
        }
    };

    static PresetIntensityTests presetIntensityTests;
}
