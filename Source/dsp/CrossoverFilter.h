#pragma once

#include <juce_dsp/juce_dsp.h>

namespace dnaorbit::dsp
{
    /**
     * 4th-order Linkwitz-Riley crossover for one channel: cascades two
     * matched 2nd-order Butterworth (Q = 1/sqrt(2)) stages for each of the
     * low and high outputs. Low + High reconstructs the input with flat
     * magnitude AND phase - the defining property of Linkwitz-Riley (a
     * plain Butterworth cascade does not sum flat; a 2nd-order
     * Linkwitz-Riley would also need one output's polarity inverted to sum
     * flat, but at 4th order the two paths are back in phase at the
     * crossover and no inversion is needed). Verified empirically by a
     * frequency sweep in Tests/CrossoverFilterTests.cpp rather than assumed.
     */
    class LinkwitzRileyCrossover
    {
    public:
        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            juce::dsp::ProcessSpec spec { sampleRate, 1, 1 };
            lp1.prepare (spec);
            lp2.prepare (spec);
            hp1.prepare (spec);
            hp2.prepare (spec);
            reset();
            setCrossoverHz (crossoverHz);
        }

        void reset() noexcept
        {
            lp1.reset();
            lp2.reset();
            hp1.reset();
            hp2.reset();
        }

        void setCrossoverHz (float hz) noexcept
        {
            const float clamped = std::clamp (hz, 20.0f, (float) (sampleRate * 0.45));
            if (std::abs (clamped - crossoverHz) < 1.0e-6f && coefficientsSet)
                return;

            crossoverHz = clamped;
            coefficientsSet = true;

            constexpr float q = 0.70710678f; // Butterworth Q; cascaded twice gives LR4
            *lp1.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, crossoverHz, q);
            *lp2.coefficients = *lp1.coefficients;
            *hp1.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, crossoverHz, q);
            *hp2.coefficients = *hp1.coefficients;
        }

        struct LowHigh
        {
            float low = 0.0f;
            float high = 0.0f;
        };

        LowHigh processSample (float input) noexcept
        {
            float low = lp1.processSample (input);
            low = lp2.processSample (low);
            float high = hp1.processSample (input);
            high = hp2.processSample (high);
            return { low, high };
        }

    private:
        double sampleRate = 44100.0;
        float crossoverHz = 120.0f;
        bool coefficientsSet = false;

        juce::dsp::IIR::Filter<float> lp1, lp2, hp1, hp2;
    };
}
