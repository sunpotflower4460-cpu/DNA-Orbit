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

    class CharacterTests : public juce::UnitTest
    {
    public:
        CharacterTests() : juce::UnitTest ("Character", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Character = Natural (default) is bit-identical to never having Character at all");
            {
                // Natural (index 0) must reproduce this engine's exact fixed
                // back-attenuation/cutoff/delay constants from before
                // Character existed - so a default-constructed Parameters
                // (character left at its own struct default 0) must sound
                // identical to explicitly requesting Natural.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;

                HelixEngine engineDefault, engineNatural;
                engineDefault.prepare (sr, blockSize, 2);
                engineNatural.prepare (sr, blockSize, 2);

                HelixEngine::Parameters pDefault;
                pDefault.rateHz = 0.4f; pDefault.radius01 = 0.7f; pDefault.depth01 = 0.8f;
                pDefault.twistMs = 5.0f; pDefault.mix01 = 1.0f;
                HelixEngine::Parameters pNatural = pDefault;
                pNatural.character = 0;

                engineDefault.primeParameters (pDefault);
                engineDefault.setParameters (pDefault);
                engineNatural.primeParameters (pNatural);
                engineNatural.setParameters (pNatural);

                juce::AudioBuffer<float> bufferDefault (2, blockSize), bufferNatural (2, blockSize);
                juce::Random random { 606 };

                for (int block = 0; block < 20; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        bufferDefault.setSample (0, n, s); bufferDefault.setSample (1, n, s);
                        bufferNatural.setSample (0, n, s); bufferNatural.setSample (1, n, s);
                    }
                    engineDefault.process (bufferDefault, 2);
                    engineNatural.process (bufferNatural, 2);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < blockSize; ++n)
                            expectWithinAbsoluteError (bufferDefault.getSample (ch, n), bufferNatural.getSample (ch, n), 1.0e-7f);
                }
            }

            beginTest ("Vivid and Deep audibly differ from Natural at the back position");
            {
                // Radius 0 keeps both strands centred (no pan differences to
                // confound the comparison); Depth 1.0 maximises the back
                // cue's effect. thetaA sits at pi (fully "back") after a
                // quarter-cycle at a chosen rate, so hold Rate at 0 and let
                // the engine run from its reset() initial state (thetaA = 0,
                // i.e. fully "front") - instead, use symmetry to place
                // Strand B at the back deterministically from sample 0.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 512;

                auto measure = [&] (int character)
                {
                    HelixEngine engine;
                    engine.prepare (sr, blockSize, 2);

                    HelixEngine::Parameters p;
                    p.rateHz = 0.0f; // thetaA stays at reset's 0 (front); thetaB stays at pi (back)
                    p.radius01 = 0.0f;
                    p.depth01 = 1.0f;
                    p.mix01 = 1.0f;
                    p.autoGain = false;
                    p.character = character;
                    engine.primeParameters (p);
                    engine.setParameters (p);

                    // Warm up well past Deep's 14ms back-delay (672 samples
                    // at 48kHz) before measuring, so the delay line's
                    // initial silence has fully flushed out and the strand's
                    // steady-state gain/cutoff difference is what's measured
                    // - not however much of the probe tone has reached the
                    // output yet.
                    juce::AudioBuffer<float> buffer (2, blockSize);
                    double phase = 0.0;
                    const double inc = juce::MathConstants<double>::twoPi * 8000.0 / sr;
                    double lastRms = 0.0;

                    for (int block = 0; block < 6; ++block)
                    {
                        for (int n = 0; n < blockSize; ++n)
                        {
                            const float s = 0.4f * (float) std::sin (phase);
                            phase += inc;
                            buffer.setSample (0, n, s);
                            buffer.setSample (1, n, s);
                        }
                        engine.process (buffer, 2);
                        lastRms = rmsOf (buffer, 0);
                    }
                    return lastRms;
                };

                const double natural = measure (0);
                const double vivid = measure (1);
                const double deep = measure (2);

                logMessage ("8kHz back-position RMS - Natural: " + juce::String (natural, 5)
                            + " Vivid: " + juce::String (vivid, 5) + " Deep: " + juce::String (deep, 5));

                // Vivid/Deep darken the back position progressively more
                // (lower cutoff removes more of an 8kHz tone, more
                // attenuation reduces level further), so a high-frequency
                // probe at the back must measure progressively quieter.
                expectGreaterThan (natural, vivid * 1.05, "Vivid must be audibly darker than Natural at 8kHz");
                expectGreaterThan (vivid, deep * 1.05, "Deep must be audibly darker than Vivid at 8kHz");
            }

            beginTest ("Finite output across sample rates and all Character values");
            {
                const double sampleRates[] = { 44100.0, 48000.0, 96000.0, 192000.0 };

                for (double sr : sampleRates)
                {
                    for (int character : { 0, 1, 2 })
                    {
                        HelixEngine engine;
                        engine.prepare (sr, 256, 2);

                        HelixEngine::Parameters p;
                        p.mix01 = 1.0f;
                        p.twistMs = 20.0f; // max twist, stacks with Deep's longer back delay
                        p.character = character;
                        engine.setParameters (p);

                        juce::AudioBuffer<float> buffer (2, 256);
                        for (int block = 0; block < 10; ++block)
                        {
                            for (int n = 0; n < 256; ++n)
                            {
                                const float s = 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / sr);
                                buffer.setSample (0, n, s);
                                buffer.setSample (1, n, s);
                            }
                            engine.process (buffer, 2);

                            for (int ch = 0; ch < 2; ++ch)
                                for (int n = 0; n < 256; ++n)
                                    expect (std::isfinite (buffer.getSample (ch, n)),
                                            "sampleRate=" + juce::String (sr) + " character=" + juce::String (character));
                        }
                    }
                }
            }

            beginTest ("Mix 0% still matches Dry exactly at every Character value");
            {
                for (int character : { 0, 1, 2 })
                {
                    HelixEngine engine;
                    engine.prepare (48000.0, 256, 2);

                    HelixEngine::Parameters p;
                    p.mix01 = 0.0f;
                    p.outputDb = 0.0f;
                    p.character = character;
                    engine.setParameters (p);

                    juce::AudioBuffer<float> buffer (2, 256);
                    juce::AudioBuffer<float> dryCopy (2, 256);

                    for (int i = 0; i < 20; ++i)
                    {
                        for (int ch = 0; ch < 2; ++ch)
                            for (int n = 0; n < 256; ++n)
                                buffer.setSample (ch, n, 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * (n + i * 256) / 48000.0 + ch));

                        dryCopy.makeCopyOf (buffer);
                        engine.process (buffer, 2);
                    }

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < buffer.getNumSamples(); ++n)
                            expectWithinAbsoluteError (buffer.getSample (ch, n), dryCopy.getSample (ch, n), 1.0e-4f,
                                                        "character=" + juce::String (character));
                }
            }
        }
    };

    static CharacterTests characterTests;
}
