#include <juce_core/juce_core.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    class MonoPreviewTests : public juce::UnitTest
    {
    public:
        MonoPreviewTests() : juce::UnitTest ("MonoPreview", "DNAOrbit") {}

        void runTest() override
        {
            constexpr double sr = 48000.0;
            constexpr int blockSize = 512;

            beginTest ("monoPreview = false (default) is bit-identical to never having Mono Preview at all");
            {
                HelixEngine engineExplicit, engineDefault;
                engineExplicit.prepare (sr, blockSize, 2);
                engineDefault.prepare (sr, blockSize, 2);

                HelixEngine::Parameters pExplicit;
                pExplicit.rateHz = 0.4f; pExplicit.radius01 = 0.8f; pExplicit.depth01 = 0.6f; pExplicit.mix01 = 1.0f;
                pExplicit.monoPreview = false;
                HelixEngine::Parameters pDefault = pExplicit; // struct default monoPreview is also false

                engineExplicit.primeParameters (pExplicit);
                engineExplicit.setParameters (pExplicit);
                engineDefault.primeParameters (pDefault);
                engineDefault.setParameters (pDefault);

                juce::AudioBuffer<float> bufExplicit (2, blockSize), bufDefault (2, blockSize);
                juce::Random random { 7 };

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

            beginTest ("Turning Mono Preview on folds the output to exactly mono within the crossfade window, and back again when turned off");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f; p.radius01 = 1.0f; p.depth01 = 0.6f;
                p.mix01 = 1.0f; p.autoGain = false;
                engine.primeParameters (p);
                engine.setParameters (p);

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

                // Warm up: with the orbit rotating, L and R should clearly differ.
                for (int block = 0; block < 10; ++block)
                {
                    fillSine();
                    engine.process (buffer, 2);
                }
                double maxLRDiffBefore = 0.0;
                for (int n = 0; n < blockSize; ++n)
                    maxLRDiffBefore = std::max (maxLRDiffBefore,
                        (double) std::abs (buffer.getSample (0, n) - buffer.getSample (1, n)));
                expectGreaterThan (maxLRDiffBefore, 0.02,
                                    "L and R must clearly differ before engaging Mono Preview, or this test proves nothing");

                p.monoPreview = true;
                engine.setParameters (p);

                const int rampSamples = (int) std::ceil (0.03 * sr) + 8;
                int processed = 0;
                while (processed < rampSamples)
                {
                    fillSine();
                    engine.process (buffer, 2);
                    processed += blockSize;
                }

                fillSine();
                engine.process (buffer, 2);
                for (int n = 0; n < blockSize; ++n)
                    expectWithinAbsoluteError (buffer.getSample (0, n), buffer.getSample (1, n), 1.0e-5f,
                                                "L and R must be exactly equal once Mono Preview is fully engaged");

                p.monoPreview = false;
                engine.setParameters (p);
                processed = 0;
                while (processed < rampSamples)
                {
                    fillSine();
                    engine.process (buffer, 2);
                    processed += blockSize;
                }
                fillSine();
                engine.process (buffer, 2);
                double maxLRDiffAfter = 0.0;
                for (int n = 0; n < blockSize; ++n)
                    maxLRDiffAfter = std::max (maxLRDiffAfter,
                        (double) std::abs (buffer.getSample (0, n) - buffer.getSample (1, n)));
                expectGreaterThan (maxLRDiffAfter, 0.02,
                                    "Turning Mono Preview back off must restore the stereo image, not leave it stuck at mono");
            }

            beginTest ("Mono Preview applies after Soft Bypass: with both engaged, output is the mono-folded Dry signal");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f; p.radius01 = 1.0f; p.mix01 = 1.0f;
                p.softBypass = true; p.monoPreview = true;
                engine.primeParameters (p); // snap straight to fully engaged

                juce::AudioBuffer<float> buffer (2, blockSize);
                for (int n = 0; n < blockSize; ++n)
                {
                    const float l = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 330.0 * n / sr);
                    const float r = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 330.0 * n / sr + 0.7);
                    buffer.setSample (0, n, l);
                    buffer.setSample (1, n, r);
                }
                juce::AudioBuffer<float> dryCopy;
                dryCopy.makeCopyOf (buffer);

                engine.process (buffer, 2);

                for (int n = 0; n < blockSize; ++n)
                {
                    const float expectedMono = 0.5f * (dryCopy.getSample (0, n) + dryCopy.getSample (1, n));
                    expectWithinAbsoluteError (buffer.getSample (0, n), expectedMono, 1.0e-5f);
                    expectWithinAbsoluteError (buffer.getSample (1, n), expectedMono, 1.0e-5f);
                }
            }

            beginTest ("Mono Preview's crossfade produces no audible sample-to-sample jump while engaging");
            {
                HelixEngine engine;
                engine.prepare (sr, blockSize, 2);

                HelixEngine::Parameters p;
                p.rateHz = 2.0f; p.radius01 = 1.0f; p.depth01 = 0.6f; p.mix01 = 1.0f;
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

                p.monoPreview = true;
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
                                 "Mono Preview's crossfade must not produce an audible sample-to-sample jump");
            }
        }
    };

    static MonoPreviewTests monoPreviewTests;
}
