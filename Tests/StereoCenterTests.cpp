#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;

    struct StereoRun
    {
        juce::AudioBuffer<float> output { 2, blockSize };
    };

    StereoRun runLOnly (float preserve01, bool autoGain)
    {
        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.7f;
        p.radius01 = 0.95f;
        p.depth01 = 0.8f;
        p.symmetry01 = 1.0f;
        p.twistMs = 9.0f;
        p.core01 = 0.25f;
        p.mix01 = 1.0f;
        p.autoGain = autoGain;
        p.stereoPreserve01 = preserve01;
        engine.primeParameters (p);
        engine.setParameters (p);

        StereoRun result;
        juce::Random random { 20260802 };

        for (int block = 0; block < 40; ++block)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const float s = 0.25f * (random.nextFloat() * 2.0f - 1.0f);
                result.output.setSample (0, n, s);
                result.output.setSample (1, n, 0.0f);
            }
            engine.process (result.output, 2);
        }

        return result;
    }

    StereoRun runPureSide (float preserve01, bool autoGain, bool nullCore)
    {
        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 1.1f;
        p.radius01 = 1.0f;
        p.depth01 = 1.0f;
        p.symmetry01 = 1.0f;
        p.twistMs = 20.0f;
        p.core01 = 1.0f;
        p.mix01 = 1.0f;
        p.autoGain = autoGain;
        p.nullCore = nullCore;
        p.stereoPreserve01 = preserve01;
        engine.primeParameters (p);
        engine.setParameters (p);

        StereoRun result;
        juce::Random random { 424242 };

        for (int block = 0; block < 30; ++block)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                result.output.setSample (0, n, s);
                result.output.setSample (1, n, -s);
            }
            engine.process (result.output, 2);
        }

        return result;
    }

    double rms (const juce::AudioBuffer<float>& buffer, int channel)
    {
        double sum = 0.0;
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const double s = buffer.getSample (channel, n);
            sum += s * s;
        }
        return std::sqrt (sum / (double) buffer.getNumSamples());
    }

    class StereoCenterTests : public juce::UnitTest
    {
    public:
        StereoCenterTests() : juce::UnitTest ("StereoCenter", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Stereo Preserve changes only the stationary Side bed, not the moving Mid orbit");
            {
                const auto legacy = runLOnly (0.0f, false);
                const auto preserved = runLOnly (1.0f, false);

                // For L-only input: M = 0.5L and S = 0.5L. The moving Mid orbit
                // is identical in both runs, so their difference must be exactly
                // the stationary Side bed: +0.5L on the left and -0.5L on the right.
                // The two runs use the same deterministic source sequence and phase.
                juce::Random random { 20260802 };
                juce::AudioBuffer<float> lastInput (2, blockSize);
                for (int block = 0; block < 40; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.25f * (random.nextFloat() * 2.0f - 1.0f);
                        lastInput.setSample (0, n, s);
                        lastInput.setSample (1, n, 0.0f);
                    }
                }

                for (int n = 0; n < blockSize; ++n)
                {
                    const float expectedSide = 0.5f * lastInput.getSample (0, n);
                    const float deltaL = preserved.output.getSample (0, n) - legacy.output.getSample (0, n);
                    const float deltaR = preserved.output.getSample (1, n) - legacy.output.getSample (1, n);
                    expectWithinAbsoluteError (deltaL, expectedSide, 2.0e-5f,
                                               "Preserve must add only +Side to L");
                    expectWithinAbsoluteError (deltaR, -expectedSide, 2.0e-5f,
                                               "Preserve must add only -Side to R");
                }
            }

            beginTest ("Pure Side input stays perfectly energy-centred");
            {
                const auto result = runPureSide (1.0f, true, false);
                expectWithinAbsoluteError (rms (result.output, 0), rms (result.output, 1), 1.0e-6,
                                           "A pure Side bed must have equal L/R energy");

                for (int n = 0; n < blockSize; ++n)
                {
                    const float mid = 0.5f * (result.output.getSample (0, n)
                                            + result.output.getSample (1, n));
                    expectWithinAbsoluteError (mid, 0.0f, 1.0e-6f,
                                               "A pure Side bed must have exactly zero Mid");
                }
            }

            beginTest ("Geometry Auto Gain does not boost the preserved Side bed");
            {
                const auto off = runPureSide (0.7f, false, false);
                const auto on  = runPureSide (0.7f, true, false);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < blockSize; ++n)
                        expectWithinAbsoluteError (off.output.getSample (ch, n), on.output.getSample (ch, n), 1.0e-6f,
                                                   "Side preservation must remain independent of Mid-orbit makeup gain");
            }

            beginTest ("NULL CORE keeps the complete Wet definition Side-only");
            {
                const auto result = runPureSide (0.7f, true, true);
                for (int n = 0; n < blockSize; ++n)
                {
                    const float mid = 0.5f * (result.output.getSample (0, n)
                                            + result.output.getSample (1, n));
                    expectWithinAbsoluteError (mid, 0.0f, 1.0e-6f,
                                               "NULL CORE Wet must remain exactly Side-only");
                }
            }
        }
    };

    static StereoCenterTests stereoCenterTests;
}
