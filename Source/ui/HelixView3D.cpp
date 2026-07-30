#include "HelixView3D.h"
#include "DnaLookAndFeel.h"

#include <algorithm>
#include <cmath>

namespace dnaorbit::ui
{
    namespace
    {
        constexpr double tubeRadiusWorld = 0.075;
        constexpr double nodeRadiusWorld = 0.130;
        constexpr double headRadiusWorld = 0.190;
        constexpr float  centroidLockedThreshold = 1.0e-3f;
        constexpr float  symmetryLockThreshold = 0.999f;
        constexpr int    lockHoldFrameCount = 12; // ~250 ms at 45 fps

        /** Inset occupies a square in the bottom-right corner. */
        juce::Rectangle<float> insetBounds (juce::Rectangle<float> total)
        {
            const float side = juce::jlimit (70.0f, 130.0f, juce::jmin (total.getWidth(), total.getHeight()) * 0.30f);
            return { total.getRight() - side - 8.0f, total.getBottom() - side - 8.0f, side, side };
        }
    }

    HelixView3D::HelixView3D (dnaorbit::dsp::HelixEngine& engineToWatch)
        : engine (engineToWatch)
    {
        setOpaque (true);
        nodes.reserve (128);

        for (auto& p : strandPathsA) p.preallocateSpace (400);
        for (auto& p : strandPathsB) p.preallocateSpace (400);
        for (auto& p : rungPaths)    p.preallocateSpace (200);
        centreLinePath.preallocateSpace (400);
    }

    HelixView3D::~HelixView3D()
    {
        stopTimer();
    }

    void HelixView3D::visibilityChanged()       { updateTimerState(); }
    void HelixView3D::parentHierarchyChanged()  { updateTimerState(); }

    void HelixView3D::updateTimerState()
    {
        // Hosts routinely keep editors alive off-screen; animating one nobody can
        // see is pure waste on the shared message thread.
        //
        // A component with no peer at all is not "hidden by the host", it is
        // detached - which is what offline/screenshot rendering looks like - so
        // let that case animate. Once a host attaches the editor it always has a
        // peer, so the optimisation still applies where it matters.
        if (isShowing() || getPeer() == nullptr)
        {
            if (! isTimerRunning())
                startTimerHz (targetFps);
        }
        else
        {
            stopTimer();
        }
    }

    void HelixView3D::applyQualityTier (int tier)
    {
        qualityTier = juce::jlimit (0, 2, tier);

        switch (qualityTier)
        {
            case 0:  historyPoints = HelixHistory::size; nodesPerStrand = 20; glowBudget = 5; targetFps = 45; break;
            case 1:  historyPoints = 100;                nodesPerStrand = 12; glowBudget = 1; targetFps = 36; break;
            default: historyPoints = 72;                 nodesPerStrand = 8;  glowBudget = 0; targetFps = 24; break;
        }

        if (isTimerRunning())
            startTimerHz (targetFps);
    }

    void HelixView3D::resized()
    {
        const auto area = getLocalBounds().toFloat();
        if (area.isEmpty())
            return;

        fit = Projection3D::computeFit (area.getWidth(), area.getHeight(), 16.0);

        // Force the lower tier on very large windows before measuring anything.
        // Previously this only set the qualityTier NUMBER without updating the
        // geometry counts it is supposed to gate, so the forced downgrade did
        // nothing to actual render cost, and a later measured transition could
        // jump 1->2 directly, skipping tier 1's settings entirely.
        if (getWidth() * getHeight() > 500000 && qualityTier == 0)
            applyQualityTier (1);

        rebuildBackground();
        rebuildSprites();
    }

    void HelixView3D::rebuildSprites()
    {
        const auto bounds = Projection3D::computeBounds();
        const double invWNear = Projection3D::focalLength / (Projection3D::cameraDistance - bounds.maxViewZ);
        const double invWFar  = Projection3D::focalLength / (Projection3D::cameraDistance - bounds.minViewZ);

        const double nearDiameter = 2.0 * nodeRadiusWorld * invWNear * fit.scale;
        const double farDiameter  = 2.0 * nodeRadiusWorld * invWFar  * fit.scale;

        if (nearDiameter < 3.0)
            return;

        // Skip the rebuild if the scale barely moved (live resize sends many events).
        if (ladderA.isBuilt() && std::abs (nearDiameter - ladderA.builtNearestDiameter()) < nearDiameter * 0.06)
            return;

        nodeDiameterNear = nearDiameter;

        const auto bg = DnaLookAndFeel::backgroundColour();
        ladderA.build (DnaLookAndFeel::strandAColour(), bg, nearDiameter, farDiameter);
        ladderB.build (DnaLookAndFeel::strandBColour(), bg, nearDiameter, farDiameter);

        const int glowSize = juce::roundToInt (juce::jlimit (24.0, 160.0, nearDiameter * 4.0));
        glowA = SphereSprite::renderGlow (DnaLookAndFeel::strandAColour(), glowSize);
        glowB = SphereSprite::renderGlow (DnaLookAndFeel::strandBColour(), glowSize);
        glowCentre = SphereSprite::renderGlow (DnaLookAndFeel::centreLockedColour(), glowSize);
        glowDrift = SphereSprite::renderGlow (DnaLookAndFeel::centreDriftColour(), glowSize);
    }

    void HelixView3D::rebuildBackground()
    {
        const int w = getWidth(), h = getHeight();
        if (w <= 0 || h <= 0)
            return;

        juce::Image image { juce::Image::ARGB, w, h, true, juce::SoftwareImageType() };
        {
            juce::Graphics g { image };
            const auto area = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h);
            const auto bg = DnaLookAndFeel::backgroundColour();

            g.setGradientFill (juce::ColourGradient (bg.brighter (0.10f), area.getCentreX(), area.getCentreY(),
                                                      bg.darker (0.45f), area.getX(), area.getBottom(), true));
            g.fillRect (area);

            // The dead-straight ideal centre axis. Drawing the live midpoint over
            // this reference is what makes "exactly zero" visible: when locked, the
            // live line sits precisely on top of it.
            const float axisX = (float) fit.toScreenX (0.0);
            const auto topAxis = Projection3D::project (0.0, 1.0, 0.0);
            const auto bottomAxis = Projection3D::project (0.0, -1.0, 0.0);
            g.setColour (DnaLookAndFeel::panelColour().brighter (0.55f).withAlpha (0.5f));
            g.drawLine (axisX, (float) fit.toScreenY (topAxis.y),
                        axisX, (float) fit.toScreenY (bottomAxis.y), 1.0f);

            // Inset frame.
            const auto inset = insetBounds (area);
            g.setColour (DnaLookAndFeel::panelColour().brighter (0.25f).withAlpha (0.7f));
            g.drawRoundedRectangle (inset, 4.0f, 1.0f);
        }

        background = juce::NativeImageType().convert (image);
    }

    void HelixView3D::timerCallback()
    {
        const auto state = engine.getVisualState();

        // A host feeding NaN audio would otherwise put NaN into a Path, which the
        // rasteriser handles very badly.
        const auto safe = [] (float v, float fallback)
        {
            return std::isfinite (v) ? v : fallback;
        };

        HelixHistory::Live live;
        live.phaseA = std::isfinite (state.phaseA) ? state.phaseA : 0.0;
        live.phi = (double) safe (state.phi, (float) orbitmath::pi);
        live.radius01 = (double) juce::jlimit (0.0f, 1.0f, safe (state.radius01, 0.8f));

        if (! history.isPrimed())
            history.reset (live);
        else
            history.advance (live);

        // Readouts.
        const auto posA = orbitmath::computePosition (live.phaseA, live.radius01);
        const auto posB = orbitmath::computePosition (live.phaseA + live.phi, live.radius01);
        const auto centroid = orbitmath::computeCentroid (posA, posB);

        panA = (float) posA.x;
        panB = (float) posB.x;
        centroidX = (float) centroid.x;
        symmetryPercent = juce::jlimit (0.0f, 100.0f, safe (state.symmetry01, 1.0f) * 100.0f);
        correlation = juce::jlimit (-1.0f, 1.0f, safe (state.correlation, 1.0f));
        nullCoreOn = state.nullCoreOn;

        const double phaseError = orbitmath::shortestAngleDelta (orbitmath::pi, orbitmath::wrapTwoPi (live.phi));
        phaseErrorDegrees = (float) (phaseError * 180.0 / orbitmath::pi);

        // Lock detection needs the symmetry gate, not just the centroid distance:
        // centroidDistance is proportional to |cos(phi/2)|, which passes through zero
        // every time phi sweeps through pi while drifting. Testing the distance alone
        // makes the label flash "LOCKED" mid-drift.
        const bool instantaneousLock = safe (state.symmetry01, 1.0f) >= symmetryLockThreshold
                                     && safe (state.centroidDistance, 0.0f) < centroidLockedThreshold;

        if (instantaneousLock)
        {
            axisLocked = true;
            lockHoldFrames = lockHoldFrameCount;
        }
        else if (lockHoldFrames > 0)
        {
            --lockHoldFrames;
        }
        else
        {
            axisLocked = false;
        }

        // UI-side one-pole on the level, so the audio thread only publishes raw RMS.
        const float rms = juce::jlimit (0.0f, 4.0f, safe (state.outputRms, 0.0f));
        smoothedRms += (rms - smoothedRms) * 0.18f;

        // Centroid wander trail for the inset.
        const auto insetArea = insetBounds (getLocalBounds().toFloat());
        const float insetRadius = insetArea.getWidth() * 0.5f - 6.0f;
        insetCentroidTrail[(size_t) insetTrailWrite] = {
            insetArea.getCentreX() + (float) (centroid.x * Projection3D::scaleX / 1.45) * insetRadius,
            insetArea.getCentreY() - (float) centroid.z * insetRadius
        };
        insetTrailWrite = (insetTrailWrite + 1) % (int) std::size (insetCentroidTrail);

        repaint();
    }

    void HelixView3D::buildFrameGeometry()
    {
        for (auto& p : strandPathsA) p.clear();
        for (auto& p : strandPathsB) p.clear();
        for (auto& p : rungPaths)    p.clear();
        centreLinePath.clear();
        nodes.clear();

        const int count = historyPoints;
        const int stride = juce::jmax (1, count / juce::jmax (1, nodesPerStrand));

        struct Point { juce::Point<float> screen; double viewZ; double halfWidth; bool valid; };
        std::array<Point, (size_t) HelixHistory::size> pointsA {}, pointsB {};

        for (int i = 0; i < count; ++i)
        {
            const auto& sample = history.at (i);
            const double age = history.ageOf (i);
            const double worldY = 1.0 - 2.0 * age;

            const auto pa = orbitmath::computePosition (sample.phaseA, sample.radius01);
            const auto pb = orbitmath::computePosition (sample.phaseA + sample.phi, sample.radius01);

            const auto projA = Projection3D::project (pa.x, worldY, pa.z);
            const auto projB = Projection3D::project (pb.x, worldY, pb.z);

            pointsA[(size_t) i] = { { (float) fit.toScreenX (projA.x), (float) fit.toScreenY (projA.y) },
                                    projA.viewZ, tubeRadiusWorld * projA.invW * fit.scale, true };
            pointsB[(size_t) i] = { { (float) fit.toScreenX (projB.x), (float) fit.toScreenY (projB.y) },
                                    projB.viewZ, tubeRadiusWorld * projB.invW * fit.scale, true };

            // Centre line: the midpoint, computed with the very same OrbitMath
            // functions the audio path uses. With phi == pi this is exactly zero.
            const auto centroid = orbitmath::computeCentroid (pa, pb);
            const auto projMid = Projection3D::project (centroid.x, worldY, centroid.z);
            const juce::Point<float> mid { (float) fit.toScreenX (projMid.x), (float) fit.toScreenY (projMid.y) };

            if (i == 0)
                centreLinePath.startNewSubPath (mid);
            else
                centreLinePath.lineTo (mid);

            if (i % stride == 0)
            {
                nodes.push_back ({ pointsA[(size_t) i].screen, Projection3D::depth01 (projA.viewZ),
                                   projA.invW, false });
                nodes.push_back ({ pointsB[(size_t) i].screen, Projection3D::depth01 (projB.viewZ),
                                   projB.invW, true });
            }
        }

        // Ribbon quads. Consecutive quads share their joint edge exactly, so seams
        // cannot appear and it is safe to bucket them by depth-shade.
        const auto addRibbon = [&] (const std::array<Point, (size_t) HelixHistory::size>& pts,
                                    std::array<juce::Path, (size_t) numBuckets>& target)
        {
            for (int i = 0; i + 1 < count; ++i)
            {
                const auto& p0 = pts[(size_t) i];
                const auto& p1 = pts[(size_t) i + 1];

                auto tangent = p1.screen - p0.screen;
                const float length = tangent.getDistanceFromOrigin();
                if (length < 1.0e-4f)
                    continue;

                tangent /= length;
                const juce::Point<float> normal { tangent.y, -tangent.x };

                const auto w0 = normal * (float) p0.halfWidth;
                const auto w1 = normal * (float) p1.halfWidth;

                const double meanZ = 0.5 * (p0.viewZ + p1.viewZ);
                const int bucket = juce::jlimit (0, numBuckets - 1,
                                                 (int) (Projection3D::depth01 (meanZ) * (numBuckets - 1) + 0.5));

                auto& path = target[(size_t) bucket];
                path.startNewSubPath (p0.screen + w0);
                path.lineTo (p1.screen + w1);
                path.lineTo (p1.screen - w1);
                path.lineTo (p0.screen - w0);
                path.closeSubPath();
            }
        };

        addRibbon (pointsA, strandPathsA);
        addRibbon (pointsB, strandPathsB);

        // Base-pair rungs.
        const int rungStride = juce::jmax (1, count / juce::jmax (1, nodesPerStrand));
        for (int i = 0; i < count; i += rungStride)
        {
            const auto& a = pointsA[(size_t) i];
            const auto& b = pointsB[(size_t) i];

            auto tangent = b.screen - a.screen;
            const float length = tangent.getDistanceFromOrigin();
            if (length < 1.0e-4f)
                continue;
            tangent /= length;
            const juce::Point<float> normal { tangent.y * 2.2f, -tangent.x * 2.2f };

            const double meanZ = 0.5 * (a.viewZ + b.viewZ);
            const int bucket = juce::jlimit (0, numBuckets - 1,
                                             (int) (Projection3D::depth01 (meanZ) * (numBuckets - 1) + 0.5));

            auto& path = rungPaths[(size_t) bucket];
            path.startNewSubPath (a.screen + normal);
            path.lineTo (b.screen + normal);
            path.lineTo (b.screen - normal);
            path.lineTo (a.screen - normal);
            path.closeSubPath();
        }
    }

    void HelixView3D::paint (juce::Graphics& g)
    {
        const auto startTicks = juce::Time::getHighResolutionTicks();

        if (background.isValid())
            g.drawImageAt (background, 0, 0);
        else
            g.fillAll (DnaLookAndFeel::backgroundColour());

        if (! history.isPrimed() || getWidth() <= 0)
            return;

        buildFrameGeometry();

        g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);

        const auto bg = DnaLookAndFeel::backgroundColour();
        const auto colourA = DnaLookAndFeel::strandAColour();
        const auto colourB = DnaLookAndFeel::strandBColour();
        const auto rungColour = juce::Colours::white.withAlpha (0.55f);

        // Fog per bucket: interpolate toward the background. Fog only, not alpha -
        // over a near-black background they look the same, and keeping the fills
        // opaque avoids compounding alpha at the ribbon joints.
        const auto fogged = [&bg] (juce::Colour c, int bucket)
        {
            const float t = (float) bucket / (float) (numBuckets - 1);
            return bg.interpolatedWith (c, 0.28f + 0.72f * std::pow (t, 0.85f));
        };

        // Ribbon/rung fills are already far-to-near via the bucket loop below;
        // sprites additionally need sorting within that so nodes emit in
        // depth order as each bucket is reached. `nodes` is fully rebuilt by
        // buildFrameGeometry() every frame before this runs, so nothing
        // depends on its original insertion order - sorting it in place
        // avoids a same-size heap-allocating copy on every single paint().
        std::size_t nodeIndex = 0;
        std::sort (nodes.begin(), nodes.end(),
                   [] (const NodeDraw& x, const NodeDraw& y) { return x.depth01 < y.depth01; });

        for (int bucket = 0; bucket < numBuckets; ++bucket)
        {
            const float bucketDepth = (float) bucket / (float) (numBuckets - 1);

            if (! rungPaths[(size_t) bucket].isEmpty())
            {
                g.setColour (fogged (rungColour, bucket));
                g.fillPath (rungPaths[(size_t) bucket]);
            }

            if (! strandPathsA[(size_t) bucket].isEmpty())
            {
                g.setColour (fogged (colourA, bucket));
                g.fillPath (strandPathsA[(size_t) bucket]);
            }

            if (! strandPathsB[(size_t) bucket].isEmpty())
            {
                g.setColour (fogged (colourB, bucket));
                g.fillPath (strandPathsB[(size_t) bucket]);
            }

            // Emit any spheres that belong at this depth.
            while (nodeIndex < nodes.size()
                   && nodes[nodeIndex].depth01 <= (double) bucketDepth + 1.0e-9)
            {
                const auto& node = nodes[nodeIndex];
                const auto& ladder = node.strandB ? ladderB : ladderA;

                if (ladder.isBuilt())
                {
                    const auto& sprite = ladder.forDepth (node.depth01);
                    const double wanted = 2.0 * nodeRadiusWorld * node.scale * fit.scale;
                    const double spriteSize = (double) sprite.getWidth();

                    if (spriteSize > 0.0)
                    {
                        // Always <= 1 in practice, so we only ever downsample.
                        const float s = (float) juce::jmin (1.0, wanted / spriteSize);
                        g.setOpacity (1.0f);
                        g.drawImageTransformed (sprite,
                            juce::AffineTransform::scale (s)
                                .translated (node.position.x - (float) spriteSize * s * 0.5f,
                                             node.position.y - (float) spriteSize * s * 0.5f));
                    }
                }

                ++nodeIndex;
            }
        }

        // --- Centre line: the physics made visible --------------------------------
        const bool locked = axisLocked;
        const auto centreColour = locked ? DnaLookAndFeel::centreLockedColour()
                                         : DnaLookAndFeel::centreDriftColour();

        g.setColour (centreColour.withAlpha (0.22f));
        g.strokePath (centreLinePath, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));
        g.setColour (centreColour.withAlpha (locked ? 0.95f : 0.85f));
        g.strokePath (centreLinePath, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                            juce::PathStrokeType::rounded));

        // --- Live heads, with an audio-reactive glow -------------------------------
        if (glowBudget > 0 && history.isPrimed())
        {
            const auto& live = history.liveState();
            const auto pa = orbitmath::computePosition (live.phaseA, live.radius01);
            const auto pb = orbitmath::computePosition (live.phaseA + live.phi, live.radius01);
            const auto projA = Projection3D::project (pa.x, 1.0, pa.z);
            const auto projB = Projection3D::project (pb.x, 1.0, pb.z);

            const float glowOpacity = juce::jlimit (0.15f, 0.85f, 0.30f + 1.6f * smoothedRms);

            const auto drawGlowAndHead = [&] (const juce::Image& glow, const SpriteLadder& ladder,
                                              const Projection3D::Projected& proj)
            {
                const juce::Point<float> centre { (float) fit.toScreenX (proj.x),
                                                  (float) fit.toScreenY (proj.y) };

                if (glow.isValid())
                {
                    const float size = (float) glow.getWidth();
                    g.setOpacity (glowOpacity);
                    g.drawImageTransformed (glow, juce::AffineTransform::translation (
                        centre.x - size * 0.5f, centre.y - size * 0.5f));
                    g.setOpacity (1.0f);
                }

                if (ladder.isBuilt())
                {
                    const auto& sprite = ladder.forDepth (Projection3D::depth01 (proj.viewZ));
                    const double wanted = 2.0 * headRadiusWorld * proj.invW * fit.scale;
                    const double spriteSize = (double) sprite.getWidth();
                    if (spriteSize > 0.0)
                    {
                        const float s = (float) (wanted / spriteSize);
                        g.drawImageTransformed (sprite,
                            juce::AffineTransform::scale (s)
                                .translated (centre.x - (float) spriteSize * s * 0.5f,
                                             centre.y - (float) spriteSize * s * 0.5f));
                    }
                }
            };

            drawGlowAndHead (glowA, ladderA, projA);
            drawGlowAndHead (glowB, ladderB, projB);

            // Core halo on the axis.
            const auto& coreGlow = locked ? glowCentre : glowDrift;
            if (coreGlow.isValid())
            {
                const auto projMid = Projection3D::project (0.0, 1.0, 0.0);
                const float size = (float) coreGlow.getWidth() * 0.7f;
                g.setOpacity (glowOpacity * 0.5f);
                g.drawImageTransformed (coreGlow, juce::AffineTransform::scale (0.7f).translated (
                    (float) fit.toScreenX (projMid.x) - size * 0.5f,
                    (float) fit.toScreenY (projMid.y) - size * 0.5f));
                g.setOpacity (1.0f);
            }
        }

        drawInset (g);

        // --- Adaptive quality, driven by measurement rather than guesswork ---------
        const double elapsedMs = (double) (juce::Time::getHighResolutionTicks() - startTicks)
                               * 1000.0 / (double) juce::Time::getHighResolutionTicksPerSecond();
        paintTimeEmaMs += (elapsedMs - paintTimeEmaMs) * 0.1;

        const double frameIntervalMs = 1000.0 / (double) juce::jmax (1, targetFps);

        if (paintTimeEmaMs > frameIntervalMs * 0.55)
        {
            if (++framesOverBudget > 20 && qualityTier < 2)
            {
                framesOverBudget = 0;
                framesUnderBudget = 0;
                applyQualityTier (qualityTier + 1);
            }
        }
        else
        {
            framesOverBudget = 0;
            if (paintTimeEmaMs < frameIntervalMs * 0.25 && ++framesUnderBudget > 120 && qualityTier > 0)
            {
                framesUnderBudget = 0;
                applyQualityTier (qualityTier - 1);
            }
        }
    }

    void HelixView3D::drawInset (juce::Graphics& g) const
    {
        const auto area = insetBounds (getLocalBounds().toFloat());
        const auto& live = history.liveState();
        const float radiusPx = area.getWidth() * 0.5f - 6.0f;
        const auto centre = area.getCentre();

        // The true orbit is an ellipse: OrbitMath scales only x by radius, so at
        // Radius 0% it collapses to a vertical line. Draw that honestly - it is
        // exactly what the inset exists to explain.
        const float semiX = juce::jmax (0.5f, (float) live.radius01 * radiusPx);
        g.setColour (DnaLookAndFeel::panelColour().brighter (0.5f).withAlpha (0.8f));
        g.drawEllipse (centre.x - semiX, centre.y - radiusPx, semiX * 2.0f, radiusPx * 2.0f, 1.0f);

        const auto toInset = [&] (double x, double z)
        {
            return juce::Point<float> (centre.x + (float) x * radiusPx,
                                        centre.y - (float) z * radiusPx);
        };

        const auto pa = orbitmath::computePosition (live.phaseA, live.radius01);
        const auto pb = orbitmath::computePosition (live.phaseA + live.phi, live.radius01);
        const auto centroid = orbitmath::computeCentroid (pa, pb);

        const auto screenA = toInset (pa.x, pa.z);
        const auto screenB = toInset (pb.x, pb.z);

        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawLine ({ screenA, screenB }, 1.0f);

        // Centroid wander trail: drift renders as a visible scribble instead of a
        // stationary dot.
        for (int i = 0; i < (int) std::size (insetCentroidTrail); ++i)
        {
            const auto& p = insetCentroidTrail[(size_t) i];
            if (p.isOrigin())
                continue;
            const float alpha = 0.06f + 0.10f * ((float) i / (float) std::size (insetCentroidTrail));
            g.setColour ((axisLocked ? DnaLookAndFeel::centreLockedColour()
                                     : DnaLookAndFeel::centreDriftColour()).withAlpha (alpha));
            g.fillEllipse (juce::Rectangle<float> (2.0f, 2.0f).withCentre (p));
        }

        g.setColour (DnaLookAndFeel::strandAColour());
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (screenA));
        g.setColour (DnaLookAndFeel::strandBColour());
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (screenB));

        g.setColour (axisLocked ? DnaLookAndFeel::centreLockedColour() : DnaLookAndFeel::centreDriftColour());
        g.fillEllipse (juce::Rectangle<float> (4.5f, 4.5f).withCentre (toInset (centroid.x, centroid.z)));

        g.setColour (DnaLookAndFeel::textColour().withAlpha (0.45f));
        g.setFont (DnaLookAndFeel::uiFont (9.5f));
        g.drawText ("L", area.getX() + 2.0f, area.getCentreY() - 6.0f, 10.0f, 12.0f, juce::Justification::centredLeft);
        g.drawText ("R", area.getRight() - 12.0f, area.getCentreY() - 6.0f, 10.0f, 12.0f, juce::Justification::centredRight);
        g.drawText (jp("前"), area.getCentreX() - 8.0f, area.getY() + 1.0f, 16.0f, 11.0f, juce::Justification::centred);
        g.drawText (jp("後"), area.getCentreX() - 8.0f, area.getBottom() - 12.0f, 16.0f, 11.0f, juce::Justification::centred);
    }
}
