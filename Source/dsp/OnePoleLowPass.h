#pragma once

#include <cmath>
#include <algorithm>

namespace dnaorbit::dsp
{
    /**
     * Real-time-safe one-pole low-pass filter. Cutoff can be changed every
     * sample (the smoothing of the *target* cutoff itself should happen
     * upstream via juce::SmoothedValue); this class only turns a cutoff
     * frequency into a stable coefficient and applies it, with denormal
     * protection.
     */
    class OnePoleLowPass
    {
    public:
        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate;
            reset();
        }

        void reset() noexcept
        {
            state = 0.0f;
        }

        void setCutoffHz (float cutoffHz) noexcept
        {
            // std::clamp is UB if lo > hi, which would happen below ~41 Hz
            // sample rate - never a real audio rate, but the max() keeps this
            // safe regardless of what prepare() was called with.
            const float upperBound = std::max (20.0f, static_cast<float> (sampleRate * 0.49));
            const float clamped = std::clamp (cutoffHz, 20.0f, upperBound);
            const float x = std::exp (-2.0f * static_cast<float> (M_PI) * clamped / static_cast<float> (sampleRate));
            coefficient = x;
        }

        float processSample (float input) noexcept
        {
            state = input + coefficient * (state - input);

            // Denormal protection.
            constexpr float antiDenormal = 1.0e-20f;
            state += antiDenormal;
            state -= antiDenormal;

            return state;
        }

    private:
        double sampleRate = 44100.0;
        float coefficient = 0.0f;
        float state = 0.0f;
    };
}
