#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"
#include "dsp/LinkwitzRileyCrossover.h"

using namespace dnaorbit::dsp;

namespace
{
    double measureCrossoverSum (double frequency)
    {
        constexpr double sampleRate = 48000.0;
        LinkwitzRileyCrossover crossover;
        crossover.prepare (sampleRate);
        crossover.setCutoffHz (120.0f);

        double inputPower = 0.0;
        double outputPower = 0.0;
        for (int n = 0; n < 48000; ++n)
        {
            const float input = (float) std::sin (juce::MathConstants<double>::twoPi
                                                  * frequency * n / sampleRate);
            float low = 0.0f, high = 0.0f;
            crossover.processSample (input, low, high);

            if (n >= 24000)
            {
                inputPower += (double) input * input;
                const double sum = (double) low + high;
                outputPower += sum * sum;
            }
        }
        return std::sqrt (outputPower / inputPower);
    }

    double runAntiPhase (double frequency, float bassAnchorHz)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 256;
        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.0f;
        p.radius01 = 0.0f;
        p.depth01 = 0.0f;
        p.twistMs = 0.0f;
        p.core01 = 0.0f;
        p.mix01 = 1.0f;
        p.autoGain = false;
        p.stereoPreserve01 = 1.0f;
        p.bassAnchorHz = bassAnchorHz;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, blockSize);
        double measuredPower = 0.0;
        int measuredSamples = 0;

        for (int block = 0; block < 180; ++block)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const auto absoluteSample = block * blockSize + n;
                const float value = 0.3f * (float) std::sin (
                    juce::MathConstants<double>::twoPi * frequency
                    * absoluteSample / sampleRate);
                buffer.setSample (0, n, value);
                buffer.setSample (1, n, -value);
            }

            engine.process (buffer, 2);

            if (block >= 140)
            {
                for (int n = 0; n < blockSize; ++n)
                {
                    const double value = buffer.getSample (0, n);
                    measuredPower += value * value;
                    ++measuredSamples;
                }
            }
        }

        return std::sqrt (measuredPower / (double) measuredSamples);
    }

    double runCharacter (int character)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 256;
        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.0f;
        p.radius01 = 0.0f;
        p.depth01 = 1.0f;
        p.twistMs = 0.0f;
        p.core01 = 0.0f;
        p.mix01 = 1.0f;
        p.autoGain = false;
        p.stereoPreserve01 = 0.0f;
        p.bassAnchorHz = 20.0f;
        p.character = character;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, blockSize);
        double power = 0.0;
        int count = 0;
        for (int block = 0; block < 120; ++block)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const int absoluteSample = block * blockSize + n;
                const float value = 0.2f * (float) std::sin (
                    juce::MathConstants<double>::twoPi * 8000.0
                    * absoluteSample / sampleRate);
                buffer.setSample (0, n, value);
                buffer.setSample (1, n, value);
            }
            engine.process (buffer, 2);

            if (block >= 100)
            {
                for (int n = 0; n < blockSize; ++n)
                {
                    const double value = buffer.getSample (0, n);
                    power += value * value;
                    ++count;
                }
            }
        }
        return std::sqrt (power / (double) count);
    }

    class MusicalDspTests : public juce::UnitTest
    {
    public:
        MusicalDspTests() : juce::UnitTest ("MusicalDSP", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Linkwitz-Riley low and high outputs recombine with flat magnitude");
            for (const double frequency : { 60.0, 120.0, 1000.0, 5000.0 })
                expectWithinAbsoluteError (measureCrossoverSum (frequency), 1.0, 0.015,
                                           "Crossover sum must stay flat at "
                                               + juce::String (frequency) + " Hz");

            beginTest ("Bass Anchor strongly removes anti-phase bass from the moving stereo bed");
            {
                const double off = runAntiPhase (60.0, 20.0f);
                const double anchored = runAntiPhase (60.0, 120.0f);
                expectGreaterThan (off, 0.02, "Test setup: 60 Hz Side must be audible with Bass Anchor off");
                expectLessThan (anchored, off * 0.2,
                                "A 120 Hz anchor must keep anti-phase 60 Hz energy out of the moving Side bed");
            }

            beginTest ("Bass Anchor leaves high stereo information substantially intact");
            {
                const double off = runAntiPhase (1000.0, 20.0f);
                const double anchored = runAntiPhase (1000.0, 120.0f);
                expectWithinAbsoluteError (anchored, off, off * 0.08,
                                           "1 kHz Side should remain essentially unchanged by a 120 Hz anchor");
            }

            beginTest ("Natural, Vivid and Deep produce distinct finite depth characters");
            {
                const double natural = runCharacter (0);
                const double vivid = runCharacter (1);
                const double deep = runCharacter (2);
                expect (std::isfinite (natural) && std::isfinite (vivid) && std::isfinite (deep));
                expect (std::abs (natural - vivid) > 1.0e-4
                     || std::abs (vivid - deep) > 1.0e-4,
                        "Character modes must not collapse to the same transfer function");
            }
        }
    };

    static MusicalDspTests musicalDspTests;
}
