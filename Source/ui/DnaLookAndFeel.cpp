#include "DnaLookAndFeel.h"

#include <cmath>

namespace dnaorbit::ui
{
    DnaLookAndFeel::DnaLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, backgroundColour());
        setColour (juce::Slider::thumbColourId, strandAColour());
        setColour (juce::Slider::rotarySliderFillColourId, strandAColour());
        setColour (juce::Slider::rotarySliderOutlineColourId, borderColour());
        setColour (juce::Slider::textBoxBackgroundColourId, raisedColour().withAlpha (0.82f));
        setColour (juce::Slider::textBoxTextColourId, textColour());
        setColour (juce::Slider::textBoxOutlineColourId, borderColour().withAlpha (0.55f));
        setColour (juce::Label::textColourId, textColour());
        setColour (juce::ToggleButton::textColourId, textColour());
        setColour (juce::ToggleButton::tickColourId, strandBColour());
        setColour (juce::ComboBox::backgroundColourId, raisedColour());
        setColour (juce::ComboBox::textColourId, textColour());
        setColour (juce::ComboBox::outlineColourId, borderColour().withAlpha (0.75f));
        setColour (juce::ComboBox::arrowColourId, mutedTextColour());
        setColour (juce::PopupMenu::backgroundColourId, raisedColour());
        setColour (juce::PopupMenu::textColourId, textColour());
        setColour (juce::PopupMenu::highlightedBackgroundColourId,
                   strandAColour().withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TextButton::textColourOffId, textColour());
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }

    juce::String DnaLookAndFeel::japaneseTypefaceName()
    {
       #if JUCE_MAC || JUCE_IOS
        return "Hiragino Sans";
       #elif JUCE_WINDOWS
        return "Yu Gothic UI";
       #else
        return "Noto Sans CJK JP";
       #endif
    }

    juce::Font DnaLookAndFeel::uiFont (float height, bool bold)
    {
        auto options = juce::FontOptions().withName (japaneseTypefaceName()).withHeight (height);
        if (bold)
            options = options.withStyle ("Bold");
        return juce::Font (options);
    }

    juce::Font DnaLookAndFeel::getLabelFont (juce::Label& label)
    {
        return uiFont (label.getFont().getHeight(), label.getFont().isBold());
    }

    juce::Font DnaLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
    {
        return uiFont (juce::jlimit (11.0f, 15.0f, (float) buttonHeight * 0.47f), true);
    }

    juce::Font DnaLookAndFeel::getComboBoxFont (juce::ComboBox& box)
    {
        return uiFont (juce::jlimit (11.0f, 14.0f, (float) box.getHeight() * 0.46f));
    }

    juce::Font DnaLookAndFeel::getPopupMenuFont()
    {
        return uiFont (14.0f);
    }

    juce::Font DnaLookAndFeel::getSliderPopupFont (juce::Slider&)
    {
        return uiFont (13.0f, true);
    }

    void DnaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y,
                                           int width, int height,
                                           float sliderPosProportional,
                                           float rotaryStartAngle,
                                           float rotaryEndAngle,
                                           juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (7.0f);
        const float diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const float radius = diameter * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = rotaryStartAngle
                          + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float enabledAlpha = slider.isEnabled() ? 1.0f : 0.34f;
        const auto pointAt = [&centre] (float r, float a)
        {
            return juce::Point<float> (centre.x + std::sin (a) * r,
                                       centre.y - std::cos (a) * r);
        };

        juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.44f),
                                     centre.x, centre.y + radius * 0.55f,
                                     juce::Colours::transparentBlack,
                                     centre.x, centre.y - radius, true);
        g.setGradientFill (shadow);
        g.fillEllipse (centre.x - radius - 2.0f, centre.y - radius + 3.0f,
                       radius * 2.0f + 4.0f, radius * 2.0f + 4.0f);

        juce::ColourGradient face (raisedColour().brighter (0.10f),
                                   centre.x - radius * 0.45f,
                                   centre.y - radius * 0.55f,
                                   surfaceColour().darker (0.18f),
                                   centre.x + radius * 0.55f,
                                   centre.y + radius * 0.70f, false);
        g.setGradientFill (face);
        g.fillEllipse (centre.x - radius, centre.y - radius, diameter, diameter);

        g.setColour (borderColour().withAlpha (0.62f * enabledAlpha));
        g.drawEllipse (centre.x - radius, centre.y - radius, diameter, diameter, 1.1f);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc (centre.x, centre.y,
                                      radius * 0.82f, radius * 0.82f, 0.0f,
                                      rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (borderColour().withAlpha (0.48f * enabledAlpha));
        g.strokePath (backgroundArc,
                      juce::PathStrokeType (3.4f,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y,
                                radius * 0.82f, radius * 0.82f, 0.0f,
                                rotaryStartAngle, angle, true);
        g.setColour (strandAColour().withAlpha (0.96f * enabledAlpha));
        g.strokePath (valueArc,
                      juce::PathStrokeType (3.5f,
                                            juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

        for (int i = 0; i <= 10; ++i)
        {
            const float t = (float) i / 10.0f;
            const float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const auto inner = pointAt (radius * 0.67f, tickAngle);
            const auto outer = pointAt (radius * 0.73f, tickAngle);
            g.setColour (mutedTextColour().withAlpha ((i == 0 || i == 10 ? 0.38f : 0.20f)
                                                      * enabledAlpha));
            g.drawLine ({ inner, outer }, 1.0f);
        }

        const auto pointerEnd = pointAt (radius * 0.58f, angle);
        g.setColour (strandBColour().withAlpha (0.95f * enabledAlpha));
        g.drawLine ({ centre, pointerEnd }, juce::jmax (2.0f, radius * 0.055f));
        g.fillEllipse (juce::Rectangle<float> (radius * 0.14f, radius * 0.14f)
                           .withCentre (centre));
    }

    void DnaLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                               juce::Button& button,
                                               const juce::Colour& background,
                                               bool highlighted, bool down)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool active = button.getToggleState();
        auto fill = active ? strandAColour().withAlpha (0.24f)
                           : (background.isTransparent() ? raisedColour() : background);

        if (highlighted)
            fill = fill.brighter (0.10f);
        if (down)
            fill = fill.darker (0.12f);
        if (! button.isEnabled())
            fill = fill.withMultipliedAlpha (0.42f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, juce::jmin (9.0f, bounds.getHeight() * 0.30f));

        const auto outline = active ? strandAColour().withAlpha (0.72f)
                                    : borderColour().withAlpha (highlighted ? 0.78f : 0.50f);
        g.setColour (outline.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.38f));
        g.drawRoundedRectangle (bounds, juce::jmin (9.0f, bounds.getHeight() * 0.30f), 1.0f);
    }

    void DnaLookAndFeel::drawToggleButton (juce::Graphics& g,
                                           juce::ToggleButton& button,
                                           bool highlighted, bool down)
    {
        auto area = button.getLocalBounds().toFloat();
        const bool on = button.getToggleState();
        const float alpha = button.isEnabled() ? 1.0f : 0.34f;
        const bool warningStyle = button.getName().containsIgnoreCase ("NULL")
                               || button.getName().containsIgnoreCase (jp ("バイパス"));
        const auto activeColour = warningStyle ? warningColour() : strandBColour();

        const float switchWidth = juce::jlimit (30.0f, 38.0f, area.getHeight() * 1.58f);
        auto switchArea = area.removeFromLeft (switchWidth).reduced (1.0f, 4.0f);
        auto trackColour = on ? activeColour.withAlpha (0.72f * alpha)
                              : borderColour().withAlpha (0.52f * alpha);
        if (highlighted)
            trackColour = trackColour.brighter (0.10f);
        if (down)
            trackColour = trackColour.darker (0.12f);

        g.setColour (trackColour);
        g.fillRoundedRectangle (switchArea, switchArea.getHeight() * 0.5f);

        const float thumbSize = switchArea.getHeight() - 4.0f;
        const float thumbX = on ? switchArea.getRight() - thumbSize - 2.0f
                                : switchArea.getX() + 2.0f;
        g.setColour ((on ? juce::Colours::white : mutedTextColour())
                         .withAlpha (0.95f * alpha));
        g.fillEllipse (thumbX, switchArea.getY() + 2.0f, thumbSize, thumbSize);

        g.setColour ((on ? textColour() : mutedTextColour()).withAlpha (alpha));
        g.setFont (uiFont (juce::jlimit (10.5f, 13.0f, area.getHeight() * 0.42f), on));
        g.drawFittedText (button.getButtonText(), area.toNearestInt().reduced (5, 0),
                          juce::Justification::centredLeft, 1);
    }

    void DnaLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                       bool isButtonDown, int buttonX, int buttonY,
                                       int buttonW, int buttonH, juce::ComboBox& box)
    {
        auto bounds = juce::Rectangle<float> (0.5f, 0.5f,
                                               (float) width - 1.0f,
                                               (float) height - 1.0f);
        auto fill = raisedColour();
        if (box.isMouseOverOrDragging())
            fill = fill.brighter (0.07f);
        if (isButtonDown)
            fill = fill.darker (0.10f);
        if (! box.isEnabled())
            fill = fill.withMultipliedAlpha (0.45f);

        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 7.0f);
        g.setColour (borderColour().withAlpha (box.isEnabled() ? 0.68f : 0.30f));
        g.drawRoundedRectangle (bounds, 7.0f, 1.0f);

        juce::Path arrow;
        const float cx = (float) buttonX + (float) buttonW * 0.5f;
        const float cy = (float) buttonY + (float) buttonH * 0.50f;
        arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
        arrow.lineTo (cx, cy + 2.0f);
        arrow.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (mutedTextColour().withAlpha (box.isEnabled() ? 0.90f : 0.35f));
        g.strokePath (arrow, juce::PathStrokeType (1.8f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    void DnaLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (9, 1, box.getWidth() - 32, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void DnaLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
    {
        g.fillAll (raisedColour());
        g.setColour (borderColour().withAlpha (0.75f));
        g.drawRect (0, 0, width, height, 1);
    }
}
