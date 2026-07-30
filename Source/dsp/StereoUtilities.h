#pragma once

#include <cmath>
#include <algorithm>

namespace dnaorbit::dsp
{
    struct MidSide
    {
        float mid = 0.0f;
        float side = 0.0f;
    };

    inline MidSide toMidSide (float left, float right) noexcept
    {
        return { 0.5f * (left + right), 0.5f * (left - right) };
    }

    struct LeftRight
    {
        float left = 0.0f;
        float right = 0.0f;
    };

    inline LeftRight fromMidSide (float mid, float side) noexcept
    {
        return { mid + side, mid - side };
    }

    /**
     * NULL CORE: zeroes the Mid component of a stereo signal, leaving only
     * the Side component. Must only ever be applied to the Wet signal, never
     * to Dry.
     */
    inline LeftRight nullCoreProcess (float left, float right) noexcept
    {
        const MidSide ms = toMidSide (left, right);
        return fromMidSide (0.0f, ms.side);
    }

    /** Equal-power dry/wet mix coefficients for a mix amount in [0, 1]. */
    struct DryWetGains
    {
        float dry = 1.0f;
        float wet = 0.0f;
    };

    inline DryWetGains equalPowerMix (float mixAmount01) noexcept
    {
        const float clamped = std::clamp (mixAmount01, 0.0f, 1.0f);
        const float angle = clamped * (float) M_PI * 0.5f;
        return { std::cos (angle), std::sin (angle) };
    }

    inline float dbToGain (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }
}
