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

    /** Runs a fixed anti-phase (L = -R) scenario at a given Stereo Preserve amount. */
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
            beginTest ("Stereo Preserve 0% still collapses anti-phase input to near silence (legacy path unaffected)");
            {
                const auto buffer = runAntiPhase (0.0f);
                expectLessThan (rmsOf (buffer, 0), 1.0e-5, "L must stay silent at Stereo Preserve 0%, anti-phase in");
                expectLessThan (rmsOf (buffer, 1), 1.0e-5, "R must stay silent at Stereo Preserve 0%, anti-phase in");
            }

            beginTest ("Stereo Preserve above 0% keeps anti-phase Wet audible (the anti-phase-silence fix)");
            {
                // 70% is the shipped default for new instances.
                const auto buffer = runAntiPhase (0.7f);
                expectGreaterThan (rmsOf (buffer, 0), 0.02, "L must be clearly audible once Stereo Preserve keeps Side content");
                expectGreaterThan (rmsOf (buffer, 1), 0.02, "R must be clearly audible once Stereo Preserve keeps Side content");
            }

            beginTest ("Anti-phase Wet level scales linearly with Stereo Preserve");
            {
                // With L = -R, Mid is exactly 0 for every sample, so sourceA/B
                // are pure multiples of Side by stereoPreserve - and every
                // stage downstream (filters, delays, pan gains, the geometry-
                // only Auto Gain makeup, the dry/wet mix) is linear and does
                // not reference stereoPreserve itself, so the whole chain
                // scales exactly linearly with it in this scenario.
                const auto full = runAntiPhase (1.0f);
                const auto half = runAntiPhase (0.5f);
                const auto quarter = runAntiPhase (0.25f);

                const double rmsFull = rmsOf (full, 0);
                expectGreaterThan (rmsFull, 0.01, "Test setup: full Stereo Preserve must be clearly audible");

                expectWithinAbsoluteError (rmsOf (half, 0), rmsFull * 0.5, rmsFull * 0.02,
                                            "RMS at 50% Stereo Preserve must be half of RMS at 100%");
                expectWithinAbsoluteError (rmsOf (quarter, 0), rmsFull * 0.25, rmsFull * 0.02,
                                            "RMS at 25% Stereo Preserve must be a quarter of RMS at 100%");
            }

            beginTest ("Mono input is unaffected by Stereo Preserve (Side is always 0)");
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
                        bufferLow.setSample (1, n, s); // mono: duplicated, not just numInputChannels==1
                        bufferHigh.setSample (0, n, s);
                        bufferHigh.setSample (1, n, s);
                    }
                    engineLow.process (bufferLow, 2);
                    engineHigh.process (bufferHigh, 2);
                }

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 256; ++n)
                        expectWithinAbsoluteError (bufferLow.getSample (ch, n), bufferHigh.getSample (ch, n), 1.0e-5f,
                                                    "Mono (perfectly correlated L/R) input must sound identical at any Stereo Preserve value");
            }

            beginTest ("Stereo Preserve 100% keeps the output tied to its originating channel");
            {
                // Radius 0 and Depth 0 remove the strand pan/back-gain's
                // dependence on the orbit angle entirely (pan collapses to
                // dead centre, back attenuation/delay collapse to none), so
                // only the Stereo Preserve bed (see the dedicated
                // energy-weighted-centre test below) can make L and R differ
                // here - Strand A/B and Core are always Mid-fed, hence always
                // symmetric, regardless of Stereo Preserve (see ADR-004).
                for (bool preserveFully : { false, true })
                {
                    const float preserve = preserveFully ? 1.0f : 0.0f;

                    HelixEngine engine;
                    engine.prepare (48000.0, 256, 2);

                    HelixEngine::Parameters p;
                    p.radius01 = 0.0f;
                    p.depth01 = 0.0f;
                    p.twistMs = 0.0f;
                    p.core01 = 1.0f;
                    p.mix01 = 1.0f;
                    p.autoGain = false;
                    p.stereoPreserve01 = preserve;
                    engine.primeParameters (p);
                    engine.setParameters (p);

                    juce::AudioBuffer<float> buffer (2, 256);
                    buffer.clear();
                    auto* l = buffer.getWritePointer (0);
                    for (int n = 0; n < 256; ++n)
                        l[n] = 1.0f; // constant: only L is driven, R stays 0
                    // Channel 1 (R) stays at 0 from buffer.clear() above.

                    engine.process (buffer, 2);

                    const float outL = buffer.getSample (0, 255);
                    const float outR = buffer.getSample (1, 255);

                    if (! preserveFully)
                    {
                        expectWithinAbsoluteError (outL, outR, 0.01f,
                                                    "At Stereo Preserve 0%, L-only input must not favour the L output channel");
                    }
                    else
                    {
                        expectGreaterThan (outL - outR, 0.3f,
                                            "At Stereo Preserve 100%, L-only input must clearly favour the L output channel");
                    }
                }
            }

            beginTest ("Energy-weighted centre: L-only and R-only give identical Strand+Core content at any Stereo Preserve");
            {
                // Phase 2.5 re-verification (see ADR-004): staying
                // GEOMETRICALLY antipodal is not the same guarantee as
                // staying ENERGY-balanced. Under an earlier design
                // (sourceA = M + p*S, sourceB = M - p*S fed directly to each
                // strand), L-only input made Strand A's own source far
                // louder than Strand B's, so even though the two strands'
                // POSITIONS stayed exactly opposite, the perceptually-
                // weighted centre drifted toward whichever strand carried
                // more energy. Feeding both strands (and Core) from Mid only
                // removes this failure mode by construction: L-only and
                // R-only share an IDENTICAL Mid signal (0.5*(L+R) is the same
                // whether L or R carries the signal), so Strand A/B/Core's
                // contribution must be bit-for-bit identical between the two
                // scenarios - only the Stereo Preserve bed (built from Side,
                // which flips sign between L-only and R-only) may differ,
                // and by an exact, predictable amount.
                for (float preserve : { 0.0f, 0.25f, 0.5f, 0.7f, 1.0f })
                {
                    HelixEngine::Parameters params;
                    params.rateHz = 0.3f;
                    params.radius01 = 0.8f;
                    params.depth01 = 0.55f;
                    params.symmetry01 = 1.0f;
                    params.twistMs = 5.0f;
                    params.core01 = 0.3f;
                    params.mix01 = 1.0f;
                    params.outputDb = 0.0f;
                    // Auto Gain off: isolates this measurement from the
                    // correlation-aware Mix Law, which - unlike everything
                    // else in the engine - IS legitimately signal-adaptive,
                    // so it can differ between the L-only and R-only runs
                    // (their Dry signals differ) even though Strand/Core
                    // themselves do not. That is tested on its own merits
                    // elsewhere, not here.
                    params.autoGain = false;
                    params.stereoPreserve01 = preserve;

                    HelixEngine engineLOnly, engineROnly;
                    engineLOnly.prepare (48000.0, 256, 2);
                    engineROnly.prepare (48000.0, 256, 2);
                    engineLOnly.primeParameters (params);
                    engineLOnly.setParameters (params);
                    engineROnly.primeParameters (params);
                    engineROnly.setParameters (params);

                    juce::AudioBuffer<float> bufferLOnly (2, 256), bufferROnly (2, 256);
                    std::vector<float> sValues (256);
                    juce::Random random { 5150 };

                    double maxError = 0.0;

                    for (int block = 0; block < 20; ++block)
                    {
                        for (int n = 0; n < 256; ++n)
                        {
                            const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                            sValues[(size_t) n] = s;
                            bufferLOnly.setSample (0, n, s);
                            bufferLOnly.setSample (1, n, 0.0f);
                            bufferROnly.setSample (0, n, 0.0f);
                            bufferROnly.setSample (1, n, s);
                        }

                        engineLOnly.process (bufferLOnly, 2);
                        engineROnly.process (bufferROnly, 2);

                        for (int n = 0; n < 256; ++n)
                        {
                            // side(L-only) = 0.5*s, side(R-only) = -0.5*s, so
                            // bedL(L-only) - bedL(R-only) == preserve * s.
                            // Strand+Core cancel exactly (identical Mid), so
                            // this predicts the FULL output difference.
                            const double expected = (double) preserve * sValues[(size_t) n];
                            const double actual = (double) bufferLOnly.getSample (0, n)
                                                 - (double) bufferROnly.getSample (0, n);
                            maxError = juce::jmax (maxError, std::abs (actual - expected));
                        }
                    }

                    expectLessThan (maxError, 1.0e-4,
                                     "L-only vs R-only output difference must match the predicted bed contribution exactly "
                                     "(Strand+Core must be energy-identical, not just position-antipodal), at Stereo Preserve = "
                                     + juce::String (preserve));
                }
            }

            beginTest ("Stereo Preserve parameter changes are smoothed, not a hard jump");
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
                // A continuous-phase sine, like HelixEngineTests's equivalent
                // test: white noise would itself jump by up to 2x its
                // amplitude sample-to-sample regardless of any parameter
                // smoothing, which would measure the test signal's own
                // jaggedness rather than anything the engine introduces.
                auto fillAntiPhase = [&] ()
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
                engine.setParameters (p2); // abrupt jump 0 -> 1, must ramp rather than step

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
                                 "Sample-to-sample jump should stay bounded when Stereo Preserve is automated abruptly");
            }
        }
    };

    static StereoPreserveTests stereoPreserveTests;
}
