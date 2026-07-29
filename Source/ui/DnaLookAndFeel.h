#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dnaorbit::ui
{
    /** Shared colour palette + rotary slider styling for DNA Orbit. */
    class DnaLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DnaLookAndFeel();

        static juce::Colour strandAColour()      { return juce::Colour::fromRGB (60, 200, 235); }   // blue-cyan
        static juce::Colour strandBColour()      { return juce::Colour::fromRGB (120, 220, 130); }  // yellow-green / emerald
        static juce::Colour centreLockedColour() { return juce::Colours::white; }
        static juce::Colour centreDriftColour()  { return juce::Colour::fromRGB (255, 150, 40); }    // orange
        static juce::Colour backgroundColour()   { return juce::Colour::fromRGB (6, 10, 22); }       // near-black navy
        static juce::Colour panelColour()        { return juce::Colour::fromRGB (14, 18, 34); }
        static juce::Colour textColour()         { return juce::Colour::fromRGB (200, 210, 225); }
        static juce::Colour warningColour()      { return juce::Colour::fromRGB (235, 70, 60); }

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                                float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                                juce::Slider&) override;
    };
}
