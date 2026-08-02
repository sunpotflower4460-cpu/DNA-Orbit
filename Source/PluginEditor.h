#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/HelixView3D.h"
#include "ui/DnaLookAndFeel.h"

class DNAOrbitAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit DNAOrbitAudioProcessorEditor (DNAOrbitAudioProcessor&);
    ~DNAOrbitAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    class PillLabel : public juce::Label
    {
    public:
        void setPillColours (juce::Colour fillColour,
                             juce::Colour outlineColour,
                             float cornerRadius = 8.0f)
        {
            fill = fillColour;
            outline = outlineColour;
            radius = cornerRadius;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            const auto bounds = getLocalBounds().toFloat();
            if (! fill.isTransparent())
            {
                g.setColour (fill);
                g.fillRoundedRectangle (bounds, radius);
            }
            if (! outline.isTransparent())
            {
                g.setColour (outline);
                g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);
            }
            juce::Label::paint (g);
        }

    private:
        juce::Colour fill { juce::Colours::transparentBlack };
        juce::Colour outline { juce::Colours::transparentBlack };
        float radius = 8.0f;
    };

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag,
                              juce::Slider::TextBoxBelow };
        juce::Label nameLabel;
        juce::Label hintLabel;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setUpKnob (Knob&, const juce::String& paramID, const juce::String& name,
                    const juce::String& hint, const juce::String& tooltip);
    void setUpChoice (juce::ComboBox&, juce::Label&, const juce::StringArray& items,
                      const juce::String& label, const juce::String& tooltip,
                      std::unique_ptr<ComboAttachment>& attachment,
                      const juce::String& paramID);
    void setKnobVisible (Knob&, bool);
    void setKnobEnabled (Knob&, bool);
    void layOutKnobRow (juce::Rectangle<int> row, const std::vector<Knob*>& knobs,
                        int minimumSlotWidth = 82);
    void layoutBasicControls (juce::Rectangle<int> area);
    void layoutDetailControls (juce::Rectangle<int> area);
    void showPage (int page);
    void applyPreset (int presetIndex);
    void timerCallback() override;

    bool usesCompactDetailLayout() const noexcept;
    int currentControlPanelHeight() const noexcept;
    juce::ValueTree uiStateTree() const;
    static juce::Font japaneseFont (float height, bool bold = false);

    DNAOrbitAudioProcessor& processorRef;
    dnaorbit::ui::DnaLookAndFeel lookAndFeel;
    dnaorbit::ui::HelixView3D helixView;
    juce::TooltipWindow tooltipWindow { this, 500 };

    juce::Label productTagLabel, titleLabel, subtitleLabel;
    juce::TextButton basicTabButton, detailTabButton;
    int currentPage = 0;

    juce::ComboBox presetBox;
    juce::Label presetLabel;
    PillLabel presetStateLabel;
    juce::TextButton revertButton;
    int currentPresetIndex = -1;

    Knob rateKnob, radiusKnob, depthKnob, mixKnob;
    Knob symmetryKnob, twistKnob, coreKnob, outputKnob, stereoPreserveKnob;
    Knob bassAnchorKnob, startPhaseKnob;

    juce::ToggleButton syncButton;
    juce::ComboBox divisionBox, characterBox, phaseModeBox, directionBox;
    juce::Label divisionLabel, characterLabel, phaseModeLabel, directionLabel;
    juce::ToggleButton autoGainButton;
    juce::ToggleButton softBypassButton;
    juce::ToggleButton nullCoreButton;

    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> softBypassAttachment;
    std::unique_ptr<ButtonAttachment> nullCoreAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment;
    std::unique_ptr<ComboAttachment> characterAttachment;
    std::unique_ptr<ComboAttachment> phaseModeAttachment;
    std::unique_ptr<ComboAttachment> directionAttachment;

    juce::Label guideLabel;
    PillLabel statusLabel, diagnosticLabel, warningLabel;
    std::unique_ptr<juce::ParameterAttachment> nullCoreWatcher;

    juce::Rectangle<int> visualCardBounds;
    juce::Rectangle<int> controlPanelBounds;
    juce::Rectangle<int> motionCardBounds;
    juce::Rectangle<int> spaceCardBounds;
    juce::Rectangle<int> outputCardBounds;

    static constexpr int minimumEditorWidth = 820;
    static constexpr int minimumEditorHeight = 650;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessorEditor)
};
