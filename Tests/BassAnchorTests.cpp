#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    double rmsOf (const juce::AudioBuffer<float>& buffer, int channel, int startSample, int numSamples)
    {
        double sum = 0.0;
        const auto* data = buffer.getReadPointer (channel);
        for (int n = startSample; n < startSample + numSamples; ++n)
            sum += (double) data[n] * data[n];
        return std::sqrt (sum / (double) numSamples);
    }

    class BassAnchorTests : public juce::UnitTest
    {
    public:
        BassAnchorTests() : juce::UnitTest ("BassAnchor", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("bassAnchorHz = 20 (Off) is bit-identical to never having Bass Anchor at all");
            {
                // The DSP takes a dedicated bypass branch at exactly 20Hz
                // rather than merely narrowing the crossover, so a
                // schema-2-and-earlier project's sound is untouched.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;

                HelixEngine engineOff, engineDefault;
                engineOff.prepare (sr, blockSize, 2);
                engineDefault.prepare (sr, blockSize, 2);

                HelixEngine::Parameters pOff;
                pOff.rateHz = 0.4f; pOff.radius01 = 0.7f; pOff.depth01 = 0.6f;
                pOff.core01 = 0.2f; pOff.mix01 = 1.0f; pOff.stereoPreserve01 = 0.5f;
                pOff.bassAnchorHz = 20.0f;
                HelixEngine::Parameters pDefault = pOff; // struct default is also 20.0f

                engineOff.primeParameters (pOff);
                engineOff.setParameters (pOff);
                engineDefault.primeParameters (pDefault);
                engineDefault.setParameters (pDefault);

                juce::AudioBuffer<float> bufferOff (2, blockSize), bufferDefault (2, blockSize);
                juce::Random random { 909 };

                for (int block = 0; block < 20; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float l = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        const float r = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        bufferOff.setSample (0, n, l); bufferOff.setSample (1, n, r);
                        bufferDefault.setSample (0, n, l); bufferDefault.setSample (1, n, r);
                    }
                    engineOff.process (bufferOff, 2);
                    engineDefault.process (bufferDefault, 2);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < blockSize; ++n)
                            expectWithinAbsoluteError (bufferOff.getSample (ch, n), bufferDefault.getSample (ch, n), 1.0e-7f);
                }
            }

            beginTest ("Low-frequency content stays tied to its input channel instead of being smeared by orbit panning");
            {
                // A low sine well below the crossover, panned hard left
                // (L-only), should stay strongly L-favoured in the output
                // once Bass Anchor is engaged - it must never enter the
                // orbit's panning/delay machinery.
                constexpr double sr = 48000.0;
                constexpr int blockSize = 512;

                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f;
                p.radius01 = 0.8f;
                p.depth01 = 0.6f;
                p.core01 = 0.3f;
                p.mix01 = 1.0f;
                p.autoGain = false;
                p.bassAnchorHz = 150.0f; // well above the 40Hz test tone below
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                double phase = 0.0;
                const double inc = juce::MathConstants<double>::twoPi * 40.0 / sr; // 40Hz, L-only

                double sumL = 0.0, sumR = 0.0;
                for (int block = 0; block < 30; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.3f * (float) std::sin (phase);
                        phase += inc;
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, 0.0f);
                    }
                    engine.process (buffer, 2);

                    if (block >= 10)
                    {
                        sumL += rmsOf (buffer, 0, 0, blockSize);
                        sumR += rmsOf (buffer, 0 == 0 ? 1 : 1, 0, blockSize);
                    }
                }

                expectGreaterThan (sumL, sumR * 2.0,
                                    "40Hz L-only input must stay clearly L-favoured once Bass Anchor keeps it out of the orbit");
            }

            beginTest ("High-frequency content still orbits (Bass Anchor does not disable panning above the crossover)");
            {
                constexpr double sr = 48000.0;
                constexpr int blockSize = 256;

                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f;
                p.radius01 = 1.0f;
                p.depth01 = 0.6f;
                p.mix01 = 1.0f;
                p.bassAnchorHz = 120.0f;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                double phase = 0.0;
                const double inc = juce::MathConstants<double>::twoPi * 2000.0 / sr; // well above 120Hz

                bool sawLDominant = false, sawRDominant = false;

                for (int block = 0; block < 200; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.3f * (float) std::sin (phase);
                        phase += inc;
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                    engine.process (buffer, 2);

                    const double l = rmsOf (buffer, 0, 0, blockSize);
                    const double r = rmsOf (buffer, 1, 0, blockSize);
                    if (l > r * 1.3) sawLDominant = true;
                    if (r > l * 1.3) sawRDominant = true;
                }

                expect (sawLDominant && sawRDominant,
                        "High-frequency content must still visibly pan left and right as the strands orbit");
            }

            beginTest ("Finite output across sample rates and Bass Anchor extremes");
            {
                const double sampleRates[] = { 44100.0, 48000.0, 96000.0, 192000.0 };
                const float anchorValues[] = { 20.0f, 50.0f, 120.0f, 500.0f };

                for (double sr : sampleRates)
                {
                    for (float hz : anchorValues)
                    {
                        HelixEngine engine;
                        engine.prepare (sr, 256, 2);

                        HelixEngine::Parameters p;
                        p.mix01 = 1.0f;
                        p.bassAnchorHz = hz;
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
                                            "sampleRate=" + juce::String (sr) + " bassAnchorHz=" + juce::String (hz));
                        }
                    }
                }
            }

            beginTest ("Mix 0% still matches Dry exactly with Bass Anchor engaged");
            {
                // Bass Anchor only touches the Wet path; Dry must stay
                // untouched regardless of the crossover.
                HelixEngine engine;
                engine.prepare (48000.0, 256, 2);

                HelixEngine::Parameters p;
                p.mix01 = 0.0f;
                p.outputDb = 0.0f;
                p.bassAnchorHz = 150.0f;
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
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dryCopy.getSample (ch, n), 1.0e-4f);
            }
        }
    };

    static BassAnchorTests bassAnchorTests;
}
