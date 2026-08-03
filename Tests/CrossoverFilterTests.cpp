#include <juce_core/juce_core.h>
#include "dsp/CrossoverFilter.h"

using namespace dnaorbit::dsp;

namespace
{
    class CrossoverFilterTests : public juce::UnitTest
    {
    public:
        CrossoverFilterTests() : juce::UnitTest ("CrossoverFilter", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Low + High reconstructs the input with flat magnitude across a 20Hz-20kHz sweep");
            {
                constexpr double sampleRate = 48000.0;
                LinkwitzRileyCrossover crossover;
                crossover.prepare (sampleRate);
                crossover.setCrossoverHz (120.0f);

                const double testFreqs[] = { 20.0, 40.0, 80.0, 120.0, 200.0, 500.0,
                                              1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };

                for (double freq : testFreqs)
                {
                    // Feed enough cycles for the IIR filters to settle, then
                    // measure the reconstructed (low + high) RMS against the
                    // input RMS over the same, later window.
                    const int totalSamples = (int) (sampleRate * 0.05); // 50ms
                    double sumInSq = 0.0, sumReconSq = 0.0;
                    const int settleSamples = totalSamples / 2;

                    for (int n = 0; n < totalSamples; ++n)
                    {
                        const float input = (float) std::sin (juce::MathConstants<double>::twoPi * freq * (double) n / sampleRate);
                        const auto lh = crossover.processSample (input);
                        const float recon = lh.low + lh.high;

                        if (n >= settleSamples)
                        {
                            sumInSq += (double) input * input;
                            sumReconSq += (double) recon * recon;
                        }
                    }

                    const double rmsIn = std::sqrt (sumInSq / (double) (totalSamples - settleSamples));
                    const double rmsRecon = std::sqrt (sumReconSq / (double) (totalSamples - settleSamples));
                    const double ratioDb = 20.0 * std::log10 ((rmsRecon + 1.0e-12) / (rmsIn + 1.0e-12));

                    expectWithinAbsoluteError (ratioDb, 0.0, 0.5,
                                                "Reconstruction must be within 0.5dB of flat at " + juce::String (freq) + "Hz");
                }
            }

            beginTest ("Low output is near-silent well above the crossover; High is near-silent well below it");
            {
                constexpr double sampleRate = 48000.0;
                LinkwitzRileyCrossover crossover;
                crossover.prepare (sampleRate);
                crossover.setCrossoverHz (120.0f);

                auto measure = [&] (double freq)
                {
                    double sumLowSq = 0.0, sumHighSq = 0.0;
                    const int totalSamples = (int) (sampleRate * 0.05);
                    const int settleSamples = totalSamples / 2;
                    int measured = 0;

                    for (int n = 0; n < totalSamples; ++n)
                    {
                        const float input = (float) std::sin (juce::MathConstants<double>::twoPi * freq * (double) n / sampleRate);
                        const auto lh = crossover.processSample (input);
                        if (n >= settleSamples)
                        {
                            sumLowSq += (double) lh.low * lh.low;
                            sumHighSq += (double) lh.high * lh.high;
                            ++measured;
                        }
                    }
                    return std::make_pair (std::sqrt (sumLowSq / measured), std::sqrt (sumHighSq / measured));
                };

                const auto atLow = measure (20.0);   // well below 120Hz
                expectGreaterThan (atLow.first, 0.5, "Low output should carry nearly all the energy at 20Hz");
                expectLessThan (atLow.second, 0.05, "High output should be near-silent at 20Hz");

                const auto atHigh = measure (5000.0); // well above 120Hz
                expectLessThan (atHigh.first, 0.05, "Low output should be near-silent at 5kHz");
                expectGreaterThan (atHigh.second, 0.5, "High output should carry nearly all the energy at 5kHz");
            }

            beginTest ("Silence in stays finite and silent");
            {
                LinkwitzRileyCrossover crossover;
                crossover.prepare (48000.0);
                crossover.setCrossoverHz (120.0f);

                for (int n = 0; n < 1000; ++n)
                {
                    const auto lh = crossover.processSample (0.0f);
                    expect (std::isfinite (lh.low) && std::isfinite (lh.high));
                    expectWithinAbsoluteError (lh.low, 0.0f, 1.0e-6f);
                    expectWithinAbsoluteError (lh.high, 0.0f, 1.0e-6f);
                }
            }

            beginTest ("Runs cleanly across supported sample rates and extreme crossover frequencies");
            {
                const double sampleRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };

                for (double sr : sampleRates)
                {
                    for (float hz : { 20.0f, 120.0f, 500.0f, 1.0e6f /* out of range, must clamp */ })
                    {
                        LinkwitzRileyCrossover crossover;
                        crossover.prepare (sr);
                        crossover.setCrossoverHz (hz);

                        for (int n = 0; n < 256; ++n)
                        {
                            const float input = 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / sr);
                            const auto lh = crossover.processSample (input);
                            expect (std::isfinite (lh.low) && std::isfinite (lh.high),
                                    "sampleRate=" + juce::String (sr) + " hz=" + juce::String (hz));
                        }
                    }
                }
            }
        }
    };

    static CrossoverFilterTests crossoverFilterTests;
}
