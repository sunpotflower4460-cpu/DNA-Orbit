#pragma once

#include <array>
#include <cmath>
#include "../dsp/OrbitMath.h"

namespace dnaorbit::ui
{
    /**
     * Ring buffer holding the recent time-history of the two strands' orbit
     * angles, resampled onto a fixed angular grid.
     *
     * The double helix drawn from this is not decoration: it IS the time-history
     * of the two strands' (x, z) positions. Two antipodal points rotating over
     * time trace a double helix as a matter of geometry, so the DNA shape falls
     * out of the actual physics.
     *
     * Two deliberate design choices:
     *
     *  - Entries are appended on a fixed ANGULAR step, so the helix has the same
     *    shape at every Rate from 0.02 Hz to 4 Hz.
     *  - We store the RELATIVE phase phi = thetaB - thetaA rather than thetaB.
     *    This makes the antipodal invariant structural: whenever phi == pi the
     *    midpoint of the two strands is exactly zero at every stored sample, so
     *    the centre line is provably straight rather than incidentally straight.
     *
     * Nothing here is in screen coordinates - projection happens once per frame
     * elsewhere - so resizing the window never disturbs the stored history.
     */
    class HelixHistory
    {
    public:
        static constexpr int    size         = 140;
        static constexpr double visibleTurns = 1.75;
        static constexpr double angleStep    = visibleTurns * orbitmath::twoPi / (double) size;

        /** Live state polled from the engine. phaseA is UNWRAPPED total phase. */
        struct Live
        {
            double phaseA = 0.0;
            double phi = orbitmath::pi;
            double radius01 = 0.8;
        };

        struct Sample
        {
            double phaseA = 0.0;
            double phi = orbitmath::pi;
            double radius01 = 0.8;
        };

        /** Fills the whole buffer by extrapolating backwards from the current state. */
        void reset (const Live& now) noexcept
        {
            for (int i = 0; i < size; ++i)
            {
                // i == size-1 is the newest.
                ring[(size_t) i] = { now.phaseA - (double) (size - 1 - i) * angleStep,
                                     now.phi, now.radius01 };
            }

            writeIndex = 0;              // next slot to overwrite is the oldest
            newestGridPhase = now.phaseA;
            live = now;
            primed = true;
        }

        /**
         * Appends however many grid entries the phase advance calls for, interpolating
         * between the previous and current live state. Angular velocity is constant
         * between polls, so linear interpolation is exact in the steady state.
         */
        void advance (const Live& now) noexcept
        {
            if (! primed || ! std::isfinite (now.phaseA) || ! std::isfinite (now.phi))
            {
                reset (now);
                return;
            }

            const double previousPhase = live.phaseA;
            double dPhase = now.phaseA - previousPhase;

            // Went backwards, or so far ahead that catching up is pointless
            // (a long message-thread stall): resynthesise instead of looping.
            if (dPhase < 0.0 || dPhase > (double) size * angleStep)
            {
                reset (now);
                return;
            }

            // Unwrap phi relative to the previous value. phi moves by at most
            // maxRateDifference * dPhase per poll, so this is never ambiguous.
            const double targetPhi = previousPhi() + orbitmath::shortestAngleDelta (previousPhi(), now.phi);

            int guard = 0;
            while (now.phaseA - newestGridPhase >= angleStep && guard++ < size)
            {
                const double gridPhase = newestGridPhase + angleStep;
                const double t = dPhase > 1.0e-12
                               ? std::clamp ((gridPhase - previousPhase) / dPhase, 0.0, 1.0)
                               : 1.0;

                ring[(size_t) writeIndex] = { gridPhase,
                                              previousPhi() + (targetPhi - previousPhi()) * t,
                                              live.radius01 + (now.radius01 - live.radius01) * t };

                writeIndex = (writeIndex + 1) % size;
                newestGridPhase = gridPhase;
            }

            live = now;
            live.phi = targetPhi;
        }

        /** ageIndex 0 is the newest stored sample, size-1 the oldest. */
        const Sample& at (int ageIndex) const noexcept
        {
            const int index = ((writeIndex - 1 - ageIndex) % size + size) % size;
            return ring[(size_t) index];
        }

        /**
         * Continuous age of a stored sample in [0, 1], measured against the LIVE
         * phase rather than the grid. Without this the helix would advance one whole
         * slot at a time - at the default 0.12 Hz that is a visible ~3 px lurch every
         * 100 ms. Driving the vertical mapping from continuous phase makes it creep
         * smoothly at any Rate.
         */
        double ageOf (int ageIndex) const noexcept
        {
            const double span = visibleTurns * orbitmath::twoPi;
            return std::clamp ((live.phaseA - at (ageIndex).phaseA) / span, 0.0, 1.0);
        }

        const Live& liveState() const noexcept { return live; }
        bool isPrimed() const noexcept { return primed; }

    private:
        double previousPhi() const noexcept { return live.phi; }

        std::array<Sample, (size_t) size> ring {};
        int writeIndex = 0;
        double newestGridPhase = 0.0;
        Live live {};
        bool primed = false;
    };
}
