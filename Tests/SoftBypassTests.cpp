#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    class SoftBypassTests : public juce::UnitTest
    {
    public:
        SoftBypassTests() : juce::UnitTest ("SoftBypass", "DNAOrbit") {}

        void runTest() override
        {
            constexpr double sr = 48000.0;
            constexpr int bs = 64;

            beginTest ("Primed Soft Bypass is exact Dry and bypasses Output trim");
            {
                HelixEngine engine;
                engine.prepare (sr, bs, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f;
                p.radius01 = 1.0f;
                p.depth01 = 1.0f;
                p.twistMs = 20.0f;
                p.core01 = 1.0f;
                p.mix01 = 1.0f;
                p.outputDb = 6.0f;
                p.stereoPreserve01 = 1.0f;
                p.softBypass = true;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, bs);
                juce::AudioBuffer<float> dry (2, bs);
                juce::Random random { 8181 };

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < bs; ++n)
                        buffer.setSample (ch, n, 0.4f * (random.nextFloat() * 2.0f - 1.0f));

                dry.makeCopyOf (buffer);
                engine.process (buffer, 2);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < bs; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dry.getSample (ch, n), 1.0e-6f,
                                                   "Soft Bypass endpoint must be exact Dry");
            }

            beginTest ("Soft Bypass ramps without a hard discontinuity and settles within 100 ms");
            {
                HelixEngine engine;
                engine.prepare (sr, bs, 2);

                HelixEngine::Parameters p;
                p.rateHz = 1.0f;
                p.radius01 = 1.0f;
                p.depth01 = 1.0f;
                p.twistMs = 12.0f;
                p.core01 = 0.5f;
                p.mix01 = 1.0f;
                p.outputDb = 3.0f;
                p.softBypass = false;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, bs);
                juce::AudioBuffer<float> dry (2, bs);
                int sampleIndex = 0;

                auto fill = [&]
                {
                    for (int n = 0; n < bs; ++n)
                    {
                        const float s = 0.25f * (float) std::sin (
                            juce::MathConstants<double>::twoPi * 220.0 * (double) sampleIndex++ / sr);
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                    dry.makeCopyOf (buffer);
                };

                for (int i = 0; i < 20; ++i)
                {
                    fill();
                    engine.process (buffer, 2);
                }

                float previous = buffer.getSample (0, bs - 1);
                float maxJump = 0.0f;
                p.softBypass = true;
                engine.setParameters (p);

                const int blocksTo100ms = (int) std::ceil (0.1 * sr / (double) bs);
                for (int block = 0; block < blocksTo100ms; ++block)
                {
                    fill();
                    engine.process (buffer, 2);
                    for (int n = 0; n < bs; ++n)
                    {
                        maxJump = juce::jmax (maxJump, std::abs (buffer.getSample (0, n) - previous));
                        previous = buffer.getSample (0, n);
                    }
                }

                expectLessThan (maxJump, 0.25f, "Soft Bypass must not introduce a large sample discontinuity");

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < bs; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dry.getSample (ch, n), 2.0e-4f,
                                                   "Soft Bypass must settle to Dry within 100 ms");
            }

            beginTest ("Orbit state keeps advancing while Soft Bypass is fully on");
            {
                HelixEngine engine;
                engine.prepare (sr, bs, 2);

                HelixEngine::Parameters p;
                p.rateHz = 1.5f;
                p.softBypass = true;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, bs);
                buffer.clear();
                const double before = engine.getVisualState().phaseA;

                for (int i = 0; i < 100; ++i)
                {
                    buffer.clear();
                    engine.process (buffer, 2);
                }

                const double after = engine.getVisualState().phaseA;
                expectGreaterThan (std::abs (after - before), 0.1,
                                   "Soft Bypass must never freeze the internal orbit");
            }
        }
    };

    static SoftBypassTests softBypassTests;
}
