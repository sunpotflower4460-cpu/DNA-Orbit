#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"

#include <cmath>

using namespace dnaorbit;
using dnaorbit::ui::jp;

namespace
{
    void configureOverlayLabel (juce::Label& label, float fontSize, bool bold,
                                juce::Justification justification)
    {
        label.setFont (dnaorbit::ui::DnaLookAndFeel::uiFont (fontSize, bold));
        label.setJustificationType (justification);
        label.setColour (juce::Label::textColourId,
                         dnaorbit::ui::DnaLookAndFeel::textColour());
        label.setInterceptsMouseClicks (false, false);
    }

    void drawCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                   juce::Colour fill, juce::Colour outline, float radius = 10.0f)
    {
        if (bounds.isEmpty())
            return;

        auto area = bounds.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        g.fillRoundedRectangle (area.translated (0.0f, 2.0f), radius);
        g.setColour (fill);
        g.fillRoundedRectangle (area, radius);
        g.setColour (outline);
        g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
    }
}

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
    setOpaque (true);
    setWantsKeyboardFocus (true);

    productTagLabel.setText ("SPATIAL MOTION", juce::dontSendNotification);
    productTagLabel.setFont (japaneseFont (9.5f, true));
    productTagLabel.setColour (juce::Label::textColourId,
                               ui::DnaLookAndFeel::strandAColour().withAlpha (0.78f));
    productTagLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (productTagLabel);

    titleLabel.setText ("DNA ORBIT", juce::dontSendNotification);
    titleLabel.setFont (japaneseFont (23.0f, true));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText (jp("中心を保ちながら、音に立体的な軌道を与えます"),
                           juce::dontSendNotification);
    subtitleLabel.setFont (japaneseFont (10.5f));
    subtitleLabel.setColour (juce::Label::textColourId,
                             ui::DnaLookAndFeel::mutedTextColour());
    subtitleLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (subtitleLabel);

    for (auto* button : { &basicTabButton, &detailTabButton })
    {
        button->setClickingTogglesState (false);
        button->setWantsKeyboardFocus (true);
        button->setColour (juce::TextButton::buttonColourId,
                           ui::DnaLookAndFeel::raisedColour());
        button->setColour (juce::TextButton::textColourOffId,
                           ui::DnaLookAndFeel::mutedTextColour());
        button->setColour (juce::TextButton::textColourOnId,
                           juce::Colours::white);
        addAndMakeVisible (*button);
    }
    basicTabButton.setButtonText (jp("基本"));
    basicTabButton.setName (jp("基本タブ"));
    basicTabButton.setTooltip (jp("曲作りで最も使う4つの操作だけを表示します。"));
    detailTabButton.setButtonText (jp("詳細"));
    detailTabButton.setName (jp("詳細タブ"));
    detailTabButton.setTooltip (jp("動き・空間・出力を細かく調整します。"));
    basicTabButton.onClick = [this] { showPage (0); };
    detailTabButton.onClick = [this] { showPage (1); };

    presetLabel.setText (jp("プリセット"), juce::dontSendNotification);
    presetLabel.setFont (japaneseFont (10.5f, true));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    presetLabel.setColour (juce::Label::textColourId,
                           ui::DnaLookAndFeel::mutedTextColour());
    presetLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetLabel);

    presetBox.setName (jp("プリセット"));
    presetBox.setWantsKeyboardFocus (true);
    presetBox.setTextWhenNothingSelected (jp("音の方向を選ぶ"));
    for (int i = 0; i < presets::numPresets; ++i)
        presetBox.addItem (jp (presets::presets[i].name), i + 1);
    presetBox.setTooltip (jp("用途に近い音から始め、主要ノブで仕上げます。"));
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index < presets::numPresets)
            applyPreset (index);
    };

    presetStateLabel.setText (jp("変更あり"), juce::dontSendNotification);
    presetStateLabel.setFont (japaneseFont (9.5f, true));
    presetStateLabel.setJustificationType (juce::Justification::centred);
    presetStateLabel.setColour (juce::Label::textColourId,
                                ui::DnaLookAndFeel::centreDriftColour());
    presetStateLabel.setPillColours (
        ui::DnaLookAndFeel::centreDriftColour().withAlpha (0.12f),
        ui::DnaLookAndFeel::centreDriftColour().withAlpha (0.42f), 7.0f);
    presetStateLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (presetStateLabel);
    presetStateLabel.setVisible (false);

    revertButton.setButtonText (jp("戻す"));
    revertButton.setName (jp("プリセットへ戻す"));
    revertButton.setTooltip (jp("選択中プリセットの全設定へ戻します。"));
    revertButton.onClick = [this]
    {
        if (currentPresetIndex >= 0 && currentPresetIndex < presets::numPresets)
            applyPreset (currentPresetIndex);
    };
    addAndMakeVisible (revertButton);
    revertButton.setVisible (false);

    setUpKnob (rateKnob, params::rateID, jp("速さ"), jp("軌道の周期"),
               jp("自由走行時の回転速度です。テンポ同期中はBPMと分割が優先されます。"));
    setUpKnob (radiusKnob, params::radiusID, jp("広がり"), jp("左右の幅"),
               jp("二本の音像が左右へ離れる距離です。"));
    setUpKnob (depthKnob, params::depthID, jp("立体感"), jp("前後の奥行き"),
               jp("ゲイン・高域・微小時間差による前後感です。"));
    setUpKnob (mixKnob, params::mixID, jp("効果量"), jp("原音との比率"),
               jp("原音と処理音を混ぜます。音量自動補正ONでは中間点の膨らみも抑えます。"));

    setUpKnob (symmetryKnob, params::symmetryID, jp("対称性"), jp("中心の安定"),
               jp("100%で二本は正反対を保ちます。下げるとBがゆっくり漂います。"));
    setUpKnob (twistKnob, params::twistID, jp("ねじれ"), jp("二本の時間差"),
               jp("Bへ追加する微小ディレイです。上げすぎると位相感が強くなります。"));
    setUpKnob (coreKnob, params::coreID, jp("中心の芯"), jp("中央に残す音"),
               jp("動く高域DNAの中央へMid成分を残します。NULL CORE中は無効です。"));
    setUpKnob (outputKnob, params::outputID, jp("出力"), jp("最終レベル"),
               jp("処理後の最終出力トリムです。"));
    setUpKnob (stereoPreserveKnob, params::stereoPreserveID,
               jp("ステレオ保持"), jp("元の横幅"),
               jp("クロスオーバーより上のSide成分を安定した横幅として戻します。"));
    setUpKnob (bassAnchorKnob, params::bassAnchorID,
               jp("低域固定"), jp("中央に残す帯域"),
               jp("指定周波数より下を中央へ固定します。20Hzは完全OFFです。"));
    setUpKnob (startPhaseKnob, params::startPhaseID,
               jp("開始位置"), jp("軌道の初期角度"),
               jp("RetriggerとHost Lockで使う軌道開始角度です。"));

    syncButton.setButtonText (jp("テンポ同期"));
    syncButton.setName (jp("テンポ同期"));
    syncButton.setTooltip (jp("ホストのBPM・拍子・分割に合わせて一周の周期を決めます。"));
    syncButton.setWantsKeyboardFocus (true);
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                          params::syncID, syncButton);

    setUpChoice (divisionBox, divisionLabel, params::syncDivisionChoices,
                 jp("分割"), jp("一周に使う小節または音価です。"),
                 divisionAttachment, params::divisionID);
    setUpChoice (characterBox, characterLabel, params::characterChoices,
                 jp("質感"), jp("前後の暗さ・減衰・時間差の性格を選びます。"),
                 characterAttachment, params::characterID);
    setUpChoice (phaseModeBox, phaseModeLabel, params::phaseModeChoices,
                 jp("再現方法"),
                 jp("Host Lockは曲位置に固定、Retriggerは再生開始で戻り、Freeは連続して動きます。"),
                 phaseModeAttachment, params::phaseModeID);
    setUpChoice (directionBox, directionLabel, params::directionChoices,
                 jp("方向"), jp("軌道の回転方向を反転します。"),
                 directionAttachment, params::directionID);

    autoGainButton.setButtonText (jp("音量をそろえる"));
    autoGainButton.setName (jp("音量自動補正"));
    autoGainButton.setTooltip (jp("処理前後の音量差を抑え、音の良し悪しを比較しやすくします。"));
    autoGainButton.setWantsKeyboardFocus (true);
    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                              params::autoGainID,
                                                              autoGainButton);

    softBypassButton.setButtonText ("BYPASS");
    softBypassButton.setName (jp("ソフトバイパス"));
    softBypassButton.setTooltip (jp("内部状態を動かしたまま60msで正確な原音へ戻します。"));
    softBypassButton.setWantsKeyboardFocus (true);
    addAndMakeVisible (softBypassButton);
    softBypassAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                                params::softBypassID,
                                                                softBypassButton);

    nullCoreButton.setButtonText ("NULL CORE");
    nullCoreButton.setName ("NULL CORE");
    nullCoreButton.setWantsKeyboardFocus (true);
    nullCoreButton.setTooltip (jp("WetのMidを除去します。モノラルでは消える可能性があります。"));
    addAndMakeVisible (nullCoreButton);
    nullCoreAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts,
                                                              params::nullCoreID,
                                                              nullCoreButton);

    configureOverlayLabel (guideLabel, 10.5f, false, juce::Justification::centredLeft);
    guideLabel.setColour (juce::Label::textColourId,
                          ui::DnaLookAndFeel::mutedTextColour());
    addAndMakeVisible (guideLabel);

    configureOverlayLabel (statusLabel, 11.0f, true, juce::Justification::centred);
    statusLabel.setPillColours (
        ui::DnaLookAndFeel::backgroundColour().withAlpha (0.78f),
        ui::DnaLookAndFeel::centreLockedColour().withAlpha (0.30f));
    addAndMakeVisible (statusLabel);

    configureOverlayLabel (diagnosticLabel, 10.0f, false,
                           juce::Justification::centredRight);
    diagnosticLabel.setColour (juce::Label::textColourId,
                               ui::DnaLookAndFeel::mutedTextColour());
    diagnosticLabel.setPillColours (
        ui::DnaLookAndFeel::backgroundColour().withAlpha (0.72f),
        ui::DnaLookAndFeel::borderColour().withAlpha (0.28f));
    addAndMakeVisible (diagnosticLabel);

    warningLabel.setText (jp("NULL CORE：モノラルでは音が消える可能性があります"),
                          juce::dontSendNotification);
    configureOverlayLabel (warningLabel, 10.5f, true, juce::Justification::centred);
    warningLabel.setColour (juce::Label::textColourId,
                            ui::DnaLookAndFeel::warningColour());
    warningLabel.setPillColours (
        ui::DnaLookAndFeel::warningColour().withAlpha (0.13f),
        ui::DnaLookAndFeel::warningColour().withAlpha (0.48f), 7.0f);
    addAndMakeVisible (warningLabel);
    warningLabel.setVisible (false);

    helixView.setName (jp("DNA軌道表示"));
    helixView.setTooltip (jp("二本の音像、中心軸、左右・前後の軌道をリアルタイム表示します。"));
    addAndMakeVisible (helixView);
    helixView.toBack();

    if (auto* nullCoreParam = processorRef.apvts.getParameter (params::nullCoreID))
    {
        nullCoreWatcher = std::make_unique<juce::ParameterAttachment> (
            *nullCoreParam,
            [this] (float value)
            {
                const bool on = value > 0.5f;
                warningLabel.setVisible (on);
                setKnobEnabled (coreKnob, ! on);
            },
            nullptr);
        nullCoreWatcher->sendInitialUpdate();
    }

    showPage ((int) uiStateTree().getProperty (params::editorPagePropertyID, 0));
    setResizable (true, true);
    setResizeLimits (minimumEditorWidth, minimumEditorHeight, 1680, 1200);

    const int savedWidth = (int) uiStateTree().getProperty (params::editorWidthPropertyID, 960);
    const int savedHeight = (int) uiStateTree().getProperty (params::editorHeightPropertyID, 700);
    setSize (juce::jlimit (minimumEditorWidth, 1680, savedWidth),
             juce::jlimit (minimumEditorHeight, 1200, savedHeight));
    startTimerHz (15);
}

DNAOrbitAudioProcessorEditor::~DNAOrbitAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DNAOrbitAudioProcessorEditor::setUpKnob (Knob& knob,
                                              const juce::String& paramID,
                                              const juce::String& name,
                                              const juce::String& hint,
                                              const juce::String& tooltip)
{
    knob.slider.setName (name);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    knob.slider.setTooltip (tooltip);
    knob.slider.setWantsKeyboardFocus (true);
    knob.slider.setMouseDragSensitivity (170);
    knob.slider.setSliderSnapsToMousePosition (false);
    knob.slider.setPopupDisplayEnabled (true, false, this);

    if (auto* parameter = processorRef.apvts.getParameter (paramID))
        knob.slider.setDoubleClickReturnValue (
            true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

    addAndMakeVisible (knob.slider);

    knob.nameLabel.setText (name, juce::dontSendNotification);
    knob.nameLabel.setFont (japaneseFont (12.5f, true));
    knob.nameLabel.setJustificationType (juce::Justification::centred);
    knob.nameLabel.setColour (juce::Label::textColourId,
                              ui::DnaLookAndFeel::textColour());
    knob.nameLabel.setTooltip (tooltip);
    knob.nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (knob.nameLabel);

    knob.hintLabel.setText (hint, juce::dontSendNotification);
    knob.hintLabel.setFont (japaneseFont (9.5f));
    knob.hintLabel.setJustificationType (juce::Justification::centred);
    knob.hintLabel.setColour (juce::Label::textColourId,
                              ui::DnaLookAndFeel::mutedTextColour());
    knob.hintLabel.setInterceptsMouseClicks (false, false);
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
    label.setFont (japaneseFont (10.0f, true));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setColour (juce::Label::textColourId,
                     ui::DnaLookAndFeel::mutedTextColour());
    label.setTooltip (tooltip);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);

    box.setName (labelText);
    box.addItemList (items, 1);
    box.setTooltip (tooltip);
    box.setWantsKeyboardFocus (true);
    addAndMakeVisible (box);
    attachment = std::make_unique<ComboAttachment> (processorRef.apvts, paramID, box);
}

void DNAOrbitAudioProcessorEditor::setKnobVisible (Knob& knob, bool visible)
{
    knob.slider.setVisible (visible);
    knob.nameLabel.setVisible (visible);
    knob.hintLabel.setVisible (visible);
}

void DNAOrbitAudioProcessorEditor::setKnobEnabled (Knob& knob, bool enabled)
{
    knob.slider.setEnabled (enabled);
    knob.nameLabel.setEnabled (enabled);
    knob.hintLabel.setEnabled (enabled);
}

bool DNAOrbitAudioProcessorEditor::usesCompactDetailLayout() const noexcept
{
    return getWidth() < 1120;
}

int DNAOrbitAudioProcessorEditor::currentControlPanelHeight() const noexcept
{
    if (currentPage == 0)
        return 214;
    return usesCompactDetailLayout() ? 318 : 246;
}

void DNAOrbitAudioProcessorEditor::applyPreset (int presetIndex)
{
    presets::apply (processorRef.apvts, presets::presets[presetIndex]);
    currentPresetIndex = presetIndex;
    presetStateLabel.setVisible (false);
    revertButton.setVisible (false);
}

void DNAOrbitAudioProcessorEditor::showPage (int page)
{
    currentPage = juce::jlimit (0, 1, page);
    uiStateTree().setProperty (params::editorPagePropertyID, currentPage, nullptr);
    const bool basic = currentPage == 0;

    for (auto* knob : { &rateKnob, &radiusKnob, &depthKnob, &mixKnob })
        setKnobVisible (*knob, basic);

    for (auto* knob : { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob,
                        &stereoPreserveKnob, &bassAnchorKnob, &startPhaseKnob })
        setKnobVisible (*knob, ! basic);

    for (auto* component : { static_cast<juce::Component*> (&characterBox),
                             &characterLabel, &phaseModeBox, &phaseModeLabel,
                             &directionBox, &directionLabel, &nullCoreButton })
        component->setVisible (! basic);

    syncButton.setVisible (true);
    divisionBox.setVisible (true);
    divisionLabel.setVisible (true);
    autoGainButton.setVisible (true);
    softBypassButton.setVisible (true);
    diagnosticLabel.setVisible (! basic);

    basicTabButton.setToggleState (basic, juce::dontSendNotification);
    detailTabButton.setToggleState (! basic, juce::dontSendNotification);

    resized();
    repaint();
}

void DNAOrbitAudioProcessorEditor::timerCallback()
{
    const auto visual = processorRef.getEngine().getVisualState();
    const bool axisLocked = helixView.isAxisLocked();
    const bool syncOn = processorRef.apvts.getRawParameterValue (params::syncID)->load() > 0.5f;
    const int phaseMode = (int) processorRef.apvts.getRawParameterValue (params::phaseModeID)->load();

    setKnobEnabled (rateKnob, ! syncOn);
    divisionBox.setEnabled (syncOn);
    divisionLabel.setEnabled (syncOn);
    phaseModeBox.setEnabled (syncOn);
    phaseModeLabel.setEnabled (syncOn);
    setKnobEnabled (startPhaseKnob, syncOn && phaseMode != params::phaseFree);

    if (currentPage == 0)
    {
        guideLabel.setText (
            syncOn ? jp("テンポ同期中：速さはホストBPMと分割で決まります")
                   : jp("まずは広がり・立体感・効果量を調整。速さは動きの存在感を決めます"),
            juce::dontSendNotification);
    }
    else if (visual.nullCoreOn)
    {
        guideLabel.setText (jp("NULL CORE中：中心の芯は無効です。モノ互換を必ず確認してください"),
                            juce::dontSendNotification);
    }
    else if (syncOn && phaseMode == params::phaseHostLock)
    {
        guideLabel.setText (jp("HOST LOCK：同じ曲位置では同じ軌道を再現します"),
                            juce::dontSendNotification);
    }
    else
    {
        guideLabel.setText (jp("動き・空間・出力の順に調整すると、意図した音へ早く到達できます"),
                            juce::dontSendNotification);
    }

    juce::String status = axisLocked ? "CENTER LOCKED" : "CENTER DRIFT";
    if (visual.hostPhaseLocked)
        status << "  •  HOST LOCK";
    statusLabel.setText (status, juce::dontSendNotification);
    const auto statusColour = axisLocked ? ui::DnaLookAndFeel::centreLockedColour()
                                         : ui::DnaLookAndFeel::centreDriftColour();
    statusLabel.setColour (juce::Label::textColourId, statusColour);
    statusLabel.setPillColours (
        ui::DnaLookAndFeel::backgroundColour().withAlpha (0.78f),
        statusColour.withAlpha (0.34f));

    const auto signed2 = [] (float value)
    {
        return juce::String (value >= 0.0f ? "+" : "-")
             + juce::String (std::abs (value), 2);
    };

    juce::String diagnostic;
    diagnostic << "PAN A " << signed2 (helixView.getPanA())
               << "   B " << signed2 (helixView.getPanB()) << "\n"
               << "CENTER " << juce::String (helixView.getCentroidX(), 3)
               << "   CORR " << juce::String (helixView.getCorrelation(), 2)
               << "   BASS "
               << (visual.bassAnchorHz <= 20.1f ? juce::String ("OFF")
                                                : juce::String (visual.bassAnchorHz, 0) + " Hz");
    diagnosticLabel.setText (diagnostic, juce::dontSendNotification);

    const bool modified = currentPresetIndex >= 0
                       && currentPresetIndex < presets::numPresets
                       && ! presets::matchesCurrentState (
                              processorRef.apvts, presets::presets[currentPresetIndex]);
    presetStateLabel.setVisible (modified);
    revertButton.setVisible (modified);
}

void DNAOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient background (ui::DnaLookAndFeel::backgroundColour().brighter (0.055f),
                                     bounds.getCentreX(), bounds.getY(),
                                     ui::DnaLookAndFeel::backgroundColour().darker (0.25f),
                                     bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (background);
    g.fillRect (bounds);

    g.setColour (ui::DnaLookAndFeel::strandAColour().withAlpha (0.48f));
    g.fillRect (14.0f, 76.0f, (float) getWidth() - 28.0f, 1.0f);

    drawCard (g, visualCardBounds,
              ui::DnaLookAndFeel::surfaceColour().withAlpha (0.74f),
              ui::DnaLookAndFeel::borderColour().withAlpha (0.48f), 12.0f);
    drawCard (g, controlPanelBounds,
              ui::DnaLookAndFeel::surfaceColour().withAlpha (0.94f),
              ui::DnaLookAndFeel::borderColour().withAlpha (0.62f), 12.0f);

    if (currentPage == 1)
    {
        drawCard (g, motionCardBounds, ui::DnaLookAndFeel::raisedColour().withAlpha (0.66f),
                  ui::DnaLookAndFeel::strandAColour().withAlpha (0.22f), 9.0f);
        drawCard (g, spaceCardBounds, ui::DnaLookAndFeel::raisedColour().withAlpha (0.66f),
                  ui::DnaLookAndFeel::strandBColour().withAlpha (0.20f), 9.0f);
        drawCard (g, outputCardBounds, ui::DnaLookAndFeel::raisedColour().withAlpha (0.66f),
                  ui::DnaLookAndFeel::borderColour().withAlpha (0.52f), 9.0f);

        g.setFont (japaneseFont (10.0f, true));
        g.setColour (ui::DnaLookAndFeel::strandAColour().withAlpha (0.82f));
        g.drawText (jp("動き  MOTION"), motionCardBounds.reduced (10, 6).removeFromTop (16),
                    juce::Justification::centredLeft);
        g.setColour (ui::DnaLookAndFeel::strandBColour().withAlpha (0.82f));
        g.drawText (jp("空間  SPACE"), spaceCardBounds.reduced (10, 6).removeFromTop (16),
                    juce::Justification::centredLeft);
        g.setColour (ui::DnaLookAndFeel::mutedTextColour());
        g.drawText (jp("出力  OUTPUT"), outputCardBounds.reduced (10, 6).removeFromTop (16),
                    juce::Justification::centredLeft);
    }
    else
    {
        g.setFont (japaneseFont (10.0f, true));
        g.setColour (ui::DnaLookAndFeel::strandAColour().withAlpha (0.78f));
        g.drawText ("ESSENTIAL CONTROLS",
                    controlPanelBounds.reduced (14, 8).removeFromTop (16),
                    juce::Justification::centredLeft);
    }
}

void DNAOrbitAudioProcessorEditor::layOutKnobRow (
    juce::Rectangle<int> row, const std::vector<Knob*>& knobs, int minimumSlotWidth)
{
    if (knobs.empty() || row.isEmpty())
        return;

    const int slotWidth = juce::jmax (minimumSlotWidth,
                                      row.getWidth() / (int) knobs.size());
    for (size_t index = 0; index < knobs.size(); ++index)
    {
        const int remaining = (int) knobs.size() - (int) index;
        const int width = remaining == 1 ? row.getWidth()
                                         : juce::jmin (slotWidth, row.getWidth());
        auto slot = row.removeFromLeft (width).reduced (3, 0);
        knobs[index]->nameLabel.setBounds (slot.removeFromTop (18));
        knobs[index]->hintLabel.setBounds (slot.removeFromBottom (15));
        knobs[index]->slider.setBounds (slot.reduced (1, 0));
    }
}

void DNAOrbitAudioProcessorEditor::layoutBasicControls (juce::Rectangle<int> area)
{
    area.reduce (14, 8);
    area.removeFromTop (18);
    guideLabel.setBounds (area.removeFromTop (24));

    auto optionRow = area.removeFromBottom (34);
    autoGainButton.setBounds (optionRow.removeFromRight (170).reduced (4, 2));
    optionRow.removeFromRight (8);
    syncButton.setBounds (optionRow.removeFromLeft (132).reduced (4, 2));
    divisionLabel.setBounds (optionRow.removeFromLeft (44));
    divisionBox.setBounds (optionRow.removeFromLeft (148).reduced (0, 2));

    area.removeFromBottom (5);
    layOutKnobRow (area, { &rateKnob, &radiusKnob, &depthKnob, &mixKnob }, 112);
}

void DNAOrbitAudioProcessorEditor::layoutDetailControls (juce::Rectangle<int> area)
{
    area.reduce (10, 8);
    guideLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (4);

    constexpr int gap = 8;
    if (usesCompactDetailLayout())
    {
        auto top = area.removeFromTop ((area.getHeight() - gap) / 2);
        area.removeFromTop (gap);
        auto bottom = area;

        motionCardBounds = top;
        const int outputWidth = juce::jmax (226, bottom.getWidth() * 30 / 100);
        outputCardBounds = bottom.removeFromRight (outputWidth);
        bottom.removeFromRight (gap);
        spaceCardBounds = bottom;
    }
    else
    {
        const int motionWidth = area.getWidth() * 42 / 100;
        const int spaceWidth = area.getWidth() * 37 / 100;
        motionCardBounds = area.removeFromLeft (motionWidth);
        area.removeFromLeft (gap);
        spaceCardBounds = area.removeFromLeft (spaceWidth);
        area.removeFromLeft (gap);
        outputCardBounds = area;
    }

    auto motion = motionCardBounds.reduced (9, 7);
    motion.removeFromTop (18);
    auto motionConfig = motion.removeFromRight (juce::jmax (220, motion.getWidth() * 42 / 100));
    motion.removeFromRight (6);
    layOutKnobRow (motion, { &symmetryKnob, &twistKnob, &startPhaseKnob }, 82);

    auto row = motionConfig.removeFromTop (25);
    syncButton.setBounds (row.removeFromLeft (118).reduced (2, 1));
    divisionLabel.setBounds (row.removeFromLeft (42));
    divisionBox.setBounds (row.reduced (0, 1));
    motionConfig.removeFromTop (4);

    auto choiceRow = [] (juce::Rectangle<int>& column, juce::Label& label,
                         juce::ComboBox& box)
    {
        auto r = column.removeFromTop (25);
        label.setBounds (r.removeFromLeft (68));
        box.setBounds (r.reduced (0, 1));
        column.removeFromTop (3);
    };
    choiceRow (motionConfig, phaseModeLabel, phaseModeBox);
    choiceRow (motionConfig, directionLabel, directionBox);

    auto space = spaceCardBounds.reduced (9, 7);
    space.removeFromTop (18);
    auto characterRow = space.removeFromBottom (28);
    characterLabel.setBounds (characterRow.removeFromLeft (54));
    characterBox.setBounds (characterRow.reduced (0, 1));
    space.removeFromBottom (3);
    layOutKnobRow (space, { &coreKnob, &stereoPreserveKnob, &bassAnchorKnob }, 82);

    auto output = outputCardBounds.reduced (9, 7);
    output.removeFromTop (18);
    auto toggles = output.removeFromBottom (84);
    autoGainButton.setBounds (toggles.removeFromTop (26).reduced (2, 1));
    toggles.removeFromTop (2);
    nullCoreButton.setBounds (toggles.removeFromTop (26).reduced (2, 1));
    layOutKnobRow (output, { &outputKnob }, 92);
}

void DNAOrbitAudioProcessorEditor::resized()
{
    uiStateTree().setProperty (params::editorWidthPropertyID, getWidth(), nullptr);
    uiStateTree().setProperty (params::editorHeightPropertyID, getHeight(), nullptr);

    auto area = getLocalBounds().reduced (14);
    auto header = area.removeFromTop (56);
    area.removeFromTop (10);

    auto titleArea = header.removeFromLeft (232);
    productTagLabel.setBounds (titleArea.removeFromTop (13));
    titleLabel.setBounds (titleArea.removeFromTop (25));
    subtitleLabel.setBounds (titleArea);

    auto tabs = header.removeFromLeft (140).reduced (2, 10);
    basicTabButton.setBounds (tabs.removeFromLeft (66));
    tabs.removeFromLeft (5);
    detailTabButton.setBounds (tabs.removeFromLeft (66));

    auto headerControls = header;
    softBypassButton.setBounds (headerControls.removeFromRight (112).reduced (5, 10));
    revertButton.setBounds (headerControls.removeFromRight (58).reduced (3, 10));
    presetStateLabel.setBounds (headerControls.removeFromRight (64).reduced (3, 11));
    presetLabel.setBounds (headerControls.removeFromLeft (58));
    presetBox.setBounds (headerControls.reduced (4, 10));

    controlPanelBounds = area.removeFromBottom (currentControlPanelHeight());
    area.removeFromBottom (10);
    visualCardBounds = area;

    helixView.setBounds (visualCardBounds.reduced (5));
    statusLabel.setBounds (visualCardBounds.getX() + 12,
                           visualCardBounds.getY() + 12, 188, 28);
    diagnosticLabel.setBounds (visualCardBounds.getRight() - 292,
                               visualCardBounds.getY() + 12, 280, 42);
    warningLabel.setBounds (visualCardBounds.getCentreX() - 190,
                            visualCardBounds.getBottom() - 38, 380, 26);

    motionCardBounds = {};
    spaceCardBounds = {};
    outputCardBounds = {};
    if (currentPage == 0)
        layoutBasicControls (controlPanelBounds);
    else
        layoutDetailControls (controlPanelBounds);
}
