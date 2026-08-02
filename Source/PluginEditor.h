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
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label nameLabel;
        juce::Label hintLabel;
        std::unique_ptr<SliderAttachment> attachment;
    };

    void setUpKnob (Knob&, const juce::String& paramID, const juce::String& name,
                    const juce::String& hint, const juce::String& tooltip);
    void layOutKnobRow (juce::Rectangle<int> row, const std::vector<Knob*>& knobs);
    void showPage (int page);
    void applyPreset (int presetIndex);
    void timerCallback() override;

    juce::ValueTree uiStateTree() const;
    static juce::Font japaneseFont (float height, bool bold = false);

    DNAOrbitAudioProcessor& processorRef;

    dnaorbit::ui::DnaLookAndFeel lookAndFeel;
    dnaorbit::ui::HelixView3D helixView;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Label titleLabel, subtitleLabel;
    juce::TextButton basicTabButton, detailTabButton;
    int currentPage = 0;

    juce::ComboBox presetBox;
    juce::Label presetLabel;
    juce::TextButton revertButton;
    int currentPresetIndex = -1;

    Knob rateKnob, radiusKnob, depthKnob, mixKnob;
    Knob symmetryKnob, twistKnob, coreKnob, outputKnob, stereoPreserveKnob;

    juce::ToggleButton syncButton;
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    juce::ToggleButton autoGainButton;
    juce::ToggleButton softBypassButton;
    juce::ToggleButton nullCoreButton;

    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ButtonAttachment> autoGainAttachment;
    std::unique_ptr<ButtonAttachment> softBypassAttachment;
    std::unique_ptr<ButtonAttachment> nullCoreAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment;

    juce::Label readoutLabel, statusLabel, warningLabel;
    std::unique_ptr<juce::ParameterAttachment> nullCoreWatcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessorEditor)
};
