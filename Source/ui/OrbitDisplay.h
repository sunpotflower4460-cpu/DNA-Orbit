#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <deque>
#include "../dsp/HelixEngine.h"

namespace dnaorbit::ui
{
    /**
     * Central visualiser: draws the shared orbit axis, both strands, the
     * line connecting them, their midpoint (the space centroid), a short
     * trail of recent positions, and status text (AXIS LOCKED/DRIFT,
     * Symmetry, NULL CORE warning).
     *
     * Polls HelixEngine's lock-free visual state on a timer; never touches
     * the audio thread directly.
     */
    class OrbitDisplay : public juce::Component,
                         private juce::Timer
    {
    public:
        explicit OrbitDisplay (dnaorbit::dsp::HelixEngine& engineToWatch);
        ~OrbitDisplay() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;

        dnaorbit::dsp::HelixEngine& engine;

        struct TrailPoint { juce::Point<float> a, b, mid; };
        std::deque<TrailPoint> trail;
        static constexpr size_t maxTrailLength = 24;

        dnaorbit::dsp::HelixEngine::VisualState lastState;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbitDisplay)
    };
}
