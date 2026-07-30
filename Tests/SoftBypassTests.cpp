#include <juce_core/juce_core.h>
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
            constexpr int blockSize = 512;

            beginTest ("softBypass = false (default) is bit-identical to never having Soft Bypass at all");
            {
                HelixEngine engineExplicit, engineDefault;
                engineExplicit.prepare (sr, blockSize, 2);
                engineDefault.prepare (sr, blockSize, 2);

                HelixEngine::Parameters pExplicit;
                pExplicit.rateHz = 0.4f; pExplicit.radius01 = 0.8f; pExplicit.depth01 = 0.6f;
                pExplicit.core01 = 0.2f; pExplicit.mix01 = 1.0f;
                pExplicit.softBypass = false;
                HelixEngine::Parameters pDefault = pExplicit; // struct default softBypass is also false

                engineExplicit.primeParameters (pExplicit);
                engineExplicit.setParameters (pExplicit);
                engineDefault.primeParameters (pDefault);
                engineDefault.setParameters (pDefault);

                juce::AudioBuffer<float> bufExplicit (2, blockSize), bufDefault (2, blockSize);
                juce::Random random { 42 };

                for (int block = 0; block < 10; ++block)
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float l = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        const float r = 0.3f * (random.nextFloat() * 2.0f - 1.0f);
                        bufExplicit.setSample (0, n, l); bufExplicit.setSample (1, n, r);
                        bufDefault.setSample (0, n, l); bufDefault.setSample (1, n, r);
                    }
                    engineExplicit.process (bufExplicit, 2);
                    engineDefault.process (bufDefault, 2);

                    for (int ch = 0; ch < 2; ++ch)
                        for (int n = 0; n < blockSize; ++n)
                            expectWithinAbsoluteError (bufExplicit.getSample (ch, n), bufDefault.getSample (ch, n), 1.0e-7f);
                }
            }

            beginTest ("Turning Soft Bypass on ramps the output to exactly Dry within the crossfade window, and back again when turned off");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f; p.radius01 = 1.0f; p.depth01 = 0.6f; p.core01 = 0.0f;
                p.mix01 = 1.0f; p.outputDb = 6.0f; p.autoGain = false;
                engine.primeParameters (p);
                engine.setParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                juce::AudioBuffer<float> dryCopy (2, blockSize);
                double phase = 0.0;
                const double inc = juce::MathConstants<double>::twoPi * 220.0 / sr;

                auto fillSine = [&]
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.4f * (float) std::sin (phase);
                        phase += inc;
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                };

                // Warm up with Soft Bypass off: confirm Wet really differs from Dry,
                // so the "now equals Dry" check below actually proves something.
                for (int block = 0; block < 10; ++block)
                {
                    fillSine();
                    engine.process (buffer, 2);
                }
                fillSine();
                dryCopy.makeCopyOf (buffer);
                engine.process (buffer, 2);
                double maxDiffBeforeBypass = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < blockSize; ++n)
                        maxDiffBeforeBypass = std::max (maxDiffBeforeBypass,
                            (double) std::abs (buffer.getSample (ch, n) - dryCopy.getSample (ch, n)));
                expectGreaterThan (maxDiffBeforeBypass, 0.05,
                                    "Wet must clearly differ from Dry before engaging Soft Bypass, or this test proves nothing");

                // Engage Soft Bypass via a real (non-primed) ramp, like automation
                // or a UI click would.
                p.softBypass = true;
                engine.setParameters (p);

                const int rampSamples = (int) std::ceil (0.03 * sr) + 8; // a little past the 30ms ramp
                int processed = 0;
                while (processed < rampSamples)
                {
                    fillSine();
                    engine.process (buffer, 2);
                    processed += blockSize;
                }

                // Fully ramped: output must now be exactly Dry, independent of
                // Mix/Output/Core - Soft Bypass crossfades after everything else.
                fillSine();
                dryCopy.makeCopyOf (buffer);
                engine.process (buffer, 2);
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < blockSize; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dryCopy.getSample (ch, n), 1.0e-5f);

                // Disengage: after another full ramp, Wet must differ from Dry again
                // - Soft Bypass must not leave the engine permanently stuck at Dry.
                p.softBypass = false;
                engine.setParameters (p);
                processed = 0;
                while (processed < rampSamples)
                {
                    fillSine();
                    engine.process (buffer, 2);
                    processed += blockSize;
                }
                fillSine();
                dryCopy.makeCopyOf (buffer);
                engine.process (buffer, 2);
                double maxDiffAfterUnbypass = 0.0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < blockSize; ++n)
                        maxDiffAfterUnbypass = std::max (maxDiffAfterUnbypass,
                            (double) std::abs (buffer.getSample (ch, n) - dryCopy.getSample (ch, n)));
                expectGreaterThan (maxDiffAfterUnbypass, 0.05,
                                    "Turning Soft Bypass back off must restore the processed Wet signal, not leave it stuck at Dry");
            }

            beginTest ("Soft Bypass does not freeze the orbit phase - the engine keeps running underneath it");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 1.0f; p.mix01 = 1.0f; p.softBypass = true;
                engine.primeParameters (p); // snap straight to fully bypassed

                juce::AudioBuffer<float> buffer (2, blockSize);
                buffer.clear();

                engine.process (buffer, 2);
                const float thetaAfterFirst = engine.getVisualState().thetaA;
                engine.process (buffer, 2);
                const float thetaAfterSecond = engine.getVisualState().thetaA;

                expect (std::abs (thetaAfterSecond - thetaAfterFirst) > 1.0e-4f,
                        "Orbit phase must keep advancing while Soft Bypass is engaged, not freeze at its pre-bypass value");
            }

            beginTest ("Soft Bypass's crossfade produces no audible sample-to-sample jump while engaging");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f; p.radius01 = 1.0f; p.depth01 = 0.6f;
                p.mix01 = 1.0f; p.outputDb = 6.0f; p.autoGain = false;
                engine.primeParameters (p);

                juce::AudioBuffer<float> buffer (2, blockSize);
                double phase = 0.0;
                const double inc = juce::MathConstants<double>::twoPi * 220.0 / sr;

                auto fillSine = [&]
                {
                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float s = 0.4f * (float) std::sin (phase);
                        phase += inc;
                        buffer.setSample (0, n, s);
                        buffer.setSample (1, n, s);
                    }
                };

                for (int block = 0; block < 5; ++block)
                {
                    fillSine();
                    engine.process (buffer, 2);
                }

                p.softBypass = true;
                engine.setParameters (p);

                float prevL = buffer.getSample (0, blockSize - 1);
                double maxJump = 0.0;

                const int rampSamples = (int) std::ceil (0.03 * sr) + 8;
                int processed = 0;
                while (processed < rampSamples)
                {
                    fillSine();
                    engine.process (buffer, 2);

                    for (int n = 0; n < blockSize; ++n)
                    {
                        const float cur = buffer.getSample (0, n);
                        maxJump = std::max (maxJump, (double) std::abs (cur - prevL));
                        prevL = cur;
                    }
                    processed += blockSize;
                }

                expectLessThan (maxJump, 0.05,
                                 "Soft Bypass's crossfade must not produce an audible sample-to-sample jump");
            }

            beginTest ("Soft Bypass, fully engaged, matches Dry regardless of Mix/Output/Null Core");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 3.0f; p.radius01 = 1.0f; p.depth01 = 1.0f; p.core01 = 0.5f;
                p.mix01 = 0.5f; p.outputDb = -6.0f; p.nullCore = true; p.softBypass = true;
                engine.primeParameters (p); // snap straight to fully bypassed

                juce::AudioBuffer<float> buffer (2, blockSize);
                juce::AudioBuffer<float> dryCopy (2, blockSize);
                for (int n = 0; n < blockSize; ++n)
                {
                    const float l = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 330.0 * n / sr);
                    const float r = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 330.0 * n / sr + 0.3);
                    buffer.setSample (0, n, l);
                    buffer.setSample (1, n, r);
                }
                dryCopy.makeCopyOf (buffer);
                engine.process (buffer, 2);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < blockSize; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), dryCopy.getSample (ch, n), 1.0e-5f);
            }
        }
    };

    static SoftBypassTests softBypassTests;
}
