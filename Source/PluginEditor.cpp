#include "PluginEditor.h"
#include "Parameters.h"

DNAOrbitAudioProcessorEditor::DNAOrbitAudioProcessorEditor (DNAOrbitAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      orbitDisplay (p.getEngine())
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("DNA ORBIT", juce::dontSendNotification);
    titleLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (orbitDisplay);

    setUpKnob (rateKnob,     dnaorbit::params::rateID,     "Rate");
    setUpKnob (radiusKnob,   dnaorbit::params::radiusID,   "Radius");
    setUpKnob (depthKnob,    dnaorbit::params::depthID,    "Depth");
    setUpKnob (symmetryKnob, dnaorbit::params::symmetryID, "Symmetry");
    setUpKnob (twistKnob,    dnaorbit::params::twistID,    "Twist");
    setUpKnob (coreKnob,     dnaorbit::params::coreID,     "Core");
    setUpKnob (mixKnob,      dnaorbit::params::mixID,      "Mix");
    setUpKnob (outputKnob,   dnaorbit::params::outputID,   "Output");

    syncButton.setColour (juce::ToggleButton::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, dnaorbit::params::syncID, syncButton);

    divisionBox.addItemList (dnaorbit::params::syncDivisionChoices, 1);
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<ComboAttachment> (processorRef.apvts, dnaorbit::params::divisionID, divisionBox);

    divisionLabel.setText ("Division", juce::dontSendNotification);
    divisionLabel.setFont (juce::FontOptions (12.0f));
    divisionLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (divisionLabel);

    nullCoreButton.setColour (juce::ToggleButton::textColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    addAndMakeVisible (nullCoreButton);
    nullCoreAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, dnaorbit::params::nullCoreID, nullCoreButton);

    nullCoreWarningLabel.setText ("NULL CORE - MONO MAY DISAPPEAR", juce::dontSendNotification);
    nullCoreWarningLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    nullCoreWarningLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    nullCoreWarningLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (nullCoreWarningLabel);
    // addAndMakeVisible() unconditionally makes its child visible, so the
    // explicit setVisible() call reflecting the actual parameter state must
    // come after it, not before - otherwise it gets clobbered and the
    // warning shows even when Null Core defaults to off.
    nullCoreWarningLabel.setVisible (processorRef.apvts.getRawParameterValue (dnaorbit::params::nullCoreID)->load() > 0.5f);
    nullCoreButton.addListener (this);

    setResizable (true, true);
    setResizeLimits (640, 400, 1400, 1000);
    setSize (760, 480);
}

DNAOrbitAudioProcessorEditor::~DNAOrbitAudioProcessorEditor()
{
    nullCoreButton.removeListener (this);
    setLookAndFeel (nullptr);
}

void DNAOrbitAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &nullCoreButton)
        nullCoreWarningLabel.setVisible (nullCoreButton.getToggleState());
}

void DNAOrbitAudioProcessorEditor::setUpKnob (Knob& knob, const juce::String& paramID, const juce::String& text)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    addAndMakeVisible (knob.slider);

    knob.label.setText (text, juce::dontSendNotification);
    knob.label.setFont (juce::FontOptions (13.0f));
    knob.label.setJustificationType (juce::Justification::centred);
    knob.label.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (knob.label);

    knob.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, knob.slider);
}

void DNAOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (dnaorbit::ui::DnaLookAndFeel::backgroundColour());
}

void DNAOrbitAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (12);

    auto topBar = area.removeFromTop (34);
    titleLabel.setBounds (topBar.removeFromLeft (200));

    nullCoreButton.setBounds (topBar.removeFromRight (110));
    auto divisionArea = topBar.removeFromRight (100);
    divisionLabel.setBounds (divisionArea.removeFromTop (14));
    divisionBox.setBounds (divisionArea);
    syncButton.setBounds (topBar.removeFromRight (70));

    auto warningBar = area.removeFromTop (18);
    nullCoreWarningLabel.setBounds (warningBar);

    auto knobRow = area.removeFromBottom (110);

    const int numKnobs = 8;
    const int knobWidth = knobRow.getWidth() / numKnobs;

    Knob* knobs[numKnobs] = { &rateKnob, &radiusKnob, &depthKnob, &symmetryKnob,
                              &twistKnob, &coreKnob, &mixKnob, &outputKnob };

    for (int i = 0; i < numKnobs; ++i)
    {
        auto slot = knobRow.removeFromLeft (knobWidth);
        knobs[(size_t) i]->label.setBounds (slot.removeFromTop (16));
        knobs[(size_t) i]->slider.setBounds (slot.reduced (2));
    }

    orbitDisplay.setBounds (area.reduced (8));
}
