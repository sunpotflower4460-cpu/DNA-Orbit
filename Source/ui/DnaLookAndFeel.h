#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dnaorbit::ui
{
    /**
     * Wraps a UTF-8 string literal for JUCE.
     *
     * juce::String's const char* constructor parses its input as CharPointer_ASCII
     * (see juce_String.cpp), so any literal containing bytes above 127 - i.e. every
     * Japanese string in this UI - is mangled unless it is handed over explicitly
     * as UTF-8. Every Japanese literal in the plugin goes through here.
     */
    inline juce::String jp (const char* utf8Literal)
    {
        return juce::String (juce::CharPointer_UTF8 (utf8Literal));
    }

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

        /**
         * Name of a font family that actually carries Japanese glyphs.
         *
         * The UI text is Japanese, and the platform default sans often is not:
         * on Linux it resolves to DejaVu Sans, which has no CJK coverage at all.
         * Naming a real Japanese family keeps the text legible and, on Linux,
         * also avoids fontconfig's habit of answering "sans:lang=ja" with a
         * Chinese face whose Han glyph shapes are wrong for Japanese.
         */
        static juce::String japaneseTypefaceName();

        /** Japanese-capable font at a given height. */
        static juce::Font uiFont (float height, bool bold = false);

        // Route every stock control through the Japanese-capable font.
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        juce::Font getSliderPopupFont (juce::Slider&) override;
    };
}
