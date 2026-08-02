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
     * rasterised through JUCE's normal 2D API. The view implements a settable
     * tooltip so its meaning is available even though it has no direct controls.
     */
    class HelixView3D : public juce::Component,
                        public juce::SettableTooltipClient,
                        private juce::Timer
    {
    public:
        explicit HelixView3D (dnaorbit::dsp::HelixEngine& engineToWatch);
        ~HelixView3D() override;

        void paint (juce::Graphics&) override;
        void resized() override;
        void visibilityChanged() override;
        void parentHierarchyChanged() override;

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
        void applyQualityTier (int tier);

        struct Quad
        {
            juce::Point<float> a, b, c, d;
        };

        void buildFrameGeometry();
        void drawInset (juce::Graphics&) const;

        dnaorbit::dsp::HelixEngine& engine;
        HelixHistory history;

        juce::Image background;
        SpriteLadder ladderA, ladderB;
        juce::Image glowA, glowB, glowCentre, glowDrift;
        Projection3D::Fit fit;
        double nodeDiameterNear = 20.0;

        static constexpr int numBuckets = 24;
        std::array<juce::Path, (size_t) numBuckets> strandPathsA, strandPathsB, rungPaths;
        juce::Path centreLinePath;

        struct NodeDraw
        {
            juce::Point<float> position;
            double depth01;
            double scale;
            bool strandB;
        };
        std::vector<NodeDraw> nodes;

        int historyPoints = HelixHistory::size;
        int nodesPerStrand = 20;
        int glowBudget = 5;
        int targetFps = 45;

        double paintTimeEmaMs = 0.0;
        int framesOverBudget = 0;
        int framesUnderBudget = 0;
        int qualityTier = 0;

        float smoothedRms = 0.0f;

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
