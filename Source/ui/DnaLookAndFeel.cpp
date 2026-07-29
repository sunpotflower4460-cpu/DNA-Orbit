#include "DnaLookAndFeel.h"

namespace dnaorbit::ui
{
    DnaLookAndFeel::DnaLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, backgroundColour());
        setColour (juce::Slider::thumbColourId, strandAColour());
        setColour (juce::Slider::rotarySliderFillColourId, strandAColour());
        setColour (juce::Slider::rotarySliderOutlineColourId, panelColour());
        setColour (juce::Slider::textBoxTextColourId, textColour());
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, textColour());
        setColour (juce::ToggleButton::textColourId, textColour());
        setColour (juce::ToggleButton::tickColourId, strandBColour());
        setColour (juce::ComboBox::backgroundColourId, panelColour());
        setColour (juce::ComboBox::textColourId, textColour());
    }

    void DnaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                                           juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        g.setColour (panelColour());
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        juce::Path arcBack;
        arcBack.addCentredArc (centre.x, centre.y, radius * 0.86f, radius * 0.86f, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (arcBack, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path arcValue;
        arcValue.addCentredArc (centre.x, centre.y, radius * 0.86f, radius * 0.86f, 0.0f,
                                 rotaryStartAngle, angle, true);
        g.setColour (slider.isEnabled() ? strandAColour() : strandAColour().withAlpha (0.4f));
        g.strokePath (arcValue, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path pointer;
        const float pointerLength = radius * 0.62f;
        const float pointerThickness = 2.4f;
        pointer.addRectangle (-pointerThickness * 0.5f, -radius * 0.72f, pointerThickness, pointerLength);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

        g.setColour (strandBColour());
        g.fillPath (pointer);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.0f);
    }
}
