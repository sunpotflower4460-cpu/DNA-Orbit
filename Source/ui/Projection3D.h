#pragma once

#include <algorithm>
#include <cmath>

/**
 * Pure, JUCE-free 3D projection maths for the helix view. Kept dependency-free
 * (like dsp/OrbitMath.h) so the geometry can be unit tested headlessly.
 *
 * World space: x = pan (right positive), y = time (up = older),
 *              z = front/back depth (toward the viewer positive, matching
 *              OrbitMath's z = cos(theta) where theta = 0 is the front).
 */
namespace dnaorbit::ui
{
    struct Projection3D
    {
        // --- Display scales -------------------------------------------------------
        // Sx is a deliberate anamorphic stretch of the PAN axis: pan is the axis the
        // listener actually hears and the axis a landscape panel has room for. It is
        // a display scale only, applied identically here and in the top-down inset so
        // the two views can never disagree. Sy sets the helix pitch.
        static constexpr double scaleX = 1.45;
        static constexpr double scaleY = 1.61;
        static constexpr double scaleZ = 1.00;

        static constexpr double visibleTurns = 1.75;

        /** World units of vertical travel per full turn. */
        static constexpr double pitch = 2.0 * scaleY / visibleTurns;

        /**
         * Screen y stays strictly monotonic in age - no fold-over, no ribbon cusps,
         * well-behaved depth ordering - only while
         *     dy'/dy = cos(a) - sin(a) * max|dz/dy| > 0,  max|dz/dy| = 2*pi / pitch
         * i.e. below atan(pitch / 2*pi). With the pitch above that is ~16.3 degrees,
         * so 13 degrees ships with a comfortable margin.
         */
        static double criticalTiltRadians() noexcept
        {
            return std::atan (pitch / 6.283185307179586);
        }

        static constexpr double tiltRadians = 13.0 * 3.14159265358979323846 / 180.0;

        // --- Pinhole camera -------------------------------------------------------
        // f == cameraDistance keeps unit scale at the origin.
        static constexpr double cameraDistance = 8.0;
        static constexpr double focalLength    = 8.0;

        struct Projected
        {
            double x = 0.0;     ///< normalised screen x (pre fit-to-bounds)
            double y = 0.0;     ///< normalised screen y, growing downward
            double viewZ = 0.0; ///< depth after tilt; larger is nearer the viewer
            double invW = 1.0;  ///< perspective scale factor, for sizing sprites
        };

        /** Projects a world-space point. Yaw is deliberately always zero - see project(). */
        static Projected project (double worldX, double worldY, double worldZ) noexcept
        {
            const double px = worldX * scaleX;
            const double py = worldY * scaleY;
            const double pz = worldZ * scaleZ;

            // Rotate about X only. Yaw MUST stay zero: rotation about X leaves x
            // untouched, so screen x remains a pure function of pan. Any yaw would
            // mix front/back depth into the horizontal axis, which for a stereo
            // imaging plugin would misreport the one axis that matters most.
            const double c = std::cos (tiltRadians);
            const double s = std::sin (tiltRadians);
            const double yTilted =  py * c - pz * s;
            const double zTilted =  py * s + pz * c;

            const double zc = std::max (cameraDistance - zTilted, 0.1);
            const double invW = focalLength / zc;

            return { px * invW, -yTilted * invW, zTilted, invW };
        }

        /** Extent of the projected geometry, used to fit the view into a component. */
        struct Bounds
        {
            double minX = 0.0, maxX = 0.0, minY = 0.0, maxY = 0.0;
            double minViewZ = 0.0, maxViewZ = 0.0;
        };

        /**
         * Conservative fit box from the 8 corners of the world AABB. Pinhole
         * projection of geometry entirely in front of the camera preserves
         * convexity, so the corner hull bounds every interior point.
         *
         * Uses the radius-INDEPENDENT box (full +/-1 in x) on purpose, so automating
         * Radius does not zoom the camera in and out.
         */
        static Bounds computeBounds() noexcept
        {
            Bounds b;
            bool first = true;

            for (int i = 0; i < 8; ++i)
            {
                const double wx = (i & 1) ? 1.0 : -1.0;
                const double wy = (i & 2) ? 1.0 : -1.0;
                const double wz = (i & 4) ? 1.0 : -1.0;
                const auto p = project (wx, wy, wz);

                if (first)
                {
                    b.minX = b.maxX = p.x;
                    b.minY = b.maxY = p.y;
                    b.minViewZ = b.maxViewZ = p.viewZ;
                    first = false;
                }
                else
                {
                    b.minX = std::min (b.minX, p.x); b.maxX = std::max (b.maxX, p.x);
                    b.minY = std::min (b.minY, p.y); b.maxY = std::max (b.maxY, p.y);
                    b.minViewZ = std::min (b.minViewZ, p.viewZ);
                    b.maxViewZ = std::max (b.maxViewZ, p.viewZ);
                }
            }

            return b;
        }

        /** Maps a projected point into component pixels. */
        struct Fit
        {
            double scale = 1.0;
            double originX = 0.0;
            double originY = 0.0;

            double toScreenX (double projectedX) const noexcept { return originX + scale * projectedX; }
            double toScreenY (double projectedY) const noexcept { return originY + scale * projectedY; }
        };

        static Fit computeFit (double componentWidth, double componentHeight, double marginPx) noexcept
        {
            const auto b = computeBounds();
            const double spanX = std::max (b.maxX - b.minX, 1.0e-6);
            const double spanY = std::max (b.maxY - b.minY, 1.0e-6);

            const double usableW = std::max (componentWidth  - 2.0 * marginPx, 1.0);
            const double usableH = std::max (componentHeight - 2.0 * marginPx, 1.0);

            Fit fit;
            fit.scale = std::min (usableW / spanX, usableH / spanY);
            fit.originX = componentWidth  * 0.5 - fit.scale * (b.minX + b.maxX) * 0.5;
            fit.originY = componentHeight * 0.5 - fit.scale * (b.minY + b.maxY) * 0.5;
            return fit;
        }

        /** Normalised 0 (farthest) .. 1 (nearest) for fog and depth bucketing. */
        static double depth01 (double viewZ) noexcept
        {
            const auto b = computeBounds();
            const double span = std::max (b.maxViewZ - b.minViewZ, 1.0e-9);
            return std::clamp ((viewZ - b.minViewZ) / span, 0.0, 1.0);
        }
    };
}
