#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/OrbitDisplay.h"
#include "ui/DnaLookAndFeel.h"

class DNAOrbitAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Button::Listener
{
public:
    explicit DNAOrbitAudioProcessorEditor (DNAOrbitAudioProcessor&);
    ~DNAOrbitAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment  = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment   = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    DNAOrbitAudioProcessor& processorRef;

    dnaorbit::ui::DnaLookAndFeel lookAndFeel;
    dnaorbit::ui::OrbitDisplay orbitDisplay;

    Knob rateKnob, radiusKnob, depthKnob, symmetryKnob, twistKnob, coreKnob, mixKnob, outputKnob;

    juce::ToggleButton syncButton { "Sync" };
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    std::unique_ptr<ButtonAttachment> syncAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment;

    juce::ToggleButton nullCoreButton { "Null Core" };
    std::unique_ptr<ButtonAttachment> nullCoreAttachment;
    juce::Label nullCoreWarningLabel;

    juce::Label titleLabel;

    void setUpKnob (Knob& knob, const juce::String& paramID, const juce::String& text);
    void buttonClicked (juce::Button*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessorEditor)
};
