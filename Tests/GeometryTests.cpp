#include <juce_core/juce_core.h>
#include "ui/HelixHistory.h"
#include "ui/Projection3D.h"

using namespace dnaorbit;
using namespace dnaorbit::ui;

namespace
{
    /** Feeds the history the way the UI timer does: one poll per simulated frame. */
    void driveHistory (HelixHistory& history, double rateHz, double phi,
                        double seconds, double frameRateHz = 45.0)
    {
        const int frames = juce::jmax (1, (int) (seconds * frameRateHz));
        const double phasePerFrame = orbitmath::twoPi * rateHz / frameRateHz;

        HelixHistory::Live live;
        live.phaseA = 0.0;
        live.phi = phi;
        live.radius01 = 0.8;
        history.reset (live);

        for (int i = 0; i < frames; ++i)
        {
            live.phaseA += phasePerFrame;
            history.advance (live);
        }
    }

    class GeometryTests : public juce::UnitTest
    {
    public:
        GeometryTests() : juce::UnitTest ("Geometry", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("History: the helix shape is identical at 0.02 Hz and at 4 Hz");
            {
                // Angular-step quantisation is the whole point: the stored grid must
                // depend on the angle travelled, never on how fast it was travelled.
                HelixHistory slow, fast;
                driveHistory (slow, 0.02, orbitmath::pi, 400.0);
                driveHistory (fast, 4.00, orbitmath::pi, 2.0);

                double worstDelta = 0.0;
                for (int i = 0; i < HelixHistory::size; ++i)
                {
                    // Compare the phase of each sample RELATIVE to its newest entry,
                    // which is what determines the drawn shape.
                    const double slowRelative = slow.at (0).phaseA - slow.at (i).phaseA;
                    const double fastRelative = fast.at (0).phaseA - fast.at (i).phaseA;
                    worstDelta = juce::jmax (worstDelta, std::abs (slowRelative - fastRelative));
                }

                logMessage ("Worst grid difference across a 200x rate change: "
                            + juce::String (worstDelta, 12) + " rad");
                expectLessThan (worstDelta, 1.0e-9,
                                 "The stored angular grid must not depend on Rate");
            }

            beginTest ("History: phi == pi gives an exactly zero centroid at every sample");
            {
                HelixHistory history;
                driveHistory (history, 0.5, orbitmath::pi, 20.0);

                double worstCentroid = 0.0;
                for (int i = 0; i < HelixHistory::size; ++i)
                {
                    const auto& sample = history.at (i);
                    const auto a = orbitmath::computePosition (sample.phaseA, sample.radius01);
                    const auto b = orbitmath::computePosition (sample.phaseA + sample.phi, sample.radius01);
                    const auto centroid = orbitmath::computeCentroid (a, b);
                    worstCentroid = juce::jmax (worstCentroid, centroid.distance);
                }

                logMessage ("Worst centroid distance with phi == pi: " + juce::String (worstCentroid, 18));

                // Not exactly 0: sin(x) and sin(x + pi) are not bit-exact negatives
                // after rounding, so a residual around 1e-16 is expected and correct.
                expectLessThan (worstCentroid, 1.0e-12,
                                 "Antipodal strands must place the centroid at zero");
            }

            beginTest ("History: a phase error moves the centroid off the axis");
            {
                HelixHistory history;
                const double phi = orbitmath::pi + 0.5; // 0.5 rad of error
                driveHistory (history, 0.5, phi, 20.0);

                double maxCentroid = 0.0;
                for (int i = 0; i < HelixHistory::size; ++i)
                {
                    const auto& sample = history.at (i);
                    const auto a = orbitmath::computePosition (sample.phaseA, sample.radius01);
                    const auto b = orbitmath::computePosition (sample.phaseA + sample.phi, sample.radius01);
                    maxCentroid = juce::jmax (maxCentroid, orbitmath::computeCentroid (a, b).distance);
                }

                // Deviation amplitude is |cos(phi/2)|; at 0.5 rad of error that is ~0.247.
                expectGreaterThan (maxCentroid, 0.2, "Phase error must visibly displace the centroid");
            }

            beginTest ("History: backwards synthesis fills the buffer on the first poll");
            {
                HelixHistory history;
                HelixHistory::Live live;
                live.phaseA = 12.34;
                live.phi = orbitmath::pi;
                live.radius01 = 0.7;
                history.reset (live);

                // Every slot must carry a distinct, uniformly spaced phase, so the
                // helix appears complete the instant the editor opens rather than
                // dribbling in over many seconds at slow rates.
                for (int i = 0; i + 1 < HelixHistory::size; ++i)
                {
                    const double step = history.at (i).phaseA - history.at (i + 1).phaseA;
                    expectWithinAbsoluteError (step, HelixHistory::angleStep, 1.0e-9);
                }
            }

            beginTest ("History: a huge phase jump resynthesises instead of looping forever");
            {
                HelixHistory history;
                HelixHistory::Live live;
                live.phaseA = 0.0;
                live.phi = orbitmath::pi;
                live.radius01 = 0.8;
                history.reset (live);

                // Simulates a long message-thread stall.
                live.phaseA = 10000.0;
                history.advance (live);

                expectWithinAbsoluteError (history.at (0).phaseA, 10000.0, 1.0e-6);
                expect (history.isPrimed());

                // Going backwards must also be handled rather than spinning.
                live.phaseA = 5.0;
                history.advance (live);
                expectWithinAbsoluteError (history.at (0).phaseA, 5.0, 1.0e-6);
            }

            beginTest ("Projection: the tilt stays below the fold-over angle");
            {
                const double critical = Projection3D::criticalTiltRadians();
                logMessage ("Critical tilt = " + juce::String (critical * 180.0 / orbitmath::pi, 2)
                            + " deg, shipping "
                            + juce::String (Projection3D::tiltRadians * 180.0 / orbitmath::pi, 2) + " deg");

                expectLessThan (Projection3D::tiltRadians, critical,
                                 "Above the critical tilt the helix folds over itself");
            }

            beginTest ("Projection: the helix never folds back on itself");
            {
                // The tilt bound applies to the tilted height BEFORE the perspective
                // divide, so that is what must be strictly monotonic - this is what
                // rules out fold-over.
                const double c = std::cos (Projection3D::tiltRadians);
                const double s = std::sin (Projection3D::tiltRadians);

                double previousTilted = -1.0e9;
                double previousScreen = -1.0e9;
                bool tiltedMonotonic = true;
                double worstScreenBacktrack = 0.0;

                const int steps = 4000;
                for (int i = 0; i < steps; ++i)
                {
                    const double age = (double) i / (double) (steps - 1);
                    const double worldY = 1.0 - 2.0 * age;
                    const double theta = age * Projection3D::visibleTurns * orbitmath::twoPi;

                    const double tilted = -(worldY * Projection3D::scaleY * c
                                            - std::cos (theta) * Projection3D::scaleZ * s);
                    const auto p = Projection3D::project (std::sin (theta), worldY, std::cos (theta));

                    if (i > 0)
                    {
                        if (tilted <= previousTilted)
                            tiltedMonotonic = false;
                        if (p.y <= previousScreen)
                            worstScreenBacktrack = juce::jmax (worstScreenBacktrack, previousScreen - p.y);
                    }

                    previousTilted = tilted;
                    previousScreen = p.y;
                }

                expect (tiltedMonotonic, "Tilted height must increase monotonically - otherwise the helix folds over");

                // The perspective divide leaves microscopic flat spots. At the
                // largest supported scale (~150 px per unit) this bound is well
                // under a tenth of a pixel, so it cannot produce a visible cusp.
                logMessage ("Worst screen-y backtrack after perspective: "
                            + juce::String (worstScreenBacktrack, 9));
                expectLessThan (worstScreenBacktrack, 1.0e-3);
            }

            beginTest ("Projection: screen x depends only on pan, never on depth");
            {
                // Yaw must be exactly zero: for a stereo imaging plugin, letting
                // front/back position shift the horizontal reading would misreport
                // the one axis the listener actually hears.
                for (double z = -1.0; z <= 1.0; z += 0.25)
                {
                    const auto atCentre = Projection3D::project (0.0, 0.0, z);
                    expectWithinAbsoluteError (atCentre.x, 0.0, 1.0e-12,
                                                "Pan 0 must project to screen centre at any depth");
                }

                // And x must be monotonic in pan.
                double previousX = -1.0e9;
                for (double x = -1.0; x <= 1.0; x += 0.1)
                {
                    const auto p = Projection3D::project (x, 0.0, 0.0);
                    expectGreaterThan (p.x, previousX);
                    previousX = p.x;
                }
            }

            beginTest ("Projection: the geometry fits inside the component at any size");
            {
                const int sizes[][2] = { { 640, 400 }, { 780, 540 }, { 900, 620 },
                                          { 1200, 800 }, { 1600, 1100 }, { 400, 900 } };

                for (const auto& size : sizes)
                {
                    const double w = size[0], h = size[1];
                    const double margin = 16.0;
                    const auto fit = Projection3D::computeFit (w, h, margin);

                    // Sweep the full parameter space the drawing can produce.
                    for (int i = 0; i <= 60; ++i)
                    {
                        const double theta = (double) i / 60.0 * orbitmath::twoPi;
                        for (double worldY : { -1.0, 0.0, 1.0 })
                        {
                            const auto p = Projection3D::project (std::sin (theta), worldY, std::cos (theta));
                            const double sx = fit.toScreenX (p.x);
                            const double sy = fit.toScreenY (p.y);

                            expect (sx >= -0.5 && sx <= w + 0.5,
                                    "x out of bounds at " + juce::String (size[0]) + "x" + juce::String (size[1]));
                            expect (sy >= -0.5 && sy <= h + 0.5,
                                    "y out of bounds at " + juce::String (size[0]) + "x" + juce::String (size[1]));
                        }
                    }
                }
            }

            beginTest ("Projection: depth01 is ordered and bounded");
            {
                const auto near = Projection3D::project (0.0, 0.0, 1.0);
                const auto far  = Projection3D::project (0.0, 0.0, -1.0);

                expectGreaterThan (Projection3D::depth01 (near.viewZ), Projection3D::depth01 (far.viewZ));
                expect (Projection3D::depth01 (near.viewZ) <= 1.0);
                expect (Projection3D::depth01 (far.viewZ) >= 0.0);

                // Nearer geometry must also be drawn larger.
                expectGreaterThan (near.invW, far.invW);
            }
        }
    };

    static GeometryTests geometryTests;
}
