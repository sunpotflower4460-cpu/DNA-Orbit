#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dnaorbit::ui
{
    inline juce::String jp (const char* utf8Literal)
    {
        return juce::String (juce::CharPointer_UTF8 (utf8Literal));
    }

    /** Shared visual system for DNA Orbit.
        The palette is deliberately restrained: colour communicates strand identity,
        active state, or warning state rather than decorating every surface. */
    class DnaLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DnaLookAndFeel();

        static juce::Colour strandAColour()      { return juce::Colour::fromRGB (72, 205, 238); }
        static juce::Colour strandBColour()      { return juce::Colour::fromRGB (128, 224, 151); }
        static juce::Colour centreLockedColour() { return juce::Colour::fromRGB (236, 245, 255); }
        static juce::Colour centreDriftColour()  { return juce::Colour::fromRGB (255, 166, 72); }
        static juce::Colour backgroundColour()   { return juce::Colour::fromRGB (5, 9, 19); }
        static juce::Colour surfaceColour()      { return juce::Colour::fromRGB (12, 18, 34); }
        static juce::Colour raisedColour()       { return juce::Colour::fromRGB (18, 26, 46); }
        static juce::Colour panelColour()        { return surfaceColour(); }
        static juce::Colour borderColour()       { return juce::Colour::fromRGB (55, 69, 96); }
        static juce::Colour textColour()         { return juce::Colour::fromRGB (222, 231, 244); }
        static juce::Colour mutedTextColour()    { return juce::Colour::fromRGB (137, 151, 174); }
        static juce::Colour warningColour()      { return juce::Colour::fromRGB (244, 101, 89); }
        static juce::Colour focusColour()        { return juce::Colour::fromRGB (112, 215, 246); }

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                                   bool highlighted, bool down) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                               bool highlighted, bool down) override;
        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox&) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

        static juce::String japaneseTypefaceName();
        static juce::Font uiFont (float height, bool bold = false);

        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        juce::Font getSliderPopupFont (juce::Slider&) override;
    };
}
