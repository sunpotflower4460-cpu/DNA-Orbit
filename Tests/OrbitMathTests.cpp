#include <juce_core/juce_core.h>
#include "dsp/OrbitMath.h"

using namespace dnaorbit::orbitmath;

namespace
{
    class OrbitMathTests : public juce::UnitTest
    {
    public:
        OrbitMathTests() : juce::UnitTest ("OrbitMath", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Perfect symmetry: centroid stays at origin for 1000+ angles");
            {
                const double radius = 0.8;
                bool allWithinTolerance = true;

                for (int i = 0; i < 2000; ++i)
                {
                    const double thetaA = (double) i * (twoPi / 2000.0);
                    const double thetaB = wrapTwoPi (thetaA + pi);

                    const auto posA = computePosition (thetaA, radius);
                    const auto posB = computePosition (thetaB, radius);
                    const auto centroid = computeCentroid (posA, posB);

                    if (std::abs (centroid.x) >= 1e-5 || std::abs (centroid.z) >= 1e-5)
                        allWithinTolerance = false;
                }

                expect (allWithinTolerance, "Centroid must stay within 1e-5 of the origin for every angle when symmetric");
            }

            beginTest ("Same position: centroid equals the shared strand position");
            {
                const double radius = 0.8;

                for (double thetaA : { 0.0, 0.4, 1.7, 3.9, 5.5 })
                {
                    const double thetaB = thetaA;
                    const auto posA = computePosition (thetaA, radius);
                    const auto posB = computePosition (thetaB, radius);
                    const auto centroid = computeCentroid (posA, posB);

                    expectWithinAbsoluteError (centroid.x, posA.x, 1e-9, "centroid.x should equal the shared strand x");
                    expectWithinAbsoluteError (centroid.z, posA.z, 1e-9, "centroid.z should equal the shared strand z");
                }
            }

            beginTest ("90 degree difference matches the analytical value");
            {
                const double radius = 1.0;
                const double thetaA = 0.0;
                const double thetaB = pi * 0.5;

                const auto posA = computePosition (thetaA, radius);
                const auto posB = computePosition (thetaB, radius);
                const auto centroid = computeCentroid (posA, posB);

                // A: x=0, z=1.  B: x=1, z=0.  Centroid: (0.5, 0.5)
                expectWithinAbsoluteError (centroid.x, 0.5, 1e-9);
                expectWithinAbsoluteError (centroid.z, 0.5, 1e-9);
            }

            beginTest ("Radius difference: centroid leans toward the larger-radius strand even at 180 degrees");
            {
                const double thetaA = pi * 0.5; // x = radiusA, z = 0
                const double thetaB = wrapTwoPi (thetaA + pi); // x = -radiusB, z = 0

                const double radiusA = 1.0;
                const double radiusB = 0.4;

                const auto posA = computePosition (thetaA, radiusA);
                const auto posB = computePosition (thetaB, radiusB);
                const auto centroid = computeCentroid (posA, posB);

                expectGreaterThan (centroid.x, 0.0, "Centroid should lean toward the larger-radius strand");
                expectWithinAbsoluteError (centroid.x, (radiusA - radiusB) * 0.5, 1e-9);
            }

            beginTest ("Rate difference: centroid distance oscillates and returns near zero only when B realigns to 180 degrees");
            {
                const double sampleRate = 48000.0;
                const double rateA = 1.0; // Hz, chosen to keep the beat period short for a fast test
                const double symmetryNormalized = 0.0; // maximum drift
                const double diff = rateDifferenceFactor (symmetryNormalized, 0.03);
                const double rateB = rateA * (1.0 + diff);

                double thetaA = 0.0;
                double thetaB = pi;

                double maxDistanceSeen = 0.0;
                bool sawNearZeroAtRealignment = false;

                // One full beat cycle between A and B is 1 / (rateB - rateA) seconds;
                // simulate a bit more than that so realignment is guaranteed to occur.
                const double beatPeriodSeconds = 1.0 / (rateB - rateA);
                const int numSamples = (int) (sampleRate * (beatPeriodSeconds * 1.5));

                for (int n = 0; n < numSamples; ++n)
                {
                    thetaA = wrapTwoPi (thetaA + angularIncrement (rateA, sampleRate));
                    thetaB = wrapTwoPi (thetaB + angularIncrement (rateB, sampleRate));

                    const auto posA = computePosition (thetaA, 0.8);
                    const auto posB = computePosition (thetaB, 0.8);
                    const auto centroid = computeCentroid (posA, posB);

                    maxDistanceSeen = juce::jmax (maxDistanceSeen, centroid.distance);

                    // Skip the very first samples, where B hasn't drifted away from
                    // its starting alignment yet.
                    if (n > (int) (sampleRate * 1.0))
                    {
                        const double relative = std::abs (shortestAngleDelta (thetaB, wrapTwoPi (thetaA + pi)));
                        if (relative < 1e-2 && centroid.distance < 1e-2)
                            sawNearZeroAtRealignment = true;
                    }
                }

                expectGreaterThan (maxDistanceSeen, 0.1, "A rate difference must cause the centroid to visibly drift");
                expect (sawNearZeroAtRealignment, "Centroid distance must return near zero when B realigns to 180 degrees after a full beat cycle");
            }

            beginTest ("Equal-power pan sums to unity power across the range");
            {
                for (double pan = -1.0; pan <= 1.0; pan += 0.1)
                {
                    const auto gains = equalPowerPan (pan);
                    const double power = gains.left * gains.left + gains.right * gains.right;
                    expectWithinAbsoluteError (power, 1.0, 1e-9);
                }
            }

            beginTest ("Back amount is 0 at the front and 1 at the back");
            {
                expectWithinAbsoluteError (backAmount (0.0), 0.0, 1e-9);
                expectWithinAbsoluteError (backAmount (pi), 1.0, 1e-9);
                expectWithinAbsoluteError (backAmount (pi * 0.5), 0.5, 1e-9);
            }
        }
    };

    static OrbitMathTests orbitMathTests;
}
