#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"
#include "dsp/OrbitMath.h"

using namespace dnaorbit::dsp;

namespace
{
    class HostPhaseLockTests : public juce::UnitTest
    {
    public:
        HostPhaseLockTests() : juce::UnitTest ("HostPhaseLock", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Phase Mode = Free (default) is bit-identical to never having Host Phase Lock at all");
            {
                // Free must ignore hostPpqPosition/hostIsPlaying entirely -
                // this is what lets it need no schema-version bump.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;

                HelixEngine engineDefault, engineExplicit;
                engineDefault.prepare (sr, blockSize, 2);
                engineExplicit.prepare (sr, blockSize, 2);

                HelixEngine::Parameters pDefault;
                pDefault.rateHz = 0.4f; pDefault.radius01 = 0.7f; pDefault.mix01 = 1.0f;

                HelixEngine::Parameters pExplicit = pDefault;
                pExplicit.phaseMode = 0; // Free
                pExplicit.hostIsPlaying = true;
                pExplicit.hostPpqPosition = 123.456;
                pExplicit.hostCycleBeats = 3.0;
                pExplicit.startPhaseDeg = 90.0f;
                pExplicit.clockwise = false;

                engineDefault.primeParameters (pDefault);
                engineDefault.setParameters (pDefault);
                engineExplicit.primeParameters (pExplicit);
                engineExplicit.setParameters (pExplicit);

                juce::AudioBuffer<float> bufferDefault (2, blockSize), bufferExplicit (2, blockSize);
                juce::Random random { 303 };

                for (int block = 0; block < 20; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        bufferDefault.setSample (0, n, s); bufferDefault.setSample (1, n, s);
                        bufferExplicit.setSample (0, n, s); bufferExplicit.setSample (1, n, s);
                    }
                    engineDefault.process (bufferDefault, 2);
                    engineExplicit.process (bufferExplicit, 2);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < blockSize; ++n)
                            expectWithinAbsoluteError (bufferDefault.getSample (ch, n), bufferExplicit.getSample (ch, n), 1.0e-7f);
                }
            }

            beginTest ("Retrigger resets the orbit to Start Phase exactly when playback starts");
            {
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;

                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 0.0f; // isolate the reset from ordinary rate drift
                p.phaseMode = 1; // Retrigger
                p.startPhaseDeg = 90.0f;
                p.clockwise = true;
                p.hostIsPlaying = false;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                buffer.clear();

                // A few blocks stopped: nothing should reset yet (thetaA
                // stays at reset()'s initial 0).
                for (int i = 0; i < 3; ++i)
                    engine.process (buffer, 2);
                expectWithinAbsoluteError (engine.getVisualState().thetaA, 0.0f, 1.0e-4f);

                // Playback starts: thetaA must snap to Start Phase (90 deg = pi/2).
                p.hostIsPlaying = true;
                engine.setParameters (p);
                engine.process (buffer, 2);

                const float expected = (float) (dnaorbit::orbitmath::pi * 0.5);
                expectWithinAbsoluteError (engine.getVisualState().thetaA, expected, 1.0e-3f);

                // Staying "playing" must not re-trigger on the next block.
                buffer.clear();
                for (int n = 0; n < blockSize; ++n)
                    buffer.setSample (0, n, 0.3f), buffer.setSample (1, n, 0.3f);
                engine.process (buffer, 2); // rateHz == 0, so thetaA should still equal expected
                expectWithinAbsoluteError (engine.getVisualState().thetaA, expected, 1.0e-3f,
                                            "Retrigger must only fire on the false->true edge, not every playing block");
            }

            beginTest ("Host Lock converges to the host's PPQ-derived phase and then tracks it with no steady-state error");
            {
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;
                constexpr double cycleBeats = 4.0;
                constexpr float rateHz = 2.0f; // must match the simulated tempo below to avoid steady-state ramp-tracking error

                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = rateHz;
                p.phaseMode = 2; // Host Lock
                p.hostCycleBeats = cycleBeats;
                p.hostIsPlaying = true;
                p.mix01 = 1.0f;
                // Start 1 beat "into" the cycle - a deliberate initial
                // misalignment relative to thetaA's reset() value of 0, so
                // convergence is actually exercised rather than starting
                // already-aligned.
                double simulatedPpq = 1.0;
                p.hostPpqPosition = simulatedPpq;
                engine.primeParameters (p);
                engine.setParameters (p);

                // Beats advanced per block at rateHz, matching a host whose
                // tempo is consistent with rateHz and cycleBeats (the
                // realistic case: Host Lock used together with Sync).
                const double beatsPerBlock = cycleBeats * rateHz * (double) blockSize / sr;

                juce::AudioBuffer<float> buffer (2, blockSize);
                juce::Random random { 404 };

                double maxErrorAfterSettling = 0.0;

                for (int block = 0; block < 60; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                    engine.process (buffer, 2);

                    simulatedPpq += beatsPerBlock;
                    p.hostPpqPosition = simulatedPpq;
                    engine.setParameters (p);

                    if (block >= 40) // after ~213ms, well past the 30ms time constant
                    {
                        const double fraction = std::fmod (simulatedPpq / cycleBeats, 1.0);
                        const double targetTheta = (fraction < 0.0 ? fraction + 1.0 : fraction) * dnaorbit::orbitmath::twoPi;
                        const double error = std::abs (dnaorbit::orbitmath::shortestAngleDelta (
                            (double) engine.getVisualState().thetaA, targetTheta));
                        maxErrorAfterSettling = juce::jmax (maxErrorAfterSettling, error);
                    }
                }

                expectLessThan (maxErrorAfterSettling, 0.05,
                                 "After settling, thetaA must track the PPQ-derived target with negligible error");
            }

            beginTest ("Host Lock direction (CCW) reverses which way the phase advances with PPQ");
            {
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;
                constexpr double cycleBeats = 4.0;

                auto finalThetaAfterAdvancing = [&] (bool clockwise)
                {
                    HelixEngine engine;
                    engine.prepare (sr, blockSize, 2);

                    HelixEngine::Parameters p;
                    p.rateHz = 0.0f; // isolate: only the Host Lock correction moves thetaA
                    p.phaseMode = 2;
                    p.hostCycleBeats = cycleBeats;
                    p.hostIsPlaying = true;
                    p.clockwise = clockwise;
                    p.hostPpqPosition = 0.0;
                    engine.primeParameters (p);
                    engine.setParameters (p);

                    juce::AudioBuffer<float> buffer (2, blockSize);
                    buffer.clear();

                    // Advance to a quarter of the cycle and let it settle.
                    for (int block = 0; block < 30; ++block)
                    {
                        p.hostPpqPosition = juce::jmin (1.0, block * 0.05);
                        engine.setParameters (p);
                        engine.process (buffer, 2);
                    }
                    return engine.getVisualState().thetaA;
                };

                const float cw = finalThetaAfterAdvancing (true);
                const float ccw = finalThetaAfterAdvancing (false);

                // A quarter-cycle forward should land near pi/2 for CW and
                // near -pi/2 (i.e. 3pi/2 wrapped) for CCW - clearly on
                // opposite sides of the circle.
                const double delta = std::abs (dnaorbit::orbitmath::shortestAngleDelta ((double) cw, (double) ccw));
                expectGreaterThan (delta, 1.0,
                                    "CW and CCW must land at clearly different phases for the same PPQ advance");
            }

            beginTest ("Host Lock's per-sample correction stays smooth even across a large transport jump");
            {
                // A loop point can make PPQ jump by a large, arbitrary
                // amount instantly. The correction term is bounded by
                // shortestAngleDelta (<= pi) and a fixed time constant, so
                // it can never itself introduce a large single-sample jump,
                // regardless of how large the position jump was.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 64;

                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 0.3f;
                p.phaseMode = 2;
                p.hostCycleBeats = 4.0;
                p.hostIsPlaying = true;
                p.mix01 = 1.0f;
                p.hostPpqPosition = 0.0;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                int blockIndex = 0;
                auto fillSine = [&] ()
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const double t = (double) (blockIndex * blockSize + n) / sr;
                        const float s = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * t);
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                    ++blockIndex;
                };

                fillSine();
                engine.process (buffer, 2);

                // Loop back: PPQ jumps from wherever it was to 0 - the
                // largest possible jump relative to a 4-beat cycle.
                p.hostPpqPosition = 0.0;
                engine.setParameters (p);

                float maxJump = 0.0f;
                float prevL = buffer.getSample (0, buffer.getNumSamples() - 1);

                for (int block = 0; block < 40; ++block)
                {
                    fillSine();
                    engine.process (buffer, 2);
                    const auto* l = buffer.getReadPointer (0);

                    for (int n = 0; n < buffer.getNumSamples(); ++n)
                    {
                        maxJump = juce::jmax (maxJump, std::abs (l[n] - prevL));
                        prevL = l[n];
                    }

                    p.hostPpqPosition += 4.0 * 0.3 * (double) blockSize / sr;
                    engine.setParameters (p);
                }

                expectLessThan (maxJump, 0.25f,
                                 "A transport jump must not produce an audible sample-to-sample discontinuity");
            }

            beginTest ("Host Lock is deterministic: identical PPQ/parameter sequences produce identical output");
            {
                constexpr double sr = 48000.0;
                constexpr int blockSize = 128;

                auto run = [&] ()
                {
                    HelixEngine engine;
                    engine.prepare (sr, blockSize, 2);

                    HelixEngine::Parameters p;
                    p.rateHz = 1.5f;
                    p.phaseMode = 2;
                    p.hostCycleBeats = 4.0;
                    p.hostIsPlaying = true;
                    p.mix01 = 1.0f;
                    p.hostPpqPosition = 2.0;
                    engine.primeParameters (p);
                    engine.setParameters (p);

                    juce::AudioBuffer<float> buffer (2, blockSize);
                    std::vector<float> captured;

                    for (int block = 0; block < 15; ++block)
                    {
                        for (int n = 0; n < blockSize; ++n)
                        {
                            const float s = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0
                                                                       * (double) (block * blockSize + n) / sr);
                            buffer.setSample (0, n, s);
                            buffer.setSample (1, n, s);
                        }
                        engine.process (buffer, 2);
                        for (int n = 0; n < blockSize; ++n)
                            captured.push_back (buffer.getSample (0, n));

                        p.hostPpqPosition += 4.0 * 1.5 * (double) blockSize / sr;
                        engine.setParameters (p);
                    }
                    return captured;
                };

                const auto runA = run();
                const auto runB = run();

                expect (runA.size() == runB.size());
                for (size_t i = 0; i < runA.size(); ++i)
                    expectWithinAbsoluteError (runA[i], runB[i], 1.0e-9f);
            }
        }
    };

    static HostPhaseLockTests hostPhaseLockTests;
}
