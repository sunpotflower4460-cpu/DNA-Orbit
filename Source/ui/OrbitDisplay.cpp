#include "OrbitDisplay.h"
#include "DnaLookAndFeel.h"

namespace dnaorbit::ui
{
    namespace
    {
        constexpr float axisLockThreshold = 0.01f;
    }

    OrbitDisplay::OrbitDisplay (dnaorbit::dsp::HelixEngine& engineToWatch)
        : engine (engineToWatch)
    {
        setOpaque (true);
        startTimerHz (45);
    }

    OrbitDisplay::~OrbitDisplay()
    {
        stopTimer();
    }

    void OrbitDisplay::timerCallback()
    {
        lastState = engine.getVisualState();

        const auto posA = dnaorbit::orbitmath::computePosition (lastState.thetaA, lastState.radius01);
        const auto posB = dnaorbit::orbitmath::computePosition (lastState.thetaB, lastState.radius01);

        const auto bounds = getLocalBounds().toFloat();
        const auto centre = bounds.getCentre();
        const float displayRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f * 0.78f;

        auto toScreen = [&] (double x, double z)
        {
            return juce::Point<float> (centre.x + (float) x * displayRadius,
                                        centre.y - (float) z * displayRadius);
        };

        TrailPoint tp;
        tp.a = toScreen (posA.x, posA.z);
        tp.b = toScreen (posB.x, posB.z);
        tp.mid = toScreen (lastState.centroidX, lastState.centroidZ);

        trail.push_back (tp);
        while (trail.size() > maxTrailLength)
            trail.pop_front();

        repaint();
    }

    void OrbitDisplay::resized()
    {
        trail.clear();
    }

    void OrbitDisplay::paint (juce::Graphics& g)
    {
        g.fillAll (DnaLookAndFeel::backgroundColour());

        const auto bounds = getLocalBounds().toFloat();
        const auto centre = bounds.getCentre();
        const float displayRadius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f * 0.78f;

        // Orbit ring + centre axis.
        g.setColour (DnaLookAndFeel::panelColour().brighter (0.15f));
        g.drawEllipse (centre.x - displayRadius, centre.y - displayRadius, displayRadius * 2.0f, displayRadius * 2.0f, 1.5f);
        g.setColour (DnaLookAndFeel::panelColour().brighter (0.3f).withAlpha (0.6f));
        g.drawLine (centre.x - displayRadius, centre.y, centre.x + displayRadius, centre.y, 0.75f);
        g.drawLine (centre.x, centre.y - displayRadius, centre.x, centre.y + displayRadius, 0.75f);

        // Trail (fading).
        for (size_t i = 0; i < trail.size(); ++i)
        {
            const float alpha = (float) (i + 1) / (float) trail.size() * 0.35f;
            const auto& tp = trail[i];
            g.setColour (DnaLookAndFeel::strandAColour().withAlpha (alpha));
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tp.a));
            g.setColour (DnaLookAndFeel::strandBColour().withAlpha (alpha));
            g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (tp.b));
        }

        if (! trail.empty())
        {
            const auto& latest = trail.back();

            // Line connecting the two strands.
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawLine ({ latest.a, latest.b }, 1.0f);

            // Strand dots.
            g.setColour (DnaLookAndFeel::strandAColour());
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (latest.a));
            g.setColour (DnaLookAndFeel::strandBColour());
            g.fillEllipse (juce::Rectangle<float> (12.0f, 12.0f).withCentre (latest.b));

            // Centroid (space midpoint).
            const bool locked = lastState.centroidDistance < axisLockThreshold;
            g.setColour (locked ? DnaLookAndFeel::centreLockedColour() : DnaLookAndFeel::centreDriftColour());
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (latest.mid));
        }

        // Status text.
        const bool locked = lastState.centroidDistance < axisLockThreshold;
        g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
        g.setColour (locked ? DnaLookAndFeel::centreLockedColour() : DnaLookAndFeel::centreDriftColour());
        auto topTextBounds = bounds;
        g.drawText (locked ? "AXIS LOCKED" : "AXIS DRIFT",
                    topTextBounds.removeFromTop (28.0f), juce::Justification::centred);

        g.setFont (juce::FontOptions (13.0f));
        g.setColour (DnaLookAndFeel::textColour());
        g.drawText (juce::String ("Symmetry ") + juce::String (lastState.symmetry01 * 100.0f, 0) + "%",
                    getLocalBounds().removeFromBottom (44).removeFromTop (20).toFloat(),
                    juce::Justification::centred);

        if (lastState.nullCoreOn)
        {
            g.setColour (DnaLookAndFeel::warningColour());
            g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
            g.drawText ("NULL CORE - MONO MAY DISAPPEAR",
                        getLocalBounds().removeFromBottom (22).toFloat(),
                        juce::Justification::centred);
        }
    }
}
