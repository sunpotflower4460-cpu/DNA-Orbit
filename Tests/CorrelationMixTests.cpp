#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    constexpr double sr = 48000.0;
    constexpr int bs = 256;

    double rms (const juce::AudioBuffer<float>& buffer)
    {
        double sum = 0.0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const double s = buffer.getSample (ch, n);
                sum += s * s;
            }
        return std::sqrt (sum / (double) (buffer.getNumChannels() * buffer.getNumSamples()));
    }

    struct Measurement
    {
        double inputRms = 0.0;
        double outputRms = 0.0;
    };

    Measurement runCorrelatedCase (float mix01, bool autoGain)
    {
        HelixEngine engine;
        engine.prepare (sr, bs, 2);

        HelixEngine::Parameters p;
        p.rateHz = 0.2f;
        p.radius01 = 0.0f;
        p.depth01 = 0.0f;
        p.symmetry01 = 1.0f;
        p.twistMs = 0.0f;
        p.core01 = 0.0f;
        p.mix01 = mix01;
        p.outputDb = 0.0f;
        p.autoGain = autoGain;
        p.stereoPreserve01 = 0.0f;
        engine.primeParameters (p);
        engine.setParameters (p);

        juce::AudioBuffer<float> buffer (2, bs);
        juce::AudioBuffer<float> input (2, bs);
        int sampleIndex = 0;

        // Two seconds is roughly eight correlation time constants, so the
        // slow estimate has settled without relying on a magic first block.
        const int blocks = (int) std::ceil (2.0 * sr / (double) bs);
        for (int block = 0; block < blocks; ++block)
        {
            for (int n = 0; n < bs; ++n)
            {
                const float s = 0.3f * (float) std::sin (
                    juce::MathConstants<double>::twoPi * 220.0 * (double) sampleIndex++ / sr);
                buffer.setSample (0, n, s);
                buffer.setSample (1, n, s);
            }
            input.makeCopyOf (buffer);
            engine.process (buffer, 2);
        }

        return { rms (input), rms (buffer) };
    }

    class CorrelationMixTests : public juce::UnitTest
    {
    public:
        CorrelationMixTests() : juce::UnitTest ("CorrelationMix", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Auto Gain removes the correlated 50% Mix loudness bump");
            {
                const auto m = runCorrelatedCase (0.5f, true);
                const double ratioDb = 20.0 * std::log10 (m.outputRms / m.inputRms);
                expectWithinAbsoluteError (ratioDb, 0.0, 0.25,
                                            "After settling, identical Dry/Wet at 50% must remain level matched");
            }

            beginTest ("Correlation normalization leaves Dry and Wet endpoints unchanged");
            {
                const auto dry = runCorrelatedCase (0.0f, true);
                const auto wet = runCorrelatedCase (1.0f, true);

                expectWithinAbsoluteError (dry.outputRms, dry.inputRms, dry.inputRms * 1.0e-4,
                                           "Mix 0% must remain exact Dry");

                const double wetDb = 20.0 * std::log10 (wet.outputRms / wet.inputRms);
                expectWithinAbsoluteError (wetDb, 0.0, 0.25,
                                            "Mix 100% must still use only Wet level matching, not Mix normalization");
            }

            beginTest ("Auto Gain off preserves the uncompensated legacy Mix law");
            {
                const auto on = runCorrelatedCase (0.5f, true);
                const auto off = runCorrelatedCase (0.5f, false);

                // This is intentionally not an exact target: it proves that the
                // Auto Gain switch controls both geometry makeup and the new
                // correlation normalization rather than silently normalizing
                // when the user asked for the raw path.
                expectGreaterThan (std::abs (20.0 * std::log10 (off.outputRms / on.outputRms)), 0.5,
                                   "Auto Gain off must be measurably different from the compensated path");
            }
        }
    };

    static CorrelationMixTests correlationMixTests;
}
