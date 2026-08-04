#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    /**
     * Measures how long the engine keeps producing audible output after its
     * input goes silent, and checks that the value the plugin reports to the
     * host via getTailLengthSeconds() actually covers it.
     *
     * This matters for offline bounce: a host uses getTailLengthSeconds() to
     * decide how long to keep pulling audio after the last input sample, so
     * under-reporting truncates the tail. The reported value predates
     * Character (which raised the maximum back-delay from a fixed 8ms to
     * 14ms at Deep), so it needed re-checking against what the engine
     * actually does rather than being assumed still correct.
     */
    class TailLengthTests : public juce::UnitTest
    {
    public:
        TailLengthTests() : juce::UnitTest ("TailLength", "DNAOrbit") {}

        /**
         * Returns the time in seconds from the last input sample until the
         * output stays below -60dBFS relative to the input's peak. -60dB is
         * the conventional "decayed to inaudible" bound for a tail.
         */
        double measureTailSeconds (const HelixEngine::Parameters& params, double sampleRate)
        {
            constexpr int blockSize = 256;
            constexpr float inputPeak = 0.5f;

            HelixEngine engine;
            engine.prepare (sampleRate, blockSize, 2);
            auto p = params;
            engine.primeParameters (p);

            juce::AudioBuffer<float> buffer (2, blockSize);

            // Drive with a steady tone long enough to fill the delay lines
            // and settle the filters, so the tail we then measure is the
            // real decay rather than a startup transient.
            double phase = 0.0;
            const double inc = juce::MathConstants<double>::twoPi * 220.0 / sampleRate;
            for (int block = 0; block < 200; ++block)
            {
                for (int n = 0; n < blockSize; ++n)
                {
                    const float s = inputPeak * (float) std::sin (phase);
                    phase += inc;
                    buffer.setSample (0, n, s);
                    buffer.setSample (1, n, s);
                }
                engine.process (buffer, 2);
            }

            // Input goes silent. Count until the output stays under the
            // threshold - "stays", not "first dips", since an orbiting,
            // filtered tail crosses zero constantly.
            const float threshold = inputPeak * juce::Decibels::decibelsToGain (-60.0f);
            const int maxBlocks = (int) std::ceil (2.0 * sampleRate / blockSize); // 2s ceiling
            int lastLoudSample = -1;
            int totalSamples = 0;

            for (int block = 0; block < maxBlocks; ++block)
            {
                buffer.clear();
                engine.process (buffer, 2);

                for (int n = 0; n < blockSize; ++n)
                {
                    const float magnitude = juce::jmax (std::abs (buffer.getSample (0, n)),
                                                         std::abs (buffer.getSample (1, n)));
                    if (magnitude > threshold)
                        lastLoudSample = totalSamples + n;
                }
                totalSamples += blockSize;
            }

            return lastLoudSample < 0 ? 0.0 : (double) (lastLoudSample + 1) / sampleRate;
        }

        void runTest() override
        {
            beginTest ("The reported tail length covers the engine's actual worst-case tail");
            {
                // Worst case for the delay path: Character = Deep (14ms
                // back-delay, the longest) stacked with the maximum Twist
                // (20ms) - 34ms of pure delay before any filter ringing.
                // Bass Anchor is engaged at its default so the crossover's
                // own ringing is included rather than bypassed.
                HelixEngine::Parameters p;
                p.rateHz = 0.5f;
                p.radius01 = 1.0f;
                p.depth01 = 1.0f;
                p.twistMs = 20.0f;
                p.character = 2;        // Deep
                p.bassAnchorHz = 120.0f;
                p.core01 = 0.0f;
                p.mix01 = 1.0f;
                p.autoGain = false;     // isolate the tail from makeup gain
                p.outputDb = 0.0f;

                const double reported = DNAOrbitAudioProcessor().getTailLengthSeconds();

                for (double sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
                {
                    const double measured = measureTailSeconds (p, sampleRate);

                    logMessage ("sampleRate=" + juce::String (sampleRate)
                                 + " measured tail=" + juce::String (measured * 1000.0, 2) + "ms"
                                 + " reported=" + juce::String (reported * 1000.0, 2) + "ms");

                    expect (measured <= reported,
                            "getTailLengthSeconds() (" + juce::String (reported * 1000.0, 2)
                                + "ms) must cover the engine's actual tail ("
                                + juce::String (measured * 1000.0, 2) + "ms) at "
                                + juce::String (sampleRate) + "Hz, or a host will truncate "
                                "the tail on offline bounce");
                }
            }

            beginTest ("The lowest engaged Bass Anchor frequency is the real worst case, and is still covered");
            {
                // The genuine worst case is not the longest delay but the
                // lowest *engaged* crossover: a 4th-order Linkwitz-Riley at
                // ~21Hz rings for far longer than one at the 120Hz default,
                // and 20Hz itself takes the bypass branch so it does not.
                // Anything just above 20Hz does engage, so that is the
                // boundary worth measuring.
                HelixEngine::Parameters p;
                p.rateHz = 0.5f;
                p.radius01 = 1.0f;
                p.depth01 = 1.0f;
                p.twistMs = 20.0f;
                p.character = 2;
                p.bassAnchorHz = 21.0f; // just above the Off threshold
                p.mix01 = 1.0f;
                p.autoGain = false;

                const double reported = DNAOrbitAudioProcessor().getTailLengthSeconds();

                for (double sampleRate : { 44100.0, 48000.0, 96000.0, 192000.0 })
                {
                    const double measured = measureTailSeconds (p, sampleRate);

                    logMessage ("Bass Anchor 21Hz, sampleRate=" + juce::String (sampleRate)
                                 + " measured tail=" + juce::String (measured * 1000.0, 2) + "ms"
                                 + " reported=" + juce::String (reported * 1000.0, 2) + "ms");

                    expect (measured <= reported,
                            "getTailLengthSeconds() (" + juce::String (reported * 1000.0, 2)
                                + "ms) must cover the lowest-crossover tail ("
                                + juce::String (measured * 1000.0, 2) + "ms) at "
                                + juce::String (sampleRate) + "Hz");
                }
            }

            beginTest ("Bass Anchor Off does not produce a longer tail than the reported length either");
            {
                // The 20Hz/Off path skips the crossover entirely, but Twist
                // and the back-delay still apply, so this is checked too
                // rather than assumed shorter.
                HelixEngine::Parameters p;
                p.rateHz = 0.5f;
                p.radius01 = 1.0f;
                p.depth01 = 1.0f;
                p.twistMs = 20.0f;
                p.character = 2;
                p.bassAnchorHz = 20.0f; // Off
                p.mix01 = 1.0f;
                p.autoGain = false;

                const double reported = DNAOrbitAudioProcessor().getTailLengthSeconds();
                const double measured = measureTailSeconds (p, 48000.0);

                logMessage ("Bass Anchor Off: measured tail=" + juce::String (measured * 1000.0, 2) + "ms");

                expect (measured <= reported,
                        "getTailLengthSeconds() must also cover the Bass-Anchor-Off tail ("
                            + juce::String (measured * 1000.0, 2) + "ms)");
            }
        }
    };

    static TailLengthTests tailLengthTests;
}
