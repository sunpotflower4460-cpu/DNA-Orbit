#pragma once

#include <cmath>
#include <algorithm>

/**
 * Pure, JUCE-free math helpers describing the "DNA" orbit geometry:
 * two strands rotating around a shared centre axis. Kept dependency-free
 * so it can be unit tested in isolation and reused by both the DSP engine
 * and the UI visualiser.
 */
namespace dnaorbit::orbitmath
{
    constexpr double twoPi = 6.283185307179586476925286766559;
    constexpr double pi    = 3.1415926535897932384626433832795;

    /** Wraps an angle (radians) into the range [0, 2*pi). */
    inline double wrapTwoPi (double theta) noexcept
    {
        theta = std::fmod (theta, twoPi);
        if (theta < 0.0)
            theta += twoPi;
        return theta;
    }

    /** Shortest signed angular difference (target - current), wrapped to [-pi, pi]. */
    inline double shortestAngleDelta (double current, double target) noexcept
    {
        double delta = std::fmod (target - current, twoPi);
        if (delta > pi)
            delta -= twoPi;
        else if (delta < -pi)
            delta += twoPi;
        return delta;
    }

    struct Position
    {
        double x = 0.0; // left/right
        double z = 0.0; // front/back
    };

    /** x = radius * sin(theta), z = cos(theta). Radius is expected in [0, 1]. */
    inline Position computePosition (double theta, double radius) noexcept
    {
        return { radius * std::sin (theta), std::cos (theta) };
    }

    struct Centroid
    {
        double x = 0.0;
        double z = 0.0;
        double distance = 0.0;
    };

    inline Centroid computeCentroid (const Position& a, const Position& b) noexcept
    {
        Centroid c;
        c.x = (a.x + b.x) * 0.5;
        c.z = (a.z + b.z) * 0.5;
        c.distance = std::sqrt (c.x * c.x + c.z * c.z);
        return c;
    }

    /** 0 at the front (theta = 0), 1 at the back (theta = pi). */
    inline double backAmount (double theta) noexcept
    {
        const double frontBack = std::cos (theta);
        return 0.5 * (1.0 - frontBack);
    }

    /** Per-sample angular increment for a given rate in Hz. */
    inline double angularIncrement (double rateHz, double sampleRate) noexcept
    {
        if (sampleRate <= 0.0)
            return 0.0;
        return twoPi * rateHz / sampleRate;
    }

    /**
     * Computes strand B's rate multiplier when Symmetry is below 100%.
     * symmetryNormalized is in [0, 1], 1 == perfectly symmetric.
     */
    inline double rateDifferenceFactor (double symmetryNormalized, double maxRateDifference = 0.03) noexcept
    {
        const double clampedSymmetry = std::clamp (symmetryNormalized, 0.0, 1.0);
        return (1.0 - clampedSymmetry) * maxRateDifference;
    }

    /** Equal-power pan gains for a pan position in [-1, 1]. */
    struct PanGains
    {
        double left = 1.0;
        double right = 1.0;
    };

    inline PanGains equalPowerPan (double pan) noexcept
    {
        const double clamped = std::clamp (pan, -1.0, 1.0);
        const double p = 0.5 * (clamped + 1.0);
        return { std::cos (p * (pi * 0.5)), std::sin (p * (pi * 0.5)) };
    }
}
