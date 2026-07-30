#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

#include "../dsp/HelixEngine.h"
#include "HelixHistory.h"
#include "Projection3D.h"
#include "SphereSprite.h"

namespace dnaorbit::ui
{
    /**
     * The 3D double-helix visualiser.
     *
     * Real 3D geometry, perspective projection, depth sorting and lighting, all
     * rasterised through JUCE's normal 2D API - which on Windows is Direct2D and
     * on macOS is CoreGraphics, so this is GPU-composited without ever creating an
     * OpenGL context (and therefore without the context-loss and DAW-conflict
     * failure modes that come with one).
     *
     * The helix is the true time-history of the two strands' (x, z) positions, so
     * the DNA shape is a consequence of the physics rather than a decoration, and
     * the running midpoint of the two strands is exactly the centre axis.
     */
    class HelixView3D : public juce::Component,
                        private juce::Timer
    {
    public:
        explicit HelixView3D (dnaorbit::dsp::HelixEngine& engineToWatch);
        ~HelixView3D() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        void visibilityChanged() override;
        void parentHierarchyChanged() override;

        // --- Readouts, polled by the editor at a low rate ------------------------
        bool  isAxisLocked() const noexcept { return axisLocked; }
        float getPanA() const noexcept { return panA; }
        float getPanB() const noexcept { return panB; }
        float getCentroidX() const noexcept { return centroidX; }
        float getPhaseErrorDegrees() const noexcept { return phaseErrorDegrees; }
        float getSymmetryPercent() const noexcept { return symmetryPercent; }
        float getCorrelation() const noexcept { return correlation; }
        bool  isNullCoreOn() const noexcept { return nullCoreOn; }

    private:
        void timerCallback() override;
        void updateTimerState();
        void rebuildBackground();
        void rebuildSprites();

        struct Quad
        {
            juce::Point<float> a, b, c, d;
        };

        void buildFrameGeometry();
        void drawInset (juce::Graphics&) const;

        dnaorbit::dsp::HelixEngine& engine;
        HelixHistory history;

        // --- Cached, size-dependent resources ------------------------------------
        juce::Image background;
        SpriteLadder ladderA, ladderB;
        juce::Image glowA, glowB, glowCentre, glowDrift;
        Projection3D::Fit fit;
        double nodeDiameterNear = 20.0;

        // --- Per-frame geometry, reused so paint() never allocates ----------------
        static constexpr int numBuckets = 24;
        std::array<juce::Path, (size_t) numBuckets> strandPathsA, strandPathsB, rungPaths;
        juce::Path centreLinePath;

        struct NodeDraw { juce::Point<float> position; double depth01; double scale; bool strandB; };
        std::vector<NodeDraw> nodes;

        int historyPoints = HelixHistory::size;
        int nodesPerStrand = 20;
        int glowBudget = 5;
        int targetFps = 45;

        // Adaptive quality: measured, not guessed.
        double paintTimeEmaMs = 0.0;
        int framesOverBudget = 0;
        int framesUnderBudget = 0;
        int qualityTier = 0;

        // Smoothed on the UI side so the audio thread stays cheap.
        float smoothedRms = 0.0f;

        // Readout state.
        bool  axisLocked = true;
        int   lockHoldFrames = 0;
        float panA = 0.0f, panB = 0.0f, centroidX = 0.0f;
        float phaseErrorDegrees = 0.0f, symmetryPercent = 100.0f, correlation = 1.0f;
        bool  nullCoreOn = false;

        juce::Point<float> insetCentroidTrail[48] {};
        int insetTrailWrite = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelixView3D)
    };
}
