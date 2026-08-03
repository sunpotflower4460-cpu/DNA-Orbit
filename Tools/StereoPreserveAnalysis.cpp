/**
 * Phase 2.5 measurement harness: prints Wet RMS/L-R balance/mono-sum/
 * correlation for a fixed set of input conditions across Stereo Preserve
 * values, using the real HelixEngine (not a re-implementation), so the
 * numbers in docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md
 * are reproducible by re-running this tool rather than taken on faith.
 *
 * The energy-weighted-centre bias claim (Strand/Core depend only on Mid, so
 * their contribution is exactly channel-symmetric regardless of Stereo
 * Preserve) is a structural proof, not a per-condition measurement - see
 * Tests/StereoPreserveTests.cpp's "Energy-weighted centre" test, which
 * verifies it directly against the engine. It is not re-measured here.
 *
 * Usage: StereoPreserveAnalysis (no arguments; prints a table to stdout)
 */

#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"
#include <cstdio>
#include <functional>
#include <vector>

using namespace dnaorbit::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 256;
    // rateHz below is 2.0 (0.5s/orbit); this covers several full orbit
    // periods so the L/R balance reflects a genuine long-term average
    // rather than a snapshot at one particular orbit phase (a much shorter
    // window left a stray ~0.3dB artifact even for perfectly symmetric
    // dual-mono input, which is not a DSP bug - just an averaging window
    // too short relative to the orbit period).
    constexpr int    kBlocks     = 400;

    struct Metrics
    {
        double rmsL = 0.0, rmsR = 0.0, lrDiffDb = 0.0, monoSumRms = 0.0, correlation = 0.0;
    };

    Metrics run (float stereoPreserve01, const std::function<float (double)>& left,
                 const std::function<float (double)>& right)
    {
        HelixEngine engine;
        engine.prepare (kSampleRate, kBlockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 2.0f;
        p.radius01 = 0.8f;
        p.depth01 = 0.55f;
        p.symmetry01 = 1.0f;
        p.twistMs = 5.0f;
        p.core01 = 0.1f;
        p.mix01 = 1.0f;
        p.outputDb = 0.0f;
        p.autoGain = true;
        p.stereoPreserve01 = stereoPreserve01;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, kBlockSize);
        double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0, sumMonoAbs = 0.0;
        long long totalSamples = 0;
        double t = 0.0;
        const double dt = 1.0 / kSampleRate;

        for (int block = 0; block < kBlocks; ++block)
        {
            for (int n = 0; n < kBlockSize; ++n)
            {
                buffer.setSample (0, n, left (t));
                buffer.setSample (1, n, right (t));
                t += dt;
            }
            engine.process (buffer, 2);

            // Only accumulate stats once smoothers/orbit have settled.
            if (block >= 10)
            {
                for (int n = 0; n < kBlockSize; ++n)
                {
                    const double l = buffer.getSample (0, n);
                    const double r = buffer.getSample (1, n);
                    sumLL += l * l;
                    sumRR += r * r;
                    sumLR += l * r;
                    sumMonoAbs += std::abs (l + r);
                    ++totalSamples;
                }
            }
        }

        Metrics m;
        m.rmsL = std::sqrt (sumLL / (double) totalSamples);
        m.rmsR = std::sqrt (sumRR / (double) totalSamples);
        m.lrDiffDb = 20.0 * std::log10 ((m.rmsL + 1.0e-12) / (m.rmsR + 1.0e-12));
        m.monoSumRms = sumMonoAbs / (double) totalSamples;
        const double denom = std::sqrt (sumLL * sumRR);
        m.correlation = denom > 1.0e-12 ? sumLR / denom : 1.0;
        return m;
    }

    struct Condition
    {
        const char* name;
        std::function<float (double)> left;
        std::function<float (double)> right;
    };
}

int main()
{
    juce::Random random { 20260730 };

    // Independent noise generators for the "uncorrelated stereo" condition -
    // captured by value into the lambdas below via shared_ptr-free static
    // buffers keyed by a simple LCG so the tool has no external dependencies.
    auto noiseAt = [] (juce::Random& rng) { return 0.3f * (rng.nextFloat() * 2.0f - 1.0f); };

    std::vector<Condition> conditions;

    conditions.push_back ({ "dual mono (L=R)",
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); },
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); } });

    conditions.push_back ({ "L-only",
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); },
        [] (double) { return 0.0f; } });

    conditions.push_back ({ "R-only",
        [] (double) { return 0.0f; },
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); } });

    conditions.push_back ({ "L/R 6dB difference",
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); },
        [] (double t) { return (float) (0.3 * std::pow (10.0, -6.0 / 20.0) * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); } });

    {
        auto rngL = std::make_shared<juce::Random> (111);
        auto rngR = std::make_shared<juce::Random> (222);
        conditions.push_back ({ "uncorrelated stereo",
            [rngL, noiseAt] (double) mutable { return noiseAt (*rngL); },
            [rngR, noiseAt] (double) mutable { return noiseAt (*rngR); } });
    }

    conditions.push_back ({ "anti-phase (L=-R)",
        [] (double t) { return (float) (0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); },
        [] (double t) { return (float) (-0.3 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)); } });

    {
        // "Wide pad": correlated but decorrelated-ish - two detuned sines,
        // like a chorused pad's L/R spread, plus a shared low-level noise bed.
        auto rngPadL = std::make_shared<juce::Random> (333);
        auto rngPadR = std::make_shared<juce::Random> (444);
        conditions.push_back ({ "wide stereo pad",
            [rngPadL] (double t) mutable
            {
                return (float) (0.22 * std::sin (juce::MathConstants<double>::twoPi * 220.0 * t)
                              + 0.05 * (rngPadL->nextFloat() * 2.0 - 1.0));
            },
            [rngPadR] (double t) mutable
            {
                return (float) (0.22 * std::sin (juce::MathConstants<double>::twoPi * 220.6 * t)
                              + 0.05 * (rngPadR->nextFloat() * 2.0 - 1.0));
            } });
    }

    const float presets[] = { 0.0f, 0.25f, 0.5f, 0.7f, 1.0f };

    std::printf ("%-22s %6s %9s %9s %9s %9s %9s\n",
                 "condition", "p", "rmsL", "rmsR", "L-R(dB)", "monoSum", "corr");
    std::printf ("--------------------------------------------------------------------------------\n");

    for (const auto& cond : conditions)
    {
        for (float p : presets)
        {
            const auto m = run (p, cond.left, cond.right);
            std::printf ("%-22s %6.2f %9.5f %9.5f %9.3f %9.5f %9.3f\n",
                         cond.name, p, m.rmsL, m.rmsR, m.lrDiffDb, m.monoSumRms, m.correlation);
        }
        std::printf ("\n");
    }

    return 0;
}
