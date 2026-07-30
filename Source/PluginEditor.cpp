#include "PluginEditor.h"
#include "Parameters.h"

using dnaorbit::ui::jp;

namespace
{
    using namespace dnaorbit;

    /** Factory presets, named after what they DO rather than what they are. */
    struct Preset
    {
        const char* name;
        float rate, radius, depth, symmetry, twist, core, mix;
        bool nullCore;
    };

    const Preset presets[] = {
        { "ボーカルを広げる",   0.10f,  75.0f, 45.0f, 100.0f, 4.0f, 10.0f, 30.0f, false },
        { "パッドを回す",       0.18f, 100.0f, 65.0f, 100.0f, 7.0f, 10.0f, 45.0f, false },
        { "ギターに揺らぎ",     0.08f,  80.0f, 60.0f,  88.0f, 6.0f, 15.0f, 40.0f, false },
        { "シンセを速く回す",   0.60f,  90.0f, 70.0f, 100.0f, 8.0f,  0.0f, 40.0f, false },
        { "実験:中心を消す",   0.04f, 100.0f, 50.0f, 100.0f, 8.0f,  0.0f, 30.0f, true  },
    };

    constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

    void setParameter (juce::AudioProcessorValueTreeState& apvts, const char* id, float actualValue)
    {
        if (auto* parameter = apvts.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
    }
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
    for (int i = 0; i < numPresets; ++i)
        presetBox.addItem (jp (presets[i].name), i + 1);
    presetBox.setTooltip (jp("まずここから選ぶのがおすすめです。つまみは後から微調整できます。"));
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0 && index < numPresets)
            applyPreset (index);
    };

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
            [this] (float value) { warningLabel.setVisible (value > 0.5f); },
            nullptr);
        nullCoreWatcher->sendInitialUpdate();
    }

    showPage (processorRef.apvts.state.getProperty ("editorPage", 0));

    setResizable (true, true);
    setResizeLimits (780, 540, 1600, 1100);

    const int savedWidth  = processorRef.apvts.state.getProperty ("editorWidth", 900);
    const int savedHeight = processorRef.apvts.state.getProperty ("editorHeight", 620);
    setSize (juce::jlimit (780, 1600, savedWidth), juce::jlimit (540, 1100, savedHeight));

    startTimerHz (12);
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
    const auto& preset = presets[presetIndex];
    auto& apvts = processorRef.apvts;

    setParameter (apvts, params::rateID, preset.rate);
    setParameter (apvts, params::radiusID, preset.radius);
    setParameter (apvts, params::depthID, preset.depth);
    setParameter (apvts, params::symmetryID, preset.symmetry);
    setParameter (apvts, params::twistID, preset.twist);
    setParameter (apvts, params::coreID, preset.core);
    setParameter (apvts, params::mixID, preset.mix);

    if (auto* nullCore = apvts.getParameter (params::nullCoreID))
        nullCore->setValueNotifyingHost (preset.nullCore ? 1.0f : 0.0f);
}

void DNAOrbitAudioProcessorEditor::showPage (int page)
{
    currentPage = juce::jlimit (0, 1, page);
    processorRef.apvts.state.setProperty ("editorPage", currentPage, nullptr);

    const bool basic = currentPage == 0;

    for (auto* knob : { &rateKnob, &radiusKnob, &depthKnob, &mixKnob })
    {
        knob->slider.setVisible (basic);
        knob->nameLabel.setVisible (basic);
        knob->hintLabel.setVisible (basic);
    }

    for (auto* knob : { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob })
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

    presetBox.setVisible (basic);
    presetLabel.setVisible (basic);

    const auto activeColour = dnaorbit::ui::DnaLookAndFeel::strandAColour().withAlpha (0.35f);
    const auto idleColour = dnaorbit::ui::DnaLookAndFeel::panelColour();
    basicTabButton.setColour (juce::TextButton::buttonColourId, basic ? activeColour : idleColour);
    detailTabButton.setColour (juce::TextButton::buttonColourId, basic ? idleColour : activeColour);

    resized();
    repaint();
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
}

void DNAOrbitAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (dnaorbit::ui::DnaLookAndFeel::backgroundColour());

    // Panel behind the control area.
    auto area = getLocalBounds().reduced (12);
    area.removeFromTop (44);
    const auto controlArea = area.removeFromBottom (128);
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
    processorRef.apvts.state.setProperty ("editorWidth", getWidth(), nullptr);
    processorRef.apvts.state.setProperty ("editorHeight", getHeight(), nullptr);

    auto area = getLocalBounds().reduced (12);

    // --- Top bar ---------------------------------------------------------------
    auto topBar = area.removeFromTop (44);
    auto titleArea = topBar.removeFromLeft (250);
    titleLabel.setBounds (titleArea.removeFromTop (24));
    subtitleLabel.setBounds (titleArea);

    auto tabArea = topBar.removeFromLeft (140).reduced (0, 8);
    basicTabButton.setBounds (tabArea.removeFromLeft (66));
    tabArea.removeFromLeft (4);
    detailTabButton.setBounds (tabArea.removeFromLeft (66));

    auto presetArea = topBar.removeFromRight (300).reduced (0, 9);
    presetLabel.setBounds (presetArea.removeFromLeft (80));
    presetArea.removeFromLeft (6);
    presetBox.setBounds (presetArea);

    // --- Control area ----------------------------------------------------------
    auto controlArea = area.removeFromBottom (128).reduced (8);

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

        layOutKnobRow (controlArea, { &symmetryKnob, &twistKnob, &coreKnob, &outputKnob });
    }

    // --- Centre: readouts | 3D helix ------------------------------------------
    auto centreArea = area;
    auto readoutColumn = centreArea.removeFromLeft (juce::jmin (140, centreArea.getWidth() / 5));

    statusLabel.setBounds (readoutColumn.removeFromTop (24));
    readoutColumn.removeFromTop (6);
    readoutLabel.setBounds (readoutColumn.removeFromTop (90));

    warningLabel.setBounds (centreArea.removeFromBottom (20));
    helixView.setBounds (centreArea.reduced (4));
}
