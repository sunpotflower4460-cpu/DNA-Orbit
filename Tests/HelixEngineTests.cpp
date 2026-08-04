#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <limits>
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
                // 1, 2 and 3 matter as much as the powers of two: the DSP
                // spec asks for "block size 1 or the minimum supported,
                // through 4096", and hosts genuinely do hand out one-sample
                // buffers when splitting a block around a sample-accurate
                // automation point. Anything computed once per block (the
                // Host Lock phase correction, the control-rate Character
                // and Bass Anchor updates) is at its most exposed there,
                // and an odd size like 3 also catches loops that quietly
                // assume an even count.
                const int blockSizes[] = { 1, 2, 3, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

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

            beginTest ("Returning Symmetry to 100% resyncs the centroid within ~300ms, with no angle jump");
            {
                HelixEngine engine;
                const double sr = 48000.0;
                const int blockSize = 32;
                engine.prepare (sr, blockSize, 2);

                // Drift B away from the ideal 180 degrees for a while at Symmetry 0%.
                HelixEngine::Parameters drifting;
                drifting.symmetry01 = 0.0f;
                drifting.rateHz = 1.0f;
                engine.setParameters (drifting);

                juce::AudioBuffer<float> buffer (2, blockSize);

                for (int i = 0; i < (int) (sr * 3.0) / blockSize; ++i)
                {
                    fillTestSignal (buffer, sr);
                    engine.process (buffer, 2);
                }

                const float centroidBeforeResync = engine.getVisualState().centroidDistance;
                expectGreaterThan (centroidBeforeResync, 0.02f, "Symmetry 0% must have visibly drifted the centroid before we test resync");

                // Now ask for full symmetry again and track how long it takes to
                // relock, watching for any single-sample angle discontinuity.
                HelixEngine::Parameters locked;
                locked.symmetry01 = 1.0f;
                locked.rateHz = 1.0f;
                engine.setParameters (locked);

                double prevThetaBAfterSwitch = engine.getVisualState().thetaB;
                bool sawJump = false;
                int samplesToRelock = -1;
                const int maxSamplesToCheck = (int) (sr * 1.0); // generous outer bound
                int samplesChecked = 0;

                while (samplesChecked < maxSamplesToCheck)
                {
                    fillTestSignal (buffer, sr);
                    engine.process (buffer, 2);
                    samplesChecked += blockSize;

                    const auto state = engine.getVisualState();

                    // thetaB should never leap by more than a small fraction of a
                    // radian between consecutive polls (one block apart here).
                    const double jump = std::abs (dnaorbit::orbitmath::shortestAngleDelta (prevThetaBAfterSwitch, (double) state.thetaB));
                    const double maxExpectedPerBlock = dnaorbit::orbitmath::angularIncrement (4.0, sr) * blockSize * 4.0; // generous margin over the fastest orbit rate
                    if (jump > maxExpectedPerBlock)
                        sawJump = true;
                    prevThetaBAfterSwitch = state.thetaB;

                    if (samplesToRelock < 0 && state.centroidDistance < 0.01f)
                        samplesToRelock = samplesChecked;
                }

                expect (! sawJump, "thetaB must never leap discontinuously while resyncing");
                expect (samplesToRelock > 0, "The centroid must relock within the outer bound");

                const double relockSeconds = (double) samplesToRelock / sr;
                expectLessThan (relockSeconds, 0.35, "Resync should complete within roughly the 100-300ms spec window");
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

            beginTest ("process() called before prepare() is a safe no-op, not a crash");
            {
                // juce::dsp::DelayLine's internal buffer has zero channels until
                // prepare() sizes it; AudioBuffer::setSample's bounds check is
                // assertion-only, so this used to write through an invalid
                // channel pointer in a Release build if a host ever violated the
                // prepare-before-process contract.
                HelixEngine engine;
                HelixEngine::Parameters p;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                fillTestSignal (buffer, 48000.0);
                juce::AudioBuffer<float> before;
                before.makeCopyOf (buffer);

                engine.process (buffer, 2);

                expect (allFinite (buffer), "Must not corrupt memory or produce non-finite output");
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 256; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), before.getSample (ch, n), 1.0e-9f,
                                                    "Buffer must be left untouched (pass-through) when unprepared");

                // And a subsequent, correct prepare()+process() must work normally.
                engine.prepare (48000.0, 256, 2);
                engine.setParameters (p);
                fillTestSignal (buffer, 48000.0);
                engine.process (buffer, 2);
                expect (allFinite (buffer), "Must process normally once properly prepared");
            }

            beginTest ("primeParameters() snaps immediately instead of fading in from silence");
            {
                // Every SmoothedValue defaults to a current value of 0. Without
                // primeParameters(), the very first block after prepare() would
                // ramp Output/Mix/etc. up from that 0 over the smoothing window,
                // audibly fading in from silence on every plugin load or
                // sample-rate change regardless of the host's actual settings.
                HelixEngine engine;
                engine.prepare (48000.0, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 1.0f;
                p.outputDb = 0.0f; // unity gain: output should be at full level immediately
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                // A sine starting at phase 0 is exactly 0 at sample 0 regardless
                // of any fade-in, which would defeat this test - use a cosine
                // (peak amplitude at sample 0) instead.
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < 256; ++n)
                        data[n] = 0.5f * (float) std::cos (juce::MathConstants<double>::twoPi * 220.0 * n / 48000.0);
                }
                engine.process (buffer, 2);

                // The very first sample of the very first block must already be
                // near full level, not near zero.
                expectGreaterThan (std::abs (buffer.getSample (0, 0)), 0.05f,
                                    "First sample must not be faded in from silence after primeParameters()");
            }

            beginTest ("Without priming, the first block visibly ramps up from silence (documents the bug this fixes)");
            {
                // Same setup as above but using setParameters() alone (no
                // primeParameters()), which is the pre-fix behaviour. This pins
                // the contrast so a future regression that removes the
                // primeParameters() call is caught.
                HelixEngine engine;
                engine.prepare (48000.0, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 1.0f;
                p.outputDb = 0.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                // Same cosine signal as the primed test above, so the only
                // difference between the two tests is the primeParameters() call.
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < 256; ++n)
                        data[n] = 0.5f * (float) std::cos (juce::MathConstants<double>::twoPi * 220.0 * n / 48000.0);
                }
                engine.process (buffer, 2);

                expectLessThan (std::abs (buffer.getSample (0, 0)), 1.0e-3f,
                                 "Without priming the very first sample should still be near zero (ramping from the SmoothedValue default)");
            }

            beginTest ("A non-finite input sample does not permanently poison the filter state");
            {
                // The two one-pole filters are recursive: state depends on the
                // previous sample. A single NaN/Inf sample used to latch that
                // state to NaN forever, since nothing downstream could ever
                // clear it.
                HelixEngine engine;
                engine.prepare (48000.0, 64, 2);
                HelixEngine::Parameters p;
                p.depth01 = 1.0f; p.mix01 = 1.0f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 64);
                buffer.clear();
                buffer.setSample (0, 0, std::numeric_limits<float>::quiet_NaN());
                buffer.setSample (1, 0, std::numeric_limits<float>::infinity());
                engine.process (buffer, 2);

                // Recover with clean silence afterward.
                bool recovered = false;
                for (int block = 0; block < 20; ++block)
                {
                    buffer.clear();
                    engine.process (buffer, 2);
                    if (allFinite (buffer))
                        recovered = true;
                    else
                        recovered = false;
                }

                expect (recovered, "Filter state must recover to finite output after a non-finite input sample");
            }

            beginTest ("A non-finite parameter value falls back safely instead of latching theta to NaN");
            {
                HelixEngine engine;
                engine.prepare (48000.0, 256, 2);

                HelixEngine::Parameters p;
                p.rateHz = std::numeric_limits<float>::quiet_NaN();
                p.radius01 = std::numeric_limits<float>::infinity();
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, 256);
                fillTestSignal (buffer, 48000.0);

                for (int block = 0; block < 10; ++block)
                {
                    fillTestSignal (buffer, 48000.0);
                    engine.process (buffer, 2);
                    expect (allFinite (buffer), "Non-finite parameters must not propagate into the output");
                }

                const auto state = engine.getVisualState();
                expect (std::isfinite (state.thetaA) && std::isfinite (state.thetaB),
                        "Non-finite rate/radius parameters must not latch the orbit angles to NaN");
            }
        }
    };

    static HelixEngineTests helixEngineTests;
}
