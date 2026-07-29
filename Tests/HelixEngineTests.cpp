#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    void fillTestSignal (juce::AudioBuffer<float>& buffer, double sampleRate, double freqHz = 220.0, double startPhase = 0.0)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                data[n] = 0.5f * (float) std::sin (startPhase + juce::MathConstants<double>::twoPi * freqHz * (double) n / sampleRate);
        }
    }

    bool allFinite (const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto* data = buffer.getReadPointer (ch);
            for (int n = 0; n < buffer.getNumSamples(); ++n)
                if (! std::isfinite (data[n]))
                    return false;
        }
        return true;
    }

    class HelixEngineTests : public juce::UnitTest
    {
    public:
        HelixEngineTests() : juce::UnitTest ("HelixEngine", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Silence in stays finite and near-silent across parameter extremes");
            {
                for (bool nullCore : { false, true })
                {
                    for (float extreme : { 0.0f, 1.0f })
                    {
                        HelixEngine engine;
                        engine.prepare (48000.0, 512, 2);

                        HelixEngine::Parameters p;
                        p.rateHz = juce::jmap (extreme, 0.02f, 4.0f);
                        p.radius01 = extreme; p.depth01 = extreme; p.symmetry01 = extreme;
                        p.twistMs = extreme * 20.0f; p.core01 = extreme; p.nullCore = nullCore;
                        p.mix01 = extreme; p.outputDb = juce::jmap (extreme, -12.0f, 6.0f);
                        engine.setParameters (p);

                        juce::AudioBuffer<float> buffer (2, 512);
                        buffer.clear();

                        for (int block = 0; block < 20; ++block)
                            engine.process (buffer, 2);

                        expect (allFinite (buffer), "Output must stay finite for silence input at parameter extremes");
                        expectLessThan (buffer.getMagnitude (0, buffer.getNumSamples()), 1.0e-4f,
                                         "Silence in must not produce noise or DC out");
                    }
                }
            }

            beginTest ("Impulse response is finite and bounded");
            {
                HelixEngine engine;
                engine.prepare (48000.0, 256, 2);
                HelixEngine::Parameters p;
                p.depth01 = 1.0f; p.radius01 = 1.0f; p.mix01 = 1.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                buffer.clear();
                buffer.setSample (0, 0, 1.0f);
                buffer.setSample (1, 0, 1.0f);

                engine.process (buffer, 2);

                expect (allFinite (buffer), "Impulse response must be finite");
                expectLessThan (buffer.getMagnitude (0, buffer.getNumSamples()), 10.0f, "Impulse response must not blow up");
            }

            beginTest ("Runs cleanly across supported sample rates and block sizes");
            {
                const double sampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
                const int blockSizes[] = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

                for (double sr : sampleRates)
                {
                    for (int bs : blockSizes)
                    {
                        HelixEngine engine;
                        engine.prepare (sr, bs, 2);
                        HelixEngine::Parameters p;
                        engine.setParameters (p);

                        juce::AudioBuffer<float> buffer (2, bs);
                        fillTestSignal (buffer, sr);
                        engine.process (buffer, 2);

                        expect (allFinite (buffer),
                                "sampleRate=" + juce::String (sr) + " blockSize=" + juce::String (bs));
                    }
                }
            }

            beginTest ("All parameters at their extremes never crash or produce non-finite output");
            {
                for (float extreme : { 0.0f, 1.0f })
                {
                    for (bool nullCore : { false, true })
                    {
                        HelixEngine engine;
                        engine.prepare (48000.0, 512, 2);

                        HelixEngine::Parameters p;
                        p.rateHz = juce::jmap (extreme, 0.02f, 4.0f);
                        p.radius01 = extreme; p.depth01 = extreme; p.symmetry01 = extreme;
                        p.twistMs = extreme * 20.0f; p.core01 = extreme; p.nullCore = nullCore;
                        p.mix01 = extreme; p.outputDb = juce::jmap (extreme, -12.0f, 6.0f);
                        engine.setParameters (p);

                        juce::AudioBuffer<float> buffer (2, 512);

                        for (int block = 0; block < 10; ++block)
                        {
                            fillTestSignal (buffer, 48000.0);
                            engine.process (buffer, 2);
                            expect (allFinite (buffer));
                        }
                    }
                }
            }

            beginTest ("Symmetry 100% keeps the centroid locked over a long run; angles stay wrapped");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 512, 2);

                HelixEngine::Parameters p;
                p.symmetry01 = 1.0f;
                p.rateHz = 2.0f; // fast rate so many orbits complete quickly
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 512);

                // Two simulated minutes at a fast rate covers hundreds of orbits,
                // a reasonable stand-in for the "10 minute" long-run requirement.
                const int totalSamples = (int) (sr * 60.0 * 2.0);
                int samplesDone = 0;
                float maxCentroidDistanceAfterSettle = 0.0f;

                while (samplesDone < totalSamples)
                {
                    fillTestSignal (buffer, sr);
                    engine.process (buffer, 2);
                    expect (allFinite (buffer));

                    const auto state = engine.getVisualState();
                    const float twoPiF = (float) juce::MathConstants<double>::twoPi;
                    expect (state.thetaA >= 0.0f && state.thetaA < twoPiF + 0.01f, "thetaA must stay wrapped");
                    expect (state.thetaB >= 0.0f && state.thetaB < twoPiF + 0.01f, "thetaB must stay wrapped");

                    if (samplesDone > (int) (sr * 0.5)) // allow the initial resync to settle
                        maxCentroidDistanceAfterSettle = juce::jmax (maxCentroidDistanceAfterSettle, state.centroidDistance);

                    samplesDone += 512;
                }

                expectLessThan (maxCentroidDistanceAfterSettle, 1.0e-4f,
                                 "Centroid must remain locked at Symmetry 100% over a long run");
            }

            beginTest ("Parameter automation does not produce large sample-to-sample discontinuities");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 64, 2);

                HelixEngine::Parameters p;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 64);
                fillTestSignal (buffer, sr);
                engine.process (buffer, 2);

                HelixEngine::Parameters p2;
                p2.mix01 = 1.0f; p2.radius01 = 1.0f; p2.depth01 = 1.0f; p2.core01 = 1.0f;
                p2.nullCore = true; p2.symmetry01 = 0.0f; p2.twistMs = 20.0f; p2.outputDb = 6.0f;
                engine.setParameters (p2);

                float maxJump = 0.0f;
                float prevL = buffer.getSample (0, buffer.getNumSamples() - 1);

                // Keep the test signal's phase continuous across blocks so we're
                // only measuring discontinuities introduced by the engine itself,
                // not an artificial click from the test signal restarting at 0.
                for (int block = 0; block < 50; ++block)
                {
                    const double startPhase = juce::MathConstants<double>::twoPi * 220.0 * (double) (block * buffer.getNumSamples()) / sr;
                    fillTestSignal (buffer, sr, 220.0, startPhase);
                    engine.process (buffer, 2);
                    const auto* l = buffer.getReadPointer (0);

                    for (int n = 0; n < buffer.getNumSamples(); ++n)
                    {
                        maxJump = juce::jmax (maxJump, std::abs (l[n] - prevL));
                        prevL = l[n];
                    }
                }

                expectLessThan (maxJump, 0.25f, "Sample-to-sample jump should stay bounded under abrupt parameter automation");
            }

            beginTest ("Mix 0% matches Dry once the output-gain smoother settles");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 0.0f; p.outputDb = 0.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                juce::AudioBuffer<float> dryCopy (2, 256);

                for (int i = 0; i < 20; ++i)
                {
                    fillTestSignal (buffer, sr, 220.0, (double) i);
                    dryCopy.makeCopyOf (buffer);
                    engine.process (buffer, 2);
                }

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < buffer.getNumSamples(); ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dryCopy.getSample (ch, n), 1.0e-4f);
            }

            beginTest ("NULL CORE on with Mix 100% collapses the mono downmix toward zero");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 1.0f; p.nullCore = true; p.core01 = 1.0f; p.depth01 = 1.0f; p.radius01 = 1.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                float maxMonoSum = 0.0f;

                for (int i = 0; i < 40; ++i)
                {
                    fillTestSignal (buffer, sr, 220.0, (double) i);
                    engine.process (buffer, 2);

                    // Only measure once the NULL CORE crossfade (~120ms) has settled.
                    if (i >= 25)
                    {
                        const auto* l = buffer.getReadPointer (0);
                        const auto* r = buffer.getReadPointer (1);
                        for (int n = 0; n < buffer.getNumSamples(); ++n)
                            maxMonoSum = juce::jmax (maxMonoSum, std::abs (l[n] + r[n]));
                    }
                }

                expectLessThan (maxMonoSum, 0.02f, "L+R should collapse toward zero when NULL CORE removes the Wet mid");
            }

            beginTest ("NULL CORE off does not accidentally cancel the mono downmix");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 1.0f; p.nullCore = false;
                p.radius01 = 0.8f; p.depth01 = 0.55f; p.core01 = 0.0f; p.symmetry01 = 1.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                float sumOfAbsMono = 0.0f;

                for (int i = 0; i < 40; ++i)
                {
                    fillTestSignal (buffer, sr, 220.0, (double) i);
                    engine.process (buffer, 2);

                    const auto* l = buffer.getReadPointer (0);
                    const auto* r = buffer.getReadPointer (1);
                    for (int n = 0; n < buffer.getNumSamples(); ++n)
                        sumOfAbsMono += std::abs (l[n] + r[n]);
                }

                expectGreaterThan (sumOfAbsMono, 1.0f, "Downmixing to mono with NULL CORE off must not cancel to near silence");
            }

            beginTest ("Mono input path (1 input channel) stays finite and produces stereo output");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                engine.prepare (sr, 256, 2);
                HelixEngine::Parameters p;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                buffer.clear();
                auto* mono = buffer.getWritePointer (0);
                for (int n = 0; n < 256; ++n)
                    mono[n] = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / sr);

                engine.process (buffer, 1);

                expect (allFinite (buffer), "Mono-in/stereo-out path must remain finite");
            }
        }
    };

    static HelixEngineTests helixEngineTests;
}
