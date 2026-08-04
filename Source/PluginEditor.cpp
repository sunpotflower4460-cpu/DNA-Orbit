#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"

using namespace dnaorbit;
using dnaorbit::ui::jp;

juce::ValueTree DNAOrbitAudioProcessorEditor::uiStateTree() const
{
    return processorRef.apvts.state.getOrCreateChildWithName (dnaorbit::params::uiStateNodeID, nullptr);
}

juce::Font DNAOrbitAudioProcessorEditor::japaneseFont (float height, bool bold)
{
    return dnaorbit::ui::DnaLookAndFeel::uiFont (height, bold);
}

DNAOrbitAudioProcessorEditor::DNAOrbitAudioProcessorEditor (DNAOrbitAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      helixView (p.getEngine())
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("DNA ORBIT", juce::dontSendNotification);
    titleLabel.setFont (japaneseFont (21.0f, true));
    titleLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText (jp("2つの音像が中心軸の反対側を回ります"), juce::dontSendNotification);
    subtitleLabel.setFont (japaneseFont (11.0f));
    subtitleLabel.setColour (juce::Label::textColourId,
                             dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.6f));
    subtitleLabel.setTooltip (jp("Ctrl+Z(macではCmd+Z)で元に戻す、Ctrl+Shift+Zでやり直せます。"));
    addAndMakeVisible (subtitleLabel);

    addAndMakeVisible (helixView);

    // --- Tabs -----------------------------------------------------------------
    for (auto* button : { &basicTabButton, &detailTabButton })
    {
        button->setClickingTogglesState (false);
        button->setColour (juce::TextButton::buttonColourId, dnaorbit::ui::DnaLookAndFeel::panelColour());
        button->setColour (juce::TextButton::textColourOffId, dnaorbit::ui::DnaLookAndFeel::textColour());
        addAndMakeVisible (*button);
    }
    basicTabButton.setButtonText (jp("基本"));
    detailTabButton.setButtonText (jp("詳細"));
    basicTabButton.onClick  = [this] { showPage (0); };
    detailTabButton.onClick = [this] { showPage (1); };

    // --- Presets ---------------------------------------------------------------
    presetLabel.setText (jp("プリセット"), juce::dontSendNotification);
    presetLabel.setFont (japaneseFont (11.0f));
    presetLabel.setJustificationType (juce::Justification::centredRight);
    presetLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (presetLabel);

    presetBox.setTextWhenNothingSelected (jp("選んでください"));
    for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
        presetBox.addItem (jp (dnaorbit::presets::presets[i].name), i + 1);
    presetBox.setTooltip (jp("まずここから選ぶのがおすすめです。つまみは後から微調整できます。"));
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index < dnaorbit::presets::numPresets)
            applyPreset (index);
    };

    revertButton.setButtonText (jp("元に戻す"));
    revertButton.setTooltip (jp("プリセットを選んだ後に動かしたつまみを、プリセットの値に戻します。"));
    revertButton.onClick = [this]
    {
        if (currentPresetIndex >= 0 && currentPresetIndex < dnaorbit::presets::numPresets)
            applyPreset (currentPresetIndex);
    };
    addAndMakeVisible (revertButton);
    revertButton.setVisible (false);

    // --- Soft Bypass (top bar, both tabs) ---------------------------------------
    bypassButton.setButtonText (jp("バイパス"));
    bypassButton.setColour (juce::ToggleButton::tickColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    bypassButton.setTooltip (jp("プラグイン内蔵のバイパスです(ホスト側のBypassとは別)。")
                             + jp("約30msでドライ音へなめらかに切り替わり、クリックが出ません。")
                             + jp("軌道の位相は裏側で回り続けるので、解除しても不自然な段差は出ません。"));
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, params::softBypassID, bypassButton);

    monoPreviewButton.setButtonText (jp("モノ確認"));
    monoPreviewButton.setTooltip (jp("最終出力をモノラルに折り畳んで試聴します(モニター専用)。")
                                  + jp("約30msでなめらかに切り替わります。左右の打ち消しがないか確認するのに使います。"));
    addAndMakeVisible (monoPreviewButton);
    monoPreviewAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, params::monoPreviewID, monoPreviewButton);

    // --- Basic page knobs -------------------------------------------------------
    setUpKnob (rateKnob, params::rateID, jp("速さ"), jp("1周する時間"),
               jp("音像が中心軸を1周するのにかかる時間です。ゆっくりだと自然、速いと目立ちます。"));
    setUpKnob (radiusKnob, params::radiusID, jp("広がり"), jp("左右の幅"),
               jp("2つの音像が左右へどれだけ離れるかです。0%で中央、100%で最大の広がり。"));
    setUpKnob (depthKnob, params::depthID, jp("立体感"), jp("前後の奥行き"),
               jp("前後方向の奥行き感です。0%だと左右の往復だけ、上げると円を描くように聞こえます。"));
    setUpKnob (mixKnob, params::mixID, jp("効果量"), jp("原音とのブレンド"),
               jp("エフェクト音の量です。0%で原音そのまま。30〜40%が実用的な範囲です。"));

    // --- Detail page ------------------------------------------------------------
    setUpKnob (symmetryKnob, params::symmetryID, jp("対称性"), jp("中心の固定度"),
               jp("100%で2つの音像が正確に反対側を保ち、中心が完全に固定されます。")
               + jp("下げるとBの速度がわずかに変わり、中心が生き物のように漂い始めます。"));
    setUpKnob (twistKnob, params::twistID, jp("ねじれ"), jp("2本の時間差"),
               jp("2本の音像に与える微小な時間差です。2本を区別しやすくします。極性反転ではありません。"));
    setUpKnob (coreKnob, params::coreID, jp("中心の芯"), jp("中央に残す音"),
               jp("中心軸に元の音をどれだけ残すかです。0%で中心が空洞、上げると中央に芯が現れます。"));
    setUpKnob (outputKnob, params::outputID, jp("出力"), jp("最終音量"),
               jp("最終的な出力音量の微調整です。"));
    setUpKnob (stereoPreserveKnob, params::stereoPreserveID, jp("ステレオ保持"), jp("左右の情報量"),
               jp("エフェクト音に元のステレオ感をどれだけ残すかです。0%は中央成分のみ、")
               + jp("100%で元の左右の広がりがそのまま加わります。逆位相の素材でも音が消えにくくなります。"));
    setUpKnob (startPhaseKnob, params::startPhaseID, jp("開始位相"), jp("回り始める角度"),
               jp("リトリガー時に戻る角度、およびホスト同期時の位相の基準点です。")
               + jp("位相モードがフリーのときは音に影響しません。"));
    setUpKnob (bassAnchorKnob, params::bassAnchorHzID, jp("低音アンカー"), jp("低域を安定させる"),
               jp("設定した周波数より下の低音は軌道に乗らず、元の定位のまま真っ直ぐ残ります。")
               + jp("Offで無効。キックやベースの中心が動いて不安定に感じるときに上げてください。"));

    syncButton.setButtonText (jp("テンポ同期"));
    syncButton.setTooltip (jp("ホストのテンポに合わせて回転速度を決めます。テンポが取得できない場合は「速さ」の値に戻ります。"));
    addAndMakeVisible (syncButton);
    syncAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, params::syncID, syncButton);

    divisionLabel.setText (jp("分割"), juce::dontSendNotification);
    divisionLabel.setFont (japaneseFont (11.0f));
    divisionLabel.setJustificationType (juce::Justification::centredLeft);
    divisionLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (divisionLabel);

    divisionBox.addItemList (params::syncDivisionChoices, 1);
    divisionBox.setTooltip (jp("テンポ同期時に、1周を何小節・何拍で回るかを選びます。"));
    addAndMakeVisible (divisionBox);
    divisionAttachment = std::make_unique<ComboAttachment> (processorRef.apvts, params::divisionID, divisionBox);

    autoGainButton.setButtonText (jp("音量自動補正"));
    autoGainButton.setTooltip (jp("エフェクト音の音量を原音に自動で合わせます。")
                               + jp("オンなら「効果量」を動かしても体感音量が変わらないので、比較しやすくなります。"));
    addAndMakeVisible (autoGainButton);
    autoGainAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, params::autoGainID, autoGainButton);

    nullCoreButton.setButtonText (jp("NULL CORE (実験的)"));
    nullCoreButton.setColour (juce::ToggleButton::textColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    nullCoreButton.setColour (juce::ToggleButton::tickColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    nullCoreButton.setTooltip (jp("実験モード。エフェクト音のMid成分を取り除きます。")
                               + jp("モノラルにまとめるとエフェクト音がほぼ消えます。通常は切っておいてください。"));
    addAndMakeVisible (nullCoreButton);
    nullCoreAttachment = std::make_unique<ButtonAttachment> (processorRef.apvts, params::nullCoreID, nullCoreButton);

    characterLabel.setText (jp("音色"), juce::dontSendNotification);
    characterLabel.setFont (japaneseFont (11.0f));
    characterLabel.setJustificationType (juce::Justification::centredLeft);
    characterLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (characterLabel);

    characterBox.addItemList (juce::StringArray { jp ("ナチュラル"), jp ("ビビッド"), jp ("ディープ") }, 1);
    characterBox.setTooltip (jp("背後に回ったときの音の変化の強さです。ナチュラルが既定(従来の音)、")
                             + jp("ビビッド・ディープと進むほど背後で暗く/揺れが大きくなります。"));
    addAndMakeVisible (characterBox);
    characterAttachment = std::make_unique<ComboAttachment> (processorRef.apvts, params::characterID, characterBox);

    phaseModeLabel.setText (jp("位相"), juce::dontSendNotification);
    phaseModeLabel.setFont (japaneseFont (11.0f));
    phaseModeLabel.setJustificationType (juce::Justification::centredLeft);
    phaseModeLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    addAndMakeVisible (phaseModeLabel);

    phaseModeBox.addItemList (juce::StringArray { jp ("フリー"), jp ("リトリガー"), jp ("ホスト同期") }, 1);
    phaseModeBox.setTooltip (jp("フリーは今までどおり自由に回り続けます。リトリガーは再生開始のたびに")
                             + jp("開始位相へ戻ります。ホスト同期はDAWの再生位置(PPQ)から位相を直接")
                             + jp("計算するので、ループやジャンプをしても常にタイムラインと一致します。"));
    addAndMakeVisible (phaseModeBox);
    phaseModeAttachment = std::make_unique<ComboAttachment> (processorRef.apvts, params::phaseModeID, phaseModeBox);

    directionBox.addItemList (juce::StringArray { "CW", "CCW" }, 1);
    directionBox.setTooltip (jp("回転方向です。CWは時計回り、CCWは反時計回り。")
                             + jp("ホスト同期のときは位相の進む向きにも影響します。"));
    addAndMakeVisible (directionBox);
    directionAttachment = std::make_unique<ComboAttachment> (processorRef.apvts, params::directionID, directionBox);

    // --- Readouts ---------------------------------------------------------------
    // Monospaced with a fixed sign column: digit-width jitter at high refresh
    // rates is the classic cheap-plugin tell.
    readoutLabel.setFont (japaneseFont (11.5f));
    readoutLabel.setColour (juce::Label::textColourId,
                            dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.85f));
    readoutLabel.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (readoutLabel);

    statusLabel.setFont (japaneseFont (13.0f, true));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    transportStatusLabel.setFont (japaneseFont (10.5f));
    transportStatusLabel.setJustificationType (juce::Justification::centred);
    transportStatusLabel.setColour (juce::Label::textColourId,
                                    dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.75f));
    addAndMakeVisible (transportStatusLabel);

    warningLabel.setText (jp("NULL CORE — モノラルで消える可能性があります"), juce::dontSendNotification);
    warningLabel.setFont (japaneseFont (12.0f, true));
    warningLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::warningColour());
    warningLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (warningLabel);
    warningLabel.setVisible (false);

    // Follow the parameter, not the button, so host automation and preset loads
    // keep the warning in sync.
    if (auto* nullCoreParam = processorRef.apvts.getParameter (params::nullCoreID))
    {
        nullCoreWatcher = std::make_unique<juce::ParameterAttachment> (
            *nullCoreParam,
            [this] (float value)
            {
                const bool on = value > 0.5f;
                warningLabel.setVisible (on);

                // Same principle as the Rate knob under an active Sync
                // (ADR-014): NULL CORE removes exactly the Mid content the
                // Core knob adds, so Core goes completely inert while it is
                // on. The README already documented that as expected
                // behaviour; leaving the knob looking live was the gap.
                coreKnob.slider.setAlpha (on ? 0.45f : 1.0f);
                coreKnob.hintLabel.setText (on ? jp("NULL CORE中は無効") : jp("中央に残す音"),
                                             juce::dontSendNotification);
                coreKnob.hintLabel.setColour (juce::Label::textColourId,
                                               on
                                                   ? dnaorbit::ui::DnaLookAndFeel::warningColour()
                                                   : dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.55f));
            },
            nullptr);
        nullCoreWatcher->sendInitialUpdate();
    }

    showPage (uiStateTree().getProperty (params::editorPagePropertyID, 0));

    setResizable (true, true);
    setResizeLimits (780, 540, 1600, 1100);

    const int savedWidth  = uiStateTree().getProperty (params::editorWidthPropertyID, 900);
    const int savedHeight = uiStateTree().getProperty (params::editorHeightPropertyID, 620);
    setSize (juce::jlimit (780, 1600, savedWidth), juce::jlimit (540, 1100, savedHeight));

    startTimerHz (12);

    // Ctrl+Z / Ctrl+Shift+Z (Cmd on macOS, via commandModifier) for
    // Undo/Redo - see keyPressed() and the undoManager member on
    // DNAOrbitAudioProcessor. Key events bubble up from whichever child has
    // focus to this top-level component if unhandled, so this alone is
    // enough without wiring every individual control.
    setWantsKeyboardFocus (true);
}

DNAOrbitAudioProcessorEditor::~DNAOrbitAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DNAOrbitAudioProcessorEditor::setUpKnob (Knob& knob, const juce::String& paramID,
                                              const juce::String& name, const juce::String& hint,
                                              const juce::String& tooltip)
{
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 17);
    knob.slider.setTooltip (tooltip);
    addAndMakeVisible (knob.slider);

    knob.nameLabel.setText (name, juce::dontSendNotification);
    knob.nameLabel.setFont (japaneseFont (14.0f, true));
    knob.nameLabel.setJustificationType (juce::Justification::centred);
    knob.nameLabel.setColour (juce::Label::textColourId, dnaorbit::ui::DnaLookAndFeel::textColour());
    knob.nameLabel.setTooltip (tooltip);
    addAndMakeVisible (knob.nameLabel);

    knob.hintLabel.setText (hint, juce::dontSendNotification);
    knob.hintLabel.setFont (japaneseFont (10.0f));
    knob.hintLabel.setJustificationType (juce::Justification::centred);
    knob.hintLabel.setColour (juce::Label::textColourId,
                              dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.55f));
    addAndMakeVisible (knob.hintLabel);

    knob.attachment = std::make_unique<SliderAttachment> (processorRef.apvts, paramID, knob.slider);
}

void DNAOrbitAudioProcessorEditor::applyPreset (int presetIndex)
{
    dnaorbit::presets::apply (processorRef.apvts, dnaorbit::presets::presets[presetIndex],
                              &processorRef.undoManager);
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

    for (auto* knob : { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob, &stereoPreserveKnob,
                        &bassAnchorKnob, &startPhaseKnob })
    {
        knob->slider.setVisible (! basic);
        knob->nameLabel.setVisible (! basic);
        knob->hintLabel.setVisible (! basic);
    }

    syncButton.setVisible (! basic);
    divisionBox.setVisible (! basic);
    divisionLabel.setVisible (! basic);
    autoGainButton.setVisible (! basic);
    nullCoreButton.setVisible (! basic);
    characterBox.setVisible (! basic);
    characterLabel.setVisible (! basic);
    phaseModeBox.setVisible (! basic);
    phaseModeLabel.setVisible (! basic);
    directionBox.setVisible (! basic);

    presetBox.setVisible (basic);
    presetLabel.setVisible (basic);
    if (! basic)
        revertButton.setVisible (false);

    const auto activeColour = dnaorbit::ui::DnaLookAndFeel::strandAColour().withAlpha (0.35f);
    const auto idleColour = dnaorbit::ui::DnaLookAndFeel::panelColour();
    basicTabButton.setColour (juce::TextButton::buttonColourId, basic ? activeColour : idleColour);
    detailTabButton.setColour (juce::TextButton::buttonColourId, basic ? idleColour : activeColour);

    resized();
    repaint();
}

bool DNAOrbitAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0))
    {
        processorRef.undoManager.undo();
        return true;
    }

    if (key == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)
        || key == juce::KeyPress ('y', juce::ModifierKeys::commandModifier, 0))
    {
        processorRef.undoManager.redo();
        return true;
    }

    return false;
}

void DNAOrbitAudioProcessorEditor::timerCallback()
{
    const bool locked = helixView.isAxisLocked();

    statusLabel.setText (locked ? jp("中心軸 固定 (0)") : jp("中心軸 ゆらぎ"), juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           locked ? dnaorbit::ui::DnaLookAndFeel::centreLockedColour()
                                  : dnaorbit::ui::DnaLookAndFeel::centreDriftColour());

    // Show the +/- physics literally: the two pan positions and their midpoint.
    // 4 decimals on the centre so it is visibly zero rather than merely rounded.
    const auto signed3 = [] (float value)
    {
        return juce::String (value >= 0.0f ? "+" : "-") + juce::String (std::abs (value), 3);
    };

    juce::String text;
    text << "A  " << signed3 (helixView.getPanA()) << "\n"
         << "B  " << signed3 (helixView.getPanB()) << "\n"
         << "中心 " << juce::String (helixView.getCentroidX(), 4) << "\n"
         << "位相差-180  " << juce::String (helixView.getPhaseErrorDegrees(), 1) << "°\n"
         << "相関  " << juce::String (helixView.getCorrelation(), 2);

    readoutLabel.setText (text, juce::dontSendNotification);

    // --- What is actually driving the orbit (spec 03 §3.1 / §3.4) ----------
    // The Rate knob is completely ignored while Sync is on and the host
    // supplies a tempo, so saying so is not decoration: without it the user
    // turns 速さ, hears nothing change, and has no way to know why.
    const auto rateSource = processorRef.getRateSourceForUi();
    juce::String transportText;
    auto transportColour = dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.75f);

    switch (rateSource)
    {
        case DNAOrbitAudioProcessor::RateSource::syncedToHost:
            transportText = jp("テンポ同期中 — 速さは分割が決めています");
            transportColour = dnaorbit::ui::DnaLookAndFeel::strandAColour();
            break;
        case DNAOrbitAudioProcessor::RateSource::syncFallbackNoTempo:
            // Sync is on but inert: showing a plain "SYNC" here would be a
            // lie, since the orbit is free-running off the Rate knob.
            transportText = jp("テンポ同期ON — ホストのテンポ不明のため「速さ」で動作中");
            transportColour = dnaorbit::ui::DnaLookAndFeel::warningColour();
            break;
        case DNAOrbitAudioProcessor::RateSource::freeRunning:
        default:
            transportText = {};
            break;
    }

    // Host Lock state chip, appended to the same line.
    const int phaseMode = phaseModeBox.getSelectedItemIndex();
    if (phaseMode == 2) // Host Lock
    {
        const auto lockText = processorRef.isHostPlayingForUi()
                            ? jp("ホスト同期: 曲位置にロック中")
                            : jp("ホスト同期: 再生待ち");
        transportText = transportText.isEmpty() ? lockText : transportText + jp(" / ") + lockText;
    }
    else if (phaseMode == 1) // Retrigger
    {
        const auto retrigText = jp("リトリガー: 再生開始で開始位相へ");
        transportText = transportText.isEmpty() ? retrigText : transportText + jp(" / ") + retrigText;
    }

    transportStatusLabel.setText (transportText, juce::dontSendNotification);
    transportStatusLabel.setColour (juce::Label::textColourId, transportColour);

    // Mirror the same fact onto the Rate knob's own hint, so it is visible
    // on the Basic tab where the knob actually lives. Only rewritten on an
    // actual state change - setText on every tick would re-lay-out glyphs
    // 12 times a second for nothing.
    const int rateSourceIndex = static_cast<int> (rateSource);
    if (rateSourceIndex != lastRateSourceShown)
    {
        lastRateSourceShown = rateSourceIndex;

        const bool rateIsInert = rateSource == DNAOrbitAudioProcessor::RateSource::syncedToHost;
        rateKnob.hintLabel.setText (rateIsInert ? jp("テンポ同期中は無効") : jp("1周する時間"),
                                     juce::dontSendNotification);
        rateKnob.hintLabel.setColour (juce::Label::textColourId,
                                       rateIsInert
                                           ? dnaorbit::ui::DnaLookAndFeel::warningColour()
                                           : dnaorbit::ui::DnaLookAndFeel::textColour().withAlpha (0.55f));
        rateKnob.slider.setAlpha (rateIsInert ? 0.45f : 1.0f);
    }

    // Modified/Revert: once anything drifts from the picked preset's stored
    // values, offer to snap back to it. Cheap enough to just recompute on
    // this already-ticking timer rather than wiring up 12 parameter listeners.
    if (currentPage == 0 && currentPresetIndex >= 0 && currentPresetIndex < dnaorbit::presets::numPresets)
    {
        const bool modified = ! dnaorbit::presets::matchesCurrentState (
            processorRef.apvts, dnaorbit::presets::presets[currentPresetIndex]);
        revertButton.setVisible (modified);
    }
    else
    {
        revertButton.setVisible (false);
    }
}

void DNAOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (dnaorbit::ui::DnaLookAndFeel::backgroundColour());

    // Panel behind the control area.
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (44);
    const auto controlArea = area.removeFromBottom (152);
    g.setColour (dnaorbit::ui::DnaLookAndFeel::panelColour().withAlpha (0.6f));
    g.fillRoundedRectangle (controlArea.toFloat(), 6.0f);
}

void DNAOrbitAudioProcessorEditor::layOutKnobRow (juce::Rectangle<int> row, const std::vector<Knob*>& knobs)
{
    if (knobs.empty())
        return;

    const int slotWidth = row.getWidth() / (int) knobs.size();

    for (auto* knob : knobs)
    {
        auto slot = row.removeFromLeft (slotWidth);
        knob->nameLabel.setBounds (slot.removeFromTop (18));
        knob->hintLabel.setBounds (slot.removeFromBottom (14));
        knob->slider.setBounds (slot.reduced (4, 0));
    }
}

void DNAOrbitAudioProcessorEditor::resized()
{
    uiStateTree().setProperty (params::editorWidthPropertyID, getWidth(), nullptr);
    uiStateTree().setProperty (params::editorHeightPropertyID, getHeight(), nullptr);

    auto area = getLocalBounds().reduced (12);

    // --- Top bar ---------------------------------------------------------------
    auto topBar = area.removeFromTop (44);
    auto titleArea = topBar.removeFromLeft (220);
    titleLabel.setBounds (titleArea.removeFromTop (24));
    subtitleLabel.setBounds (titleArea);

    auto tabArea = topBar.removeFromLeft (140).reduced (0, 8);
    basicTabButton.setBounds (tabArea.removeFromLeft (66));
    tabArea.removeFromLeft (4);
    detailTabButton.setBounds (tabArea.removeFromLeft (66));

    auto presetArea = topBar.removeFromRight (320).reduced (0, 9);
    revertButton.setBounds (presetArea.removeFromRight (58));
    presetArea.removeFromRight (6);
    presetLabel.setBounds (presetArea.removeFromLeft (74));
    presetArea.removeFromLeft (6);
    presetBox.setBounds (presetArea);

    // Whatever remains of topBar (between the tabs and the preset menu) is
    // the Soft Bypass / Mono Preview toggles - visible on both pages since
    // they are top-level, always-relevant controls.
    auto utilityArea = topBar.reduced (4, 9);
    bypassButton.setBounds (utilityArea.removeFromLeft (utilityArea.getWidth() / 2));
    monoPreviewButton.setBounds (utilityArea);

    // --- Control area ----------------------------------------------------------
    auto controlArea = area.removeFromBottom (152).reduced (8);

    if (currentPage == 0)
    {
        layOutKnobRow (controlArea, { &rateKnob, &radiusKnob, &depthKnob, &mixKnob });
    }
    else
    {
        auto toggleColumn = controlArea.removeFromRight
            (juce::jmin (240, controlArea.getWidth() / 3));
        toggleColumn.reduce (4, 2);

        auto syncRow = toggleColumn.removeFromTop (26);
        syncButton.setBounds (syncRow.removeFromLeft (110));
        divisionLabel.setBounds (syncRow.removeFromLeft (34));
        divisionBox.setBounds (syncRow);

        toggleColumn.removeFromTop (4);
        autoGainButton.setBounds (toggleColumn.removeFromTop (26));
        toggleColumn.removeFromTop (4);
        nullCoreButton.setBounds (toggleColumn.removeFromTop (26));

        toggleColumn.removeFromTop (4);
        auto characterRow = toggleColumn.removeFromTop (26);
        characterLabel.setBounds (characterRow.removeFromLeft (34));
        characterBox.setBounds (characterRow);

        toggleColumn.removeFromTop (4);
        auto phaseRow = toggleColumn; // whatever remains
        phaseModeLabel.setBounds (phaseRow.removeFromLeft (34));
        directionBox.setBounds (phaseRow.removeFromRight (54));
        phaseModeBox.setBounds (phaseRow);

        layOutKnobRow (controlArea, { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob,
                                       &stereoPreserveKnob, &bassAnchorKnob, &startPhaseKnob });
    }

    // --- Centre: readouts | 3D helix ------------------------------------------
    auto centreArea = area;
    auto readoutColumn = centreArea.removeFromLeft (juce::jmin (140, centreArea.getWidth() / 5));

    statusLabel.setBounds (readoutColumn.removeFromTop (24));
    readoutColumn.removeFromTop (6);
    readoutLabel.setBounds (readoutColumn.removeFromTop (90));

    warningLabel.setBounds (centreArea.removeFromBottom (20));
    // Directly under the helix and spanning its full width: this line
    // explains why the orbit is moving the way it is, so it belongs next to
    // the motion itself rather than tucked into the narrow readout column.
    transportStatusLabel.setBounds (centreArea.removeFromBottom (16));
    helixView.setBounds (centreArea.reduced (4));
}
