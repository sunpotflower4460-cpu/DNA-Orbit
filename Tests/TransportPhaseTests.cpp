#include <juce_core/juce_core.h>
#include "Parameters.h"
#include "dsp/HelixEngine.h"

using namespace dnaorbit;
using namespace dnaorbit::dsp;

namespace
{
    float runHostLockedTheta (double ppq, bool reverse, float startDegrees = 0.0f)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 64;

        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.5f;       // 120 BPM, one cycle per 4 quarter notes
        p.phaseMode = params::phaseHostLock;
        p.startPhaseDegrees = startDegrees;
        p.reverseDirection = reverse;
        p.transportPlaying = true;
        p.transportJustStarted = true;
        p.hostPositionValid = true;
        p.hostPpqPosition = ppq;
        p.cycleBeats = 4.0;
        p.bassAnchorHz = 20.0f;
        p.mix01 = 0.0f;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, blockSize);
        buffer.clear();
        engine.process (buffer, 2);
        return engine.getVisualState().thetaA;
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
