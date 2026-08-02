#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"

using namespace dnaorbit;
using dnaorbit::ui::jp;

juce::ValueTree DNAOrbitAudioProcessorEditor::uiStateTree() const
{
    return processorRef.apvts.state.getOrCreateChildWithName (params::uiStateNodeID, nullptr);
}

juce::Font DNAOrbitAudioProcessorEditor::japaneseFont (float height, bool bold)
{
    return ui::DnaLookAndFeel::uiFont (height, bold);
}

DNAOrbitAudioProcessorEditor::DNAOrbitAudioProcessorEditor (DNAOrbitAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), helixView (p.getEngine())
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("DNA ORBIT", juce::dontSendNotification);
    titleLabel.setFont (japaneseFont (21.0f, true));
    titleLabel.setColour (juce::Label::textColourId, ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText (jp("中心を保ちながら、二本の音像が軌道を描きます"),
                           juce::dontSendNotification);
    subtitleLabel.setFont (japaneseFont (11.0f));
    subtitleLabel.setColour (juce::Label::textColourId,
                             ui::DnaLookAndFeel::textColour().withAlpha (0.6f));
    addAndMakeVisible (subtitleLabel);
    addAndMakeVisible (helixView);

    for (auto* button : { &basicTabButton, &detailTabButton })
    {
        button->setClickingTogglesState (false);
        button->setColour (juce::TextButton::buttonColourId, ui::DnaLookAndFeel::panelColour());
        button->setColour (juce::TextButton::textColourOffId, ui::DnaLookAndFeel::textColour());
        addAndMakeVisible (*button);
    }
    basicTabButton.setButtonText (jp("基本"));
    detailTabButton.setButtonText (jp("詳細"));
    basicTabButton.onClick = [this] { showPage (0); };
    detailTabButton.onClick = [this] { showPage (1); };

    presetLabel.setText (jp("プリセット"), juce::dontSendNotification);
    presetLabel.setFont (japaneseFont (11.0f));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    presetLabel.setColour (juce::Label::textColourId, ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (presetLabel);

    presetBox.setTextWhenNothingSelected (jp("選んでください"));
    for (int i = 0; i < presets::numPresets; ++i)
        presetBox.addItem (jp (presets::presets[i].name), i + 1);
    presetBox.setTooltip (jp("用途に近い音から始め、必要な部分だけ微調整します。"));
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index < presets::numPresets)
            applyPreset (index);
    };

    revertButton.setButtonText (jp("元に戻す"));
    revertButton.setTooltip (jp("選択したプリセットの全設定へ戻します。"));
    revertButton.onClick = [this]
    {
        if (currentPresetIndex >= 0 && currentPresetIndex < presets::numPresets)
            applyPreset (currentPresetIndex);
    };
    addAndMakeVisible (revertButton);
    revertButton.setVisible (false);

    setUpKnob (rateKnob, params::rateID, jp("速さ"), jp("1周する時間"),
               jp("自由走行時の回転速度。テンポ同期中はホストのBPMと分割が優先されます。"));
    setUpKnob (radiusKnob, params::radiusID, jp("広がり"), jp("左右の幅"),
               jp("二本の音像が左右へ離れる距離です。"));
    setUpKnob (depthKnob, params::depthID, jp("立体感"), jp("前後の奥行き"),
               jp("ゲイン・高域・微小時間差による前後感の強さです。"));
    setUpKnob (mixKnob, params::mixID, jp("効果量"), jp("原音とのブレンド"),
               jp("相関対応のDry/Wetブレンド。音量自動補正ON時は中間点の膨らみも抑えます。"));

    setUpKnob (symmetryKnob, params::symmetryID, jp("対称性"), jp("中心の固定度"),
               jp("100%で二本は正反対を保ちます。下げるとBがわずかに漂います。"));
    setUpKnob (twistKnob, params::twistID, jp("ねじれ"), jp("二本の時間差"),
               jp("Bへ追加する微小ディレイです。"));
    setUpKnob (coreKnob, params::coreID, jp("中心の芯"), jp("中央に残す音"),
               jp("動く高域DNAの中央へMid成分を残します。NULL CORE中は無効です。"));
    setUpKnob (outputKnob, params::outputID, jp("出力"), jp("最終音量"),
               jp("処理後の最終出力トリムです。"));
    setUpKnob (stereoPreserveKnob, params::stereoPreserveID, jp("ステレオ保持"), jp("元の横幅"),
               jp("クロスオーバーより上のSide成分を、安定した横幅として戻します。"));
    setUpKnob (bassAnchorKnob, params::bassAnchorID, jp("低域固定"), jp("中央に残す帯域"),
               jp("指定周波数より下を中央へ固定します。20Hzは実質OFFです。"));
    setUpKnob (startPhaseKnob, params::startPhaseID, jp("開始位置"), jp("軌道の初期角度"),
               jp("RetriggerとHost Lockの軌道開始角度です。"));

    syncButton.setButtonText (jp("テンポ同期"));
    syncButton.setTooltip (jp("BPM・拍子・分割に合わせて回転周期を決めます。"));
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                          params::syncID, syncButton);

    setUpChoice (divisionBox, divisionLabel, params::syncDivisionChoices,
                 jp("分割"), jp("一周に使う小節または音価です。"),
                 divisionAttachment, params::divisionID);
    setUpChoice (characterBox, characterLabel,
                 { "Natural", "Vivid", "Deep" },
                 jp("質感"), jp("前後の暗さ・減衰・時間差の性格です。"),
                 characterAttachment, params::characterID);
    setUpChoice (phaseModeBox, phaseModeLabel,
                 { "Free", "Retrigger", "Host Lock" },
                 jp("位相"), jp("Host Lockは曲位置から軌道を再現します。Retriggerは再生開始で戻ります。"),
                 phaseModeAttachment, params::phaseModeID);
    setUpChoice (directionBox, directionLabel,
                 { "CW", "CCW" },
                 jp("方向"), jp("回転方向を反転します。"),
                 directionAttachment, params::directionID);

    autoGainButton.setButtonText (jp("音量自動補正"));
    autoGainButton.setTooltip (jp("Wet音量とDry/Wet相関を補正し、公平に比較しやすくします。"));
    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                              params::autoGainID,
                                                              autoGainButton);

    softBypassButton.setButtonText (jp("ソフトバイパス"));
    softBypassButton.setTooltip (jp("内部状態を動かしたまま、60msで正確な原音へ戻します。"));
    addAndMakeVisible (softBypassButton);
    softBypassAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                                params::softBypassID,
                                                                softBypassButton);

    nullCoreButton.setButtonText (jp("NULL CORE (実験的)"));
    nullCoreButton.setColour (juce::ToggleButton::textColourId,
                              ui::DnaLookAndFeel::warningColour());
    nullCoreButton.setColour (juce::ToggleButton::tickColourId,
                              ui::DnaLookAndFeel::warningColour());
    nullCoreButton.setTooltip (jp("WetのMidを除去します。モノラルでは消える可能性があります。"));
    addAndMakeVisible (nullCoreButton);
    nullCoreAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                              params::nullCoreID,
                                                              nullCoreButton);

    readoutLabel.setFont (japaneseFont (11.5f));
    readoutLabel.setColour (juce::Label::textColourId,
                            ui::DnaLookAndFeel::textColour().withAlpha (0.85f));
    readoutLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (readoutLabel);

    statusLabel.setFont (japaneseFont (13.0f, true));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    warningLabel.setText (jp("NULL CORE — モノラルで消える可能性があります"),
                          juce::dontSendNotification);
    warningLabel.setFont (japaneseFont (12.0f, true));
    warningLabel.setColour (juce::Label::textColourId, ui::DnaLookAndFeel::warningColour());
    warningLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (warningLabel);
    warningLabel.setVisible (false);

    if (auto* nullCoreParam = processorRef.apvts.getParameter (params::nullCoreID))
    {
        nullCoreWatcher = std::make_unique<juce::ParameterAttachment> (
            *nullCoreParam,
            [this] (float value)
            {
                const bool on = value > 0.5f;
                warningLabel.setVisible (on);
                coreKnob.slider.setEnabled (! on);
                coreKnob.nameLabel.setEnabled (! on);
                coreKnob.hintLabel.setEnabled (! on);
            },
            nullptr);
        nullCoreWatcher->sendInitialUpdate();
    }

    showPage ((int) uiStateTree().getProperty (params::editorPagePropertyID, 0));
    setResizable (true, true);
    setResizeLimits (780, 540, 1600, 1100);

    const int savedWidth = (int) uiStateTree().getProperty (params::editorWidthPropertyID, 900);
    const int savedHeight = (int) uiStateTree().getProperty (params::editorHeightPropertyID, 680);
    setSize (juce::jlimit (780, 1600, savedWidth),
             juce::jlimit (540, 1100, savedHeight));
    startTimerHz (12);
}

DNAOrbitAudioProcessorEditor::~DNAOrbitAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DNAOrbitAudioProcessorEditor::setUpKnob (Knob& knob, const juce::String& paramID,
                                              const juce::String& name,
                                              const juce::String& hint,
                                              const juce::String& tooltip)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 17);
    knob.slider.setTooltip (tooltip);
    knob.slider.setWantsKeyboardFocus (true);
    addAndMakeVisible (knob.slider);

    knob.nameLabel.setText (name, juce::dontSendNotification);
    knob.nameLabel.setFont (japaneseFont (13.0f, true));
    knob.nameLabel.setJustificationType (juce::Justification::centred);
    knob.nameLabel.setColour (juce::Label::textColourId, ui::DnaLookAndFeel::textColour());
    knob.nameLabel.setTooltip (tooltip);
    addAndMakeVisible (knob.nameLabel);

    knob.hintLabel.setText (hint, juce::dontSendNotification);
    knob.hintLabel.setFont (japaneseFont (9.5f));
    knob.hintLabel.setJustificationType (juce::Justification::centred);
    knob.hintLabel.setColour (juce::Label::textColourId,
                              ui::DnaLookAndFeel::textColour().withAlpha (0.55f));
    addAndMakeVisible (knob.hintLabel);

    knob.attachment = std::make_unique<SliderAttachment> (processorRef.apvts,
                                                          paramID, knob.slider);
}

void DNAOrbitAudioProcessorEditor::setUpChoice (
    juce::ComboBox& box, juce::Label& label, const juce::StringArray& items,
    const juce::String& labelText, const juce::String& tooltip,
    std::unique_ptr<ComboAttachment>& attachment, const juce::String& paramID)
{
    label.setText (labelText, juce::dontSendNotification);
    label.setFont (japaneseFont (10.5f));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId, ui::DnaLookAndFeel::textColour());
    label.setTooltip (tooltip);
    addAndMakeVisible (label);

    box.addItemList (items, 1);
    box.setTooltip (tooltip);
    box.setWantsKeyboardFocus (true);
    addAndMakeVisible (box);
    attachment = std::make_unique<ComboAttachment> (processorRef.apvts, paramID, box);
}

void DNAOrbitAudioProcessorEditor::applyPreset (int presetIndex)
{
    presets::apply (processorRef.apvts, presets::presets[presetIndex]);
    currentPresetIndex = presetIndex;
}

void DNAOrbitAudioProcessorEditor::showPage (int page)
{
    currentPage = juce::jlimit (0, 1, page);
    uiStateTree().setProperty (params::editorPagePropertyID, currentPage, nullptr);
    const bool basic = currentPage == 0;

    for (auto* knob : { &rateKnob, &radiusKnob, &depthKnob, &mixKnob })
    {
        knob->slider.setVisible (basic);
        knob->nameLabel.setVisible (basic);
        knob->hintLabel.setVisible (basic);
    }

    for (auto* knob : { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob,
                        &stereoPreserveKnob, &bassAnchorKnob, &startPhaseKnob })
    {
        knob->slider.setVisible (! basic);
        knob->nameLabel.setVisible (! basic);
        knob->hintLabel.setVisible (! basic);
    }

    for (auto* component : { static_cast<juce::Component*> (&syncButton),
                             &divisionBox, &divisionLabel,
                             &characterBox, &characterLabel,
                             &phaseModeBox, &phaseModeLabel,
                             &directionBox, &directionLabel,
                             &autoGainButton, &softBypassButton, &nullCoreButton })
        component->setVisible (! basic);

    presetBox.setVisible (basic);
    presetLabel.setVisible (basic);
    if (! basic)
        revertButton.setVisible (false);

    const auto active = ui::DnaLookAndFeel::strandAColour().withAlpha (0.35f);
    const auto idle = ui::DnaLookAndFeel::panelColour();
    basicTabButton.setColour (juce::TextButton::buttonColourId, basic ? active : idle);
    detailTabButton.setColour (juce::TextButton::buttonColourId, basic ? idle : active);
    resized();
    repaint();
}

void DNAOrbitAudioProcessorEditor::timerCallback()
{
    const bool axisLocked = helixView.isAxisLocked();
    const auto visual = processorRef.getEngine().getVisualState();
    const bool syncOn = processorRef.apvts.getRawParameterValue (params::syncID)->load() > 0.5f;

    divisionBox.setEnabled (syncOn);
    phaseModeBox.setEnabled (syncOn);
    startPhaseKnob.slider.setEnabled (syncOn);
    startPhaseKnob.nameLabel.setEnabled (syncOn);
    startPhaseKnob.hintLabel.setEnabled (syncOn);

    juce::String status = axisLocked ? jp("中心軸 固定") : jp("中心軸 ゆらぎ");
    if (visual.hostPhaseLocked)
        status << "  •  HOST LOCK";
    statusLabel.setText (status, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           axisLocked ? ui::DnaLookAndFeel::centreLockedColour()
                                      : ui::DnaLookAndFeel::centreDriftColour());

    const auto signed3 = [] (float value)
    {
        return juce::String (value >= 0.0f ? "+" : "-")
             + juce::String (std::abs (value), 3);
    };

    juce::String text;
    text << "A  " << signed3 (helixView.getPanA()) << "\n"
         << "B  " << signed3 (helixView.getPanB()) << "\n"
         << jp("中心 ") << juce::String (helixView.getCentroidX(), 4) << "\n"
         << jp("相関 ") << juce::String (helixView.getCorrelation(), 2) << "\n"
         << jp("低域固定 ")
         << (visual.bassAnchorHz <= 20.1f ? juce::String ("OFF")
                                          : juce::String (visual.bassAnchorHz, 0) + " Hz");
    readoutLabel.setText (text, juce::dontSendNotification);

    if (currentPage == 0 && currentPresetIndex >= 0 && currentPresetIndex < presets::numPresets)
        revertButton.setVisible (! presets::matchesCurrentState (
            processorRef.apvts, presets::presets[currentPresetIndex]));
    else
        revertButton.setVisible (false);
}

void DNAOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::DnaLookAndFeel::backgroundColour());
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (44);
    const auto controlArea = area.removeFromBottom (controlPanelHeight);
    g.setColour (ui::DnaLookAndFeel::panelColour().withAlpha (0.6f));
    g.fillRoundedRectangle (controlArea.toFloat(), 6.0f);
}

void DNAOrbitAudioProcessorEditor::layOutKnobRow (
    juce::Rectangle<int> row, const std::vector<Knob*>& knobs)
{
    if (knobs.empty())
        return;

    const int slotWidth = row.getWidth() / (int) knobs.size();
    for (auto* knob : knobs)
    {
        auto slot = row.removeFromLeft (slotWidth);
        knob->nameLabel.setBounds (slot.removeFromTop (18));
        knob->hintLabel.setBounds (slot.removeFromBottom (14));
        knob->slider.setBounds (slot.reduced (2, 0));
    }
}

void DNAOrbitAudioProcessorEditor::resized()
{
    uiStateTree().setProperty (params::editorWidthPropertyID, getWidth(), nullptr);
    uiStateTree().setProperty (params::editorHeightPropertyID, getHeight(), nullptr);

    auto area = getLocalBounds().reduced (12);
    auto topBar = area.removeFromTop (44);
    auto titleArea = topBar.removeFromLeft (250);
    titleLabel.setBounds (titleArea.removeFromTop (24));
    subtitleLabel.setBounds (titleArea);

    auto tabArea = topBar.removeFromLeft (140).reduced (0, 8);
    basicTabButton.setBounds (tabArea.removeFromLeft (66));
    tabArea.removeFromLeft (4);
    detailTabButton.setBounds (tabArea.removeFromLeft (66));

    auto presetArea = topBar.removeFromRight (350).reduced (0, 9);
    revertButton.setBounds (presetArea.removeFromRight (58));
    presetArea.removeFromRight (6);
    presetLabel.setBounds (presetArea.removeFromLeft (74));
    presetArea.removeFromLeft (6);
    presetBox.setBounds (presetArea);

    auto controlArea = area.removeFromBottom (controlPanelHeight).reduced (8);

    if (currentPage == 0)
    {
        layOutKnobRow (controlArea, { &rateKnob, &radiusKnob, &depthKnob, &mixKnob });
    }
    else
    {
        auto configColumn = controlArea.removeFromRight (
            juce::jmin (280, controlArea.getWidth() / 3));
        configColumn.reduce (4, 2);

        auto choiceRow = [&configColumn] (juce::Label& label, juce::ComboBox& box)
        {
            auto row = configColumn.removeFromTop (22);
            label.setBounds (row.removeFromLeft (62));
            box.setBounds (row);
            configColumn.removeFromTop (2);
        };

        auto syncRow = configColumn.removeFromTop (22);
        syncButton.setBounds (syncRow.removeFromLeft (104));
        divisionLabel.setBounds (syncRow.removeFromLeft (42));
        divisionBox.setBounds (syncRow);
        configColumn.removeFromTop (2);

        choiceRow (phaseModeLabel, phaseModeBox);
        choiceRow (directionLabel, directionBox);
        choiceRow (characterLabel, characterBox);

        auto buttons = configColumn;
        const int buttonWidth = buttons.getWidth() / 2;
        auto row1 = buttons.removeFromTop (24);
        autoGainButton.setBounds (row1.removeFromLeft (buttonWidth));
        softBypassButton.setBounds (row1);
        buttons.removeFromTop (2);
        nullCoreButton.setBounds (buttons.removeFromTop (24));

        layOutKnobRow (controlArea,
                       { &symmetryKnob, &twistKnob, &coreKnob, &stereoPreserveKnob,
                         &bassAnchorKnob, &startPhaseKnob, &outputKnob });
    }

    auto centreArea = area;
    auto readoutColumn = centreArea.removeFromLeft (juce::jmin (150, centreArea.getWidth() / 5));
    statusLabel.setBounds (readoutColumn.removeFromTop (24));
    readoutColumn.removeFromTop (6);
    readoutLabel.setBounds (readoutColumn.removeFromTop (110));

    warningLabel.setBounds (centreArea.removeFromBottom (20));
    helixView.setBounds (centreArea.reduced (4));
}
