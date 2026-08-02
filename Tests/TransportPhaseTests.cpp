#include <juce_core/juce_core.h>
#include "Parameters.h"
#include "dsp/HelixEngine.h"

using namespace dnaorbit;
using namespace dnaorbit::dsp;

namespace
{
    HelixEngine::VisualState runHostLockedState (double ppq, bool reverse,
                                                  float startDegrees = 0.0f,
                                                  float symmetry = 1.0f,
                                                  int freeHistoryBlocks = 0)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;

        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.5f;
        p.phaseMode = params::phaseFree;
        p.startPhaseDegrees = startDegrees;
        p.reverseDirection = reverse;
        p.symmetry01 = symmetry;
        p.transportPlaying = true;
        p.hostPositionValid = true;
        p.hostPpqPosition = ppq;
        p.cycleBeats = 4.0;
        p.bassAnchorHz = 20.0f;
        p.mix01 = 0.0f;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, blockSize);
        buffer.clear();

        for (int block = 0; block < freeHistoryBlocks; ++block)
            engine.process (buffer, 2);

        p.phaseMode = params::phaseHostLock;
        p.transportJustStarted = false;
        engine.setParameters (p);
        engine.process (buffer, 2);
        return engine.getVisualState();
    }

    float runHostLockedTheta (double ppq, bool reverse, float startDegrees = 0.0f)
    {
        return runHostLockedState (ppq, reverse, startDegrees).thetaA;
    }

    class TransportPhaseTests : public juce::UnitTest
    {
    public:
        TransportPhaseTests() : juce::UnitTest ("TransportPhase", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Bar divisions follow the host time signature");
            {
                expectWithinAbsoluteError (params::divisionIndexToBeats (2, 4, 4), 4.0, 1.0e-12);
                expectWithinAbsoluteError (params::divisionIndexToBeats (2, 3, 4), 3.0, 1.0e-12);
                expectWithinAbsoluteError (params::divisionIndexToBeats (2, 6, 8), 3.0, 1.0e-12);
                expectWithinAbsoluteError (params::divisionIndexToBeats (0, 3, 4), 12.0, 1.0e-12);
            }

            beginTest ("Host Lock is deterministic at the same PPQ position");
            {
                const float a = runHostLockedTheta (5.25, false, 37.0f);
                const float b = runHostLockedTheta (5.25, false, 37.0f);
                expectWithinAbsoluteError (a, b, 1.0e-6f,
                                           "Two fresh renders from the same song position must use the same orbit phase");
            }

            beginTest ("Both strands are deterministic below 100% Symmetry regardless of prior playback");
            {
                const auto shortHistory = runHostLockedState (5.25, false, 37.0f, 0.25f, 3);
                const auto longHistory = runHostLockedState (5.25, false, 37.0f, 0.25f, 137);

                expectWithinAbsoluteError (shortHistory.thetaA, longHistory.thetaA, 1.0e-6f,
                                           "Strand A must ignore free-running history after Host Lock activates");
                expectWithinAbsoluteError (shortHistory.thetaB, longHistory.thetaB, 1.0e-6f,
                                           "Drifting Strand B must also be derived from PPQ, not its previous state");
                expectWithinAbsoluteError (shortHistory.phi, longHistory.phi, 1.0e-6f,
                                           "The complete DNA geometry must be repeatable at a song position");
            }

            beginTest ("Host Lock maps a quarter cycle to 90 degrees plus per-sample advance");
            {
                constexpr double sampleRate = 48000.0;
                constexpr int blockSize = 64;
                const double increment = orbitmath::angularIncrement (0.5, sampleRate);
                const double expected = orbitmath::wrapTwoPi (
                    orbitmath::pi * 0.5 + increment * (blockSize - 1));
                expectWithinAbsoluteError ((double) runHostLockedTheta (1.0, false),
                                           expected, 1.0e-5,
                                           "PPQ 1 in a four-beat cycle must begin at 90 degrees");
            }

            beginTest ("Direction reverses the host-locked orbit");
            {
                constexpr double sampleRate = 48000.0;
                constexpr int blockSize = 64;
                const double increment = orbitmath::angularIncrement (0.5, sampleRate);
                const double cwExpected = orbitmath::wrapTwoPi (
                    orbitmath::pi * 0.5 + increment * (blockSize - 1));
                const double ccwExpected = orbitmath::wrapTwoPi (
                    -orbitmath::pi * 0.5 - increment * (blockSize - 1));

                expectWithinAbsoluteError ((double) runHostLockedTheta (1.0, false),
                                           cwExpected, 1.0e-5);
                expectWithinAbsoluteError ((double) runHostLockedTheta (1.0, true),
                                           ccwExpected, 1.0e-5);
            }

            beginTest ("Start Phase offsets the deterministic song position");
            {
                constexpr double sampleRate = 48000.0;
                constexpr int blockSize = 64;
                const double increment = orbitmath::angularIncrement (0.5, sampleRate);
                const double expected = orbitmath::wrapTwoPi (
                    orbitmath::pi + increment * (blockSize - 1));
                expectWithinAbsoluteError ((double) runHostLockedTheta (0.0, false, 180.0f),
                                           expected, 1.0e-5);
            }
        }
    };

    static TransportPhaseTests transportPhaseTests;
}
