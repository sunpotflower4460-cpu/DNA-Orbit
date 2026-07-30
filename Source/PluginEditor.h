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

    /** A rotary control with a Japanese name and a one-line explanation beneath it. */
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

    /** Child ValueTree holding editor-only state (tab, window size); see Parameters.h. */
    juce::ValueTree uiStateTree() const;

    /** Prefers a real Japanese font: fontconfig's default for ja can be a Chinese face. */
    static juce::Font japaneseFont (float height, bool bold = false);

    DNAOrbitAudioProcessor& processorRef;

    dnaorbit::ui::DnaLookAndFeel lookAndFeel;
    dnaorbit::ui::HelixView3D helixView;
    juce::TooltipWindow tooltipWindow { this, 600 };

    juce::Label titleLabel, subtitleLabel;

    // Button text is set in the constructor via jp(), since juce::String's
    // const char* constructor would mangle these UTF-8 literals.
    juce::TextButton basicTabButton, detailTabButton;
    int currentPage = 0;

    juce::ComboBox presetBox;
    juce::Label presetLabel;
    juce::TextButton revertButton;

    // Always visible (both tabs), in the top bar: an in-plugin Soft Bypass,
    // independent of the host's own Bypass. See ADR-008.
    juce::ToggleButton bypassButton;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    // Tracks which factory preset (if any) is active, so the UI can show a
    // "Modified" state once the user nudges anything and offer Revert.
    // -1 means "no preset selected" (e.g. a project saved before this preset
    // was picked, or one loaded from a DAW project rather than chosen here).
    int currentPresetIndex = -1;

    // Basic page.
    Knob rateKnob, radiusKnob, depthKnob, mixKnob;

    // Detail page.
    Knob symmetryKnob, twistKnob, coreKnob, outputKnob, stereoPreserveKnob, bassAnchorKnob;
    juce::ToggleButton syncButton;
    juce::ComboBox divisionBox;
    juce::Label divisionLabel;
    juce::ToggleButton autoGainButton;
    juce::ToggleButton nullCoreButton;
    juce::ComboBox characterBox;
    juce::Label characterLabel;
    // Start Phase itself has no dedicated knob yet (deferred to Phase 5's UI
    // pass); it remains fully controllable via the host's generic parameter
    // list / automation in the meantime.
    juce::ComboBox phaseModeBox, directionBox;
    juce::Label phaseModeLabel;

    std::unique_ptr<ButtonAttachment> syncAttachment, autoGainAttachment, nullCoreAttachment;
    std::unique_ptr<ComboAttachment> divisionAttachment, characterAttachment, phaseModeAttachment, directionAttachment;

    // Live readouts, updated on a slow timer so 45 fps helix repaints never
    // trigger glyph re-layout.
    juce::Label readoutLabel, statusLabel, warningLabel;

    // Tracks the nullCore parameter itself, so the warning follows host
    // automation and preset loads rather than only UI clicks.
    std::unique_ptr<juce::ParameterAttachment> nullCoreWatcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessorEditor)
};
