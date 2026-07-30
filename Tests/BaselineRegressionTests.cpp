#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

/**
 * Numeric fingerprints of the current (schema 1, mono-derived-Wet) DSP
 * behaviour, captured before the Stereo Preserve rewrite (commercial upgrade
 * Phase 2). These are regression pins, not behaviour specs: once Phase 2
 * lands, running the same signals with stereoPreserve=0 must reproduce these
 * exact numbers, proving the legacy/migrated path is bit-for-bit unchanged
 * for anyone reloading a project saved before Stereo Preserve existed.
 *
 * If Phase 2 intentionally changes the stereoPreserve=0 path, these numbers
 * must be re-derived deliberately (see docs/commercial-upgrade/09_decision
 * log) rather than updated silently to make a failing test pass.
 */
namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 256;

    double rmsOf (const juce::AudioBuffer<float>& buffer, int channel)
    {
        double sum = 0.0;
        const auto* data = buffer.getReadPointer (channel);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            sum += (double) data[n] * data[n];
        return std::sqrt (sum / (double) buffer.getNumSamples());
    }

    float peakOf (const juce::AudioBuffer<float>& buffer, int channel)
    {
        float peak = 0.0f;
        const auto* data = buffer.getReadPointer (channel);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            peak = juce::jmax (peak, std::abs (data[n]));
        return peak;
    }

    /** Deterministic fingerprint: not a perceptual measure, just a stable checksum. */
    double fingerprintOf (const juce::AudioBuffer<float>& buffer)
    {
        double acc = 0.0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                acc += (double) data[n] * (double) (n + 1) * (ch == 0 ? 1.0 : -1.0);
        }
        return acc;
    }

    struct Fingerprint { double rmsL, rmsR, peakL, peakR, checksum; };

    Fingerprint runScenario (bool stereoIn, bool antiPhase, bool useNoise)
    {
        HelixEngine engine;
        engine.prepare (kSampleRate, kBlockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.3f;
        p.radius01 = 0.8f;
        p.depth01 = 0.55f;
        p.symmetry01 = 1.0f;
        p.twistMs = 5.0f;
        p.core01 = 0.1f;
        p.mix01 = 1.0f;
        p.outputDb = 0.0f;
        p.autoGain = true;
        // Explicit, not relying on the struct default: this is what makes
        // these scenarios a schema-1/legacy fingerprint (see the class
        // comment and Source/Parameters.h's stereoPreserveIntroducedInSchema).
        p.stereoPreserve01 = 0.0f;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, kBlockSize);
        juce::Random random { 99001 };

        // Warm up so smoothers/orbit settle into a representative state.
        for (int i = 0; i < 30; ++i)
        {
            for (int n = 0; n < kBlockSize; ++n)
            {
                const float s = useNoise ? 0.3f * (random.nextFloat() * 2.0f - 1.0f)
                                          : 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0
                                                                      * (double) (i * kBlockSize + n) / kSampleRate);
                buffer.setSample (0, n, s);
                buffer.setSample (1, n, stereoIn ? (antiPhase ? -s : s) : s);
            }
            engine.process (buffer, stereoIn ? 2 : 1);
        }

        return { rmsOf (buffer, 0), rmsOf (buffer, 1), peakOf (buffer, 0), peakOf (buffer, 1), fingerprintOf (buffer) };
    }

    void expectFingerprint (juce::UnitTest& test, const Fingerprint& actual, const Fingerprint& expected,
                            const juce::String& label)
    {
        test.expectWithinAbsoluteError (actual.rmsL, expected.rmsL, 1.0e-4, label + ": rmsL");
        test.expectWithinAbsoluteError (actual.rmsR, expected.rmsR, 1.0e-4, label + ": rmsR");
        test.expectWithinAbsoluteError ((double) actual.peakL, (double) expected.peakL, 1.0e-4, label + ": peakL");
        test.expectWithinAbsoluteError ((double) actual.peakR, (double) expected.peakR, 1.0e-4, label + ": peakR");
        test.expectWithinAbsoluteError (actual.checksum, expected.checksum, std::abs (expected.checksum) * 1.0e-4 + 1.0e-6,
                                        label + ": checksum");
    }

    class BaselineRegressionTests : public juce::UnitTest
    {
    public:
        BaselineRegressionTests() : juce::UnitTest ("BaselineRegression", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Mono input, sine - schema 1 baseline fingerprint");
            {
                // Captured from the actual current implementation - see class
                // comment. Re-derive deliberately, never to silence a failure.
                const Fingerprint expected { 0.348666, 0.363815, 0.509689, 0.530329, -21.5309 };
                expectFingerprint (*this, runScenario (false, false, false), expected, "mono/sine");
            }

            beginTest ("Correlated stereo input, pink-ish noise - schema 1 baseline fingerprint");
            {
                const Fingerprint expected { 0.105009, 0.11667, 0.274519, 0.299682, 67.5208 };
                expectFingerprint (*this, runScenario (true, false, true), expected, "stereo-correlated/noise");
            }

            beginTest ("Anti-phase stereo input (L=-R) - schema 1 baseline fingerprint");
            {
                // The whole point of Phase 2 is to change THIS scenario's
                // outcome (today the wet signal built from 0.5*(L+R) is exactly
                // zero for perfectly anti-phase input). Pinning it here proves
                // that claim numerically and gives Phase 2 an explicit,
                // intentional line to cross rather than an assumption.
                const Fingerprint actual = runScenario (true, true, false);
                expectWithinAbsoluteError (actual.rmsL, 0.0, 1.0e-6,
                                            "Schema 1: anti-phase stereo input collapses Wet to exact silence (L)");
                expectWithinAbsoluteError (actual.rmsR, 0.0, 1.0e-6,
                                            "Schema 1: anti-phase stereo input collapses Wet to exact silence (R)");
            }
        }
    };

    static BaselineRegressionTests baselineRegressionTests;
}
