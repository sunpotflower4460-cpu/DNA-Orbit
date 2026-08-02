#pragma once

#include <algorithm>
#include <cmath>
#include "OrbitMath.h"

namespace dnaorbit::dsp
{
    /**
     * Allocation-free fourth-order Linkwitz-Riley crossover.
     *
     * Each output is two cascaded second-order Butterworth sections at the
     * same cutoff. The low/high paths therefore have matching phase at the
     * crossover and sum flat when recombined. Coefficients are only updated
     * when the caller changes the cutoff; processSample() is multiplication
     * and addition only.
     */
    class LinkwitzRileyCrossover
    {
    public:
        void prepare (double newSampleRate) noexcept
        {
            sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
            lastCutoffHz = -1.0f;
            reset();
            setCutoffHz (120.0f);
        }

        void reset() noexcept
        {
            low1.reset();
            low2.reset();
            high1.reset();
            high2.reset();
        }

        void setCutoffHz (float cutoffHz) noexcept
        {
            const float upper = std::max (20.0f, (float) sampleRate * 0.45f);
            const float clamped = std::clamp (std::isfinite (cutoffHz) ? cutoffHz : 120.0f,
                                              20.0f, upper);

            // Avoid four trigonometric coefficient rebuilds when a control-rate
            // update produces the same rounded cutoff as the previous update.
            if (std::abs (clamped - lastCutoffHz) < 0.01f)
                return;

            lastCutoffHz = clamped;
            low1.setLowPass (sampleRate, clamped);
            low2.setLowPass (sampleRate, clamped);
            high1.setHighPass (sampleRate, clamped);
            high2.setHighPass (sampleRate, clamped);
        }

        void processSample (float input, float& low, float& high) noexcept
        {
            low = low2.process (low1.process (input));
            high = high2.process (high1.process (input));
        }

    private:
        class Biquad
        {
        public:
            void reset() noexcept
            {
                z1 = 0.0f;
                z2 = 0.0f;
            }

            void setLowPass (double sampleRate, float cutoffHz) noexcept
            {
                setCoefficients (sampleRate, cutoffHz, false);
            }

            void setHighPass (double sampleRate, float cutoffHz) noexcept
            {
                setCoefficients (sampleRate, cutoffHz, true);
            }

            float process (float input) noexcept
            {
                const float output = b0 * input + z1;
                z1 = b1 * input - a1 * output + z2;
                z2 = b2 * input - a2 * output;

                constexpr float antiDenormal = 1.0e-20f;
                z1 += antiDenormal; z1 -= antiDenormal;
                z2 += antiDenormal; z2 -= antiDenormal;
                return output;
            }

        private:
            void setCoefficients (double sampleRate, float cutoffHz, bool highPass) noexcept
            {
                constexpr double q = 0.7071067811865475244;
                const double omega = orbitmath::twoPi * (double) cutoffHz / sampleRate;
                const double cosine = std::cos (omega);
                const double sine = std::sin (omega);
                const double alpha = sine / (2.0 * q);
                const double a0 = 1.0 + alpha;

                const double common = highPass ? 0.5 * (1.0 + cosine)
                                               : 0.5 * (1.0 - cosine);
                b0 = (float) (common / a0);
                b1 = (float) ((highPass ? -(1.0 + cosine) : (1.0 - cosine)) / a0);
                b2 = b0;
                a1 = (float) ((-2.0 * cosine) / a0);
                a2 = (float) ((1.0 - alpha) / a0);
            }

            float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
            float a1 = 0.0f, a2 = 0.0f;
            float z1 = 0.0f, z2 = 0.0f;
        };

        double sampleRate = 44100.0;
        float lastCutoffHz = -1.0f;
        Biquad low1, low2, high1, high2;
    };
}
