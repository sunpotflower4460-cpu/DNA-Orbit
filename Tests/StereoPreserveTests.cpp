#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    double rmsOf (const juce::AudioBuffer<float>& buffer, int channel)
    {
        double sum = 0.0;
        const auto* data = buffer.getReadPointer (channel);
        for (int n = 0; n < buffer.getNumSamples(); ++n)
            sum += (double) data[n] * data[n];
        return std::sqrt (sum / (double) buffer.getNumSamples());
    }

    /** Pure Side input: L = -R, so the moving Mid orbit receives silence. */
    juce::AudioBuffer<float> runAntiPhase (float stereoPreserve01, int sampleRate = 48000, int blockSize = 256)
    {
        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

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
        p.stereoPreserve01 = stereoPreserve01;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::Random random { 99001 };

        for (int i = 0; i < 30; ++i)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                buffer.setSample (0, n, s);
                buffer.setSample (1, n, -s);
            }
            engine.process (buffer, 2);
        }

        return buffer;
    }

    class StereoPreserveTests : public juce::UnitTest
    {
    public:
        StereoPreserveTests() : juce::UnitTest ("StereoPreserve", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Stereo Preserve 0% reproduces the legacy anti-phase silence path");
            {
                const auto buffer = runAntiPhase (0.0f);
                expectLessThan (rmsOf (buffer, 0), 1.0e-5, "Legacy L Wet must remain silent for pure Side input");
                expectLessThan (rmsOf (buffer, 1), 1.0e-5, "Legacy R Wet must remain silent for pure Side input");
            }

            beginTest ("Stereo Preserve above 0% keeps pure Side material audible");
            {
                const auto buffer = runAntiPhase (0.7f);
                expectGreaterThan (rmsOf (buffer, 0), 0.02, "L must be audible once original Side is retained");
                expectGreaterThan (rmsOf (buffer, 1), 0.02, "R must be audible once original Side is retained");
            }

            beginTest ("Pure Side Wet level scales linearly with Stereo Preserve");
            {
                // With L=-R, Mid is exactly zero. The complete Wet is therefore
                // only the stationary Side bed, whose gain is p by definition.
                const auto full = runAntiPhase (1.0f);
                const auto half = runAntiPhase (0.5f);
                const auto quarter = runAntiPhase (0.25f);

                const double rmsFull = rmsOf (full, 0);
                expectGreaterThan (rmsFull, 0.01, "Full Side preservation must be audible");

                expectWithinAbsoluteError (rmsOf (half, 0), rmsFull * 0.5, rmsFull * 0.02,
                                            "50% must retain half the pure-Side RMS");
                expectWithinAbsoluteError (rmsOf (quarter, 0), rmsFull * 0.25, rmsFull * 0.02,
                                            "25% must retain one quarter of the pure-Side RMS");
            }

            beginTest ("Mono input is unaffected because Side is zero");
            {
                HelixEngine engineLow, engineHigh;
                engineLow.prepare (48000.0, 256, 2);
                engineHigh.prepare (48000.0, 256, 2);

                HelixEngine::Parameters pLow, pHigh;
                pLow.radius01 = pHigh.radius01 = 0.8f;
                pLow.depth01 = pHigh.depth01 = 0.55f;
                pLow.core01 = pHigh.core01 = 0.2f;
                pLow.mix01 = pHigh.mix01 = 1.0f;
                pLow.stereoPreserve01 = 0.0f;
                pHigh.stereoPreserve01 = 1.0f;
                engineLow.primeParameters (pLow);
                engineLow.setParameters (pLow);
                engineHigh.primeParameters (pHigh);
                engineHigh.setParameters (pHigh);

                juce::AudioBuffer<float> bufferLow (2, 256);
                juce::AudioBuffer<float> bufferHigh (2, 256);
                juce::Random random { 4242 };

                for (int i = 0; i < 20; ++i)
                {
                    for (int n = 0; n < 256; ++n)
                    {
                        const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        bufferLow.setSample (0, n, s);
                        bufferLow.setSample (1, n, s);
                        bufferHigh.setSample (0, n, s);
                        bufferHigh.setSample (1, n, s);
                    }
                    engineLow.process (bufferLow, 2);
                    engineHigh.process (bufferHigh, 2);
                }

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 256; ++n)
                        expectWithinAbsoluteError (bufferLow.getSample (ch, n), bufferHigh.getSample (ch, n), 1.0e-5f,
                                                    "Mono input must sound identical at every Preserve value");
            }

            beginTest ("Stereo Preserve parameter changes are smoothed");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 64, 2);

                HelixEngine::Parameters p;
                p.stereoPreserve01 = 0.0f;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 64);
                int blockIndex = 0;
                auto fillAntiPhase = [&]
                {
                    const double startPhase = juce::MathConstants<double>::twoPi * 220.0 * (double) (blockIndex * 64) / sr;
                    for (int n = 0; n < 64; ++n)
                    {
                        const float s = 0.3f * (float) std::sin (startPhase + juce::MathConstants<double>::twoPi * 220.0 * n / sr);
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, -s);
                    }
                    ++blockIndex;
                };

                fillAntiPhase();
                engine.process (buffer, 2);

                HelixEngine::Parameters p2 = p;
                p2.stereoPreserve01 = 1.0f;
                engine.setParameters (p2);

                float maxJump = 0.0f;
                float prevL = buffer.getSample (0, buffer.getNumSamples() - 1);

                for (int block = 0; block < 20; ++block)
                {
                    fillAntiPhase();
                    engine.process (buffer, 2);
                    const auto* l = buffer.getReadPointer (0);

                    for (int n = 0; n < buffer.getNumSamples(); ++n)
                    {
                        maxJump = juce::jmax (maxJump, std::abs (l[n] - prevL));
                        prevL = l[n];
                    }
                }

                expectLessThan (maxJump, 0.25f,
                                 "Abrupt Preserve automation must not create a large sample discontinuity");
            }
        }
    };

    static StereoPreserveTests stereoPreserveTests;
}
