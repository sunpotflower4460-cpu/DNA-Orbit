#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int    kBlockSize  = 256;

    /**
     * Deterministic test signal (fixed seed, so results are reproducible).
     *
     * The noise mode generates PINK noise, not white. This matters: the Depth
     * low-pass has unity DC gain but removes real energy from a broadband
     * signal, and white noise puts half its power above 12 kHz where the
     * filter bites hardest. Pink noise (-3 dB/octave) is far closer to the
     * spectrum of actual programme material, so it is the honest yardstick
     * for a "does the loudness stay matched" test.
     */
    struct SignalGenerator
    {
        juce::Random random { 20240729 };
        double phase = 0.0;
        bool useNoise = true;

        // Paul Kellet's economy pink-noise filter state, one set per channel.
        struct PinkState { float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f; };
        PinkState pink[2];

        float nextPink (PinkState& s)
        {
            const float white = random.nextFloat() * 2.0f - 1.0f;
            s.b0 = 0.99765f * s.b0 + white * 0.0990460f;
            s.b1 = 0.96300f * s.b1 + white * 0.2965164f;
            s.b2 = 0.57000f * s.b2 + white * 1.0526913f;
            return (s.b0 + s.b1 + s.b2 + white * 0.1848f) * 0.20f;
        }

        void fill (juce::AudioBuffer<float>& buffer, bool correlatedStereo)
        {
            const int n = buffer.getNumSamples();
            for (int i = 0; i < n; ++i)
            {
                float sample;
                if (useNoise)
                {
                    sample = nextPink (pink[0]);
                }
                else
                {
                    sample = 0.4f * (float) std::sin (phase);
                    phase += juce::MathConstants<double>::twoPi * 220.0 / kSampleRate;
                    if (phase > juce::MathConstants<double>::twoPi)
                        phase -= juce::MathConstants<double>::twoPi;
                }

                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, correlatedStereo ? sample : nextPink (pink[1]));
            }
        }
    };

    double rmsOf (const juce::AudioBuffer<float>& buffer)
    {
        double sum = 0.0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const double s = buffer.getSample (ch, n);
                sum += s * s;
            }
        return std::sqrt (sum / (double) (buffer.getNumChannels() * buffer.getNumSamples()));
    }

    /**
     * Runs the engine at Mix 100% and returns the output/input RMS ratio in dB,
     * averaged over enough time to cover many full orbits.
     */
    double measureWetGainDb (const HelixEngine::Parameters& params, bool useNoise, bool correlatedStereo)
    {
        HelixEngine engine;
        engine.prepare (kSampleRate, kBlockSize, 2);
        engine.setParameters (params);

        SignalGenerator generator;
        generator.useNoise = useNoise;

        juce::AudioBuffer<float> buffer (2, kBlockSize);

        // Warm up: let every smoother settle before measuring.
        for (int i = 0; i < 40; ++i)
        {
            generator.fill (buffer, correlatedStereo);
            engine.process (buffer, 2);
        }

        double inputPower = 0.0, outputPower = 0.0;
        int blocks = 0;

        // ~4 seconds; at rateHz = 2 that is 8 full orbits, so the rotation averages out.
        const int totalBlocks = (int) (kSampleRate * 4.0 / kBlockSize);
        juce::AudioBuffer<float> inputCopy (2, kBlockSize);

        for (int i = 0; i < totalBlocks; ++i)
        {
            generator.fill (buffer, correlatedStereo);
            inputCopy.makeCopyOf (buffer);
            engine.process (buffer, 2);

            const double inRms = rmsOf (inputCopy);
            const double outRms = rmsOf (buffer);
            inputPower += inRms * inRms;
            outputPower += outRms * outRms;
            ++blocks;
        }

        if (blocks == 0 || inputPower <= 0.0)
            return -999.0;

        return 20.0 * std::log10 (std::sqrt (outputPower / inputPower));
    }

    class LevelMatchTests : public juce::UnitTest
    {
    public:
        LevelMatchTests() : juce::UnitTest ("LevelMatch", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Auto gain keeps Mix 100% within +/-2 dB of the input across a parameter sweep");
            {
                const float radii[]  = { 0.0f, 0.5f, 1.0f };
                const float depths[] = { 0.0f, 0.55f, 1.0f };
                const float twists[] = { 0.0f, 5.0f, 20.0f };
                const float cores[]  = { 0.0f, 0.5f, 1.0f };

                double worstDb = 0.0;
                juce::String worstCase;

                for (float radius : radii)
                    for (float depth : depths)
                        for (float twist : twists)
                            for (float core : cores)
                            {
                                HelixEngine::Parameters p;
                                p.mix01 = 1.0f;
                                p.rateHz = 2.0f;
                                p.radius01 = radius;
                                p.depth01 = depth;
                                p.twistMs = twist;
                                p.core01 = core;
                                p.autoGain = true;
                                p.nullCore = false;

                                const double db = measureWetGainDb (p, true, true);

                                if (std::abs (db) > std::abs (worstDb))
                                {
                                    worstDb = db;
                                    worstCase = "radius=" + juce::String (radius)
                                              + " depth=" + juce::String (depth)
                                              + " twist=" + juce::String (twist)
                                              + " core=" + juce::String (core);
                                }
                            }

                logMessage ("Worst deviation with auto gain ON: " + juce::String (worstDb, 2)
                            + " dB at " + worstCase);
                expectLessThan (std::abs (worstDb), 2.0,
                                 "Auto gain should hold the wet level within 2 dB of the input");
            }

            beginTest ("Auto gain OFF leaves the original (quieter) wet level");
            {
                HelixEngine::Parameters p;
                p.mix01 = 1.0f;
                p.rateHz = 2.0f;
                p.autoGain = false;

                const double db = measureWetGainDb (p, true, true);
                logMessage ("Wet level with auto gain OFF at defaults: " + juce::String (db, 2) + " dB");

                expectLessThan (db, -3.0, "With auto gain off the wet path should still be clearly quieter");
            }

            beginTest ("Auto gain ON is measurably louder than OFF at the same settings");
            {
                HelixEngine::Parameters on;
                on.mix01 = 1.0f; on.rateHz = 2.0f; on.autoGain = true;
                HelixEngine::Parameters off = on;
                off.autoGain = false;

                const double dbOn = measureWetGainDb (on, true, true);
                const double dbOff = measureWetGainDb (off, true, true);

                expectGreaterThan (dbOn - dbOff, 2.0, "Auto gain should raise the wet level meaningfully");
            }

            beginTest ("A tonal source is also held within +/-2.5 dB");
            {
                HelixEngine::Parameters p;
                p.mix01 = 1.0f;
                p.rateHz = 2.0f;
                p.autoGain = true;

                const double db = measureWetGainDb (p, false, true);
                logMessage ("Tonal (220 Hz sine) wet level with auto gain ON: " + juce::String (db, 2) + " dB");

                // Slightly wider bound: a pure tone stays correlated across the twist
                // delay, so it sits at the coherent end of the estimate.
                expectLessThan (std::abs (db), 2.5);
            }

            beginTest ("Wide (uncorrelated) stereo input is quieter by a bounded amount");
            {
                HelixEngine::Parameters p;
                p.mix01 = 1.0f;
                p.rateHz = 2.0f;
                p.autoGain = true;

                const double db = measureWetGainDb (p, true, false);
                logMessage ("Uncorrelated stereo input wet level: " + juce::String (db, 2) + " dB");

                // Documented limitation: the wet path is built from the mono downmix,
                // which is inherently ~3 dB down for uncorrelated input, and the
                // compensation is deliberately signal-independent so it cannot pump.
                expectLessThan (std::abs (db), 5.0);
            }

            beginTest ("Auto gain's accuracy across Stereo Preserve (the bed is deliberately NOT compensated)");
            {
                // Strand/Core stay Mid-only regardless of Stereo Preserve (see
                // ADR-004), so the existing geometry-only Auto Gain formula
                // remains exactly as accurate for them at any Stereo Preserve
                // value as it always was at 0%. The Stereo Preserve bed rides
                // on top, deliberately uncompensated (a signal-dependent
                // correction would risk pumping - see the wetMakeup comment in
                // HelixEngine.cpp). This logs the real, measured deviation
                // rather than asserting an untested number, so a future
                // change that makes this meaningfully worse is caught here.
                const float preserves[] = { 0.0f, 0.25f, 0.5f, 0.7f, 1.0f };
                double worstDb = 0.0;
                juce::String worstCase;

                for (float preserve : preserves)
                {
                    for (bool correlatedStereo : { true, false })
                    {
                        HelixEngine::Parameters p;
                        p.mix01 = 1.0f;
                        p.rateHz = 2.0f;
                        p.radius01 = 0.8f;
                        p.depth01 = 0.55f;
                        p.twistMs = 5.0f;
                        p.core01 = 0.1f;
                        p.autoGain = true;
                        p.stereoPreserve01 = preserve;

                        const double db = measureWetGainDb (p, true, correlatedStereo);
                        logMessage ("stereoPreserve=" + juce::String (preserve, 2)
                                    + (correlatedStereo ? " correlated" : " uncorrelated")
                                    + ": " + juce::String (db, 2) + " dB");

                        if (std::abs (db) > std::abs (worstDb))
                        {
                            worstDb = db;
                            worstCase = "stereoPreserve=" + juce::String (preserve, 2)
                                      + (correlatedStereo ? " correlated" : " uncorrelated");
                        }
                    }
                }

                logMessage ("Worst deviation across Stereo Preserve: " + juce::String (worstDb, 2)
                            + " dB at " + worstCase);
                // Wider bound than the 0%-only sweep above: the bed is
                // measured, not guaranteed, to stay within this. If a future
                // change needs a wider bound, widen it deliberately with a
                // comment, not silently.
                expectLessThan (std::abs (worstDb), 4.0,
                                 "Stereo Preserve's uncompensated bed should not push Auto Gain's deviation "
                                 "far beyond its 0%-only bound");
            }

            beginTest ("Correlation-aware Mix Law prevents a loudness bump at Mix 50% for correlated Dry/Wet");
            {
                // Low Radius/Depth (minimal strand decorrelation) and high
                // Core make Wet closely resemble Dry - the scenario where the
                // plain equal-power law's classic +3dB bump around Mix 50%
                // would be most audible without this correction.
                HelixEngine::Parameters p;
                p.rateHz = 2.0f;
                p.radius01 = 0.1f;
                p.depth01 = 0.1f;
                p.core01 = 0.9f;
                p.twistMs = 0.0f;
                p.autoGain = true;

                p.mix01 = 0.0f;
                const double db0 = measureWetGainDb (p, true, true);
                p.mix01 = 0.5f;
                const double db50 = measureWetGainDb (p, true, true);
                p.mix01 = 1.0f;
                const double db100 = measureWetGainDb (p, true, true);

                logMessage ("Mix 0%: " + juce::String (db0, 2) + " dB, Mix 50%: " + juce::String (db50, 2)
                            + " dB, Mix 100%: " + juce::String (db100, 2) + " dB");

                expectLessThan (std::abs (db0), 0.1, "Test setup: Mix 0% must be exactly Dry");
                expectLessThan (db50, juce::jmax (db0, db100) + 1.5,
                                 "Mix 50% must not read meaningfully louder than both endpoints - "
                                 "that gap is the correlated-blend bump this correction exists to remove");
            }

            beginTest ("Correlation meter reports sane values for known signals");
            {
                struct Case { const char* name; bool mono; bool inverted; float expected; };

                // Mix 0% so the meter sees the input unchanged.
                for (auto& c : { Case { "mono", true, false, 1.0f },
                                 Case { "inverted", true, true, -1.0f } })
                {
                    HelixEngine engine;
                    engine.prepare (kSampleRate, kBlockSize, 2);
                    HelixEngine::Parameters p;
                    p.mix01 = 0.0f;
                    engine.setParameters (p);

                    juce::AudioBuffer<float> buffer (2, kBlockSize);
                    juce::Random random { 4242 };

                    for (int i = 0; i < 30; ++i)
                    {
                        for (int n = 0; n < kBlockSize; ++n)
                        {
                            const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                            buffer.setSample (0, n, s);
                            buffer.setSample (1, n, c.inverted ? -s : s);
                        }
                        engine.process (buffer, 2);
                    }

                    const float measured = engine.getVisualState().correlation;
                    logMessage (juce::String (c.name) + " correlation = " + juce::String (measured, 3));
                    expectWithinAbsoluteError (measured, c.expected, 0.05f);
                }
            }

            beginTest ("Output RMS meter tracks the signal and returns to zero for silence");
            {
                HelixEngine engine;
                engine.prepare (kSampleRate, kBlockSize, 2);
                HelixEngine::Parameters p;
                p.mix01 = 0.5f;
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, kBlockSize);
                SignalGenerator generator;

                for (int i = 0; i < 30; ++i)
                {
                    generator.fill (buffer, true);
                    engine.process (buffer, 2);
                }

                expectGreaterThan (engine.getVisualState().outputRms, 0.01f,
                                    "Meter should read a level while audio is playing");

                for (int i = 0; i < 30; ++i)
                {
                    buffer.clear();
                    engine.process (buffer, 2);
                }

                expectLessThan (engine.getVisualState().outputRms, 1.0e-4f,
                                 "Meter should fall back to zero for silence");
            }
        }
    };

    static LevelMatchTests levelMatchTests;
}
