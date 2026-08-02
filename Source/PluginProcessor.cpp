#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    void migrateLegacyUiState (juce::ValueTree& root)
    {
        using namespace dnaorbit::params;
        auto uiState = root.getOrCreateChildWithName (uiStateNodeID, nullptr);

        for (const auto* legacyID : { editorPagePropertyID, editorWidthPropertyID, editorHeightPropertyID })
        {
            if (root.hasProperty (legacyID))
            {
                if (! uiState.hasProperty (legacyID))
                    uiState.setProperty (legacyID, root.getProperty (legacyID), nullptr);
                root.removeProperty (legacyID, nullptr);
            }
        }
    }
}

DNAOrbitAudioProcessor::DNAOrbitAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", dnaorbit::params::createParameterLayout())
{
    using namespace dnaorbit::params;
    rateHzParam = apvts.getRawParameterValue (rateID);
    syncParam = apvts.getRawParameterValue (syncID);
    divisionParam = apvts.getRawParameterValue (divisionID);
    radiusParam = apvts.getRawParameterValue (radiusID);
    depthParam = apvts.getRawParameterValue (depthID);
    symmetryParam = apvts.getRawParameterValue (symmetryID);
    twistParam = apvts.getRawParameterValue (twistID);
    coreParam = apvts.getRawParameterValue (coreID);
    nullCoreParam = apvts.getRawParameterValue (nullCoreID);
    mixParam = apvts.getRawParameterValue (mixID);
    outputParam = apvts.getRawParameterValue (outputID);
    autoGainParam = apvts.getRawParameterValue (autoGainID);
    stereoPreserveParam = apvts.getRawParameterValue (stereoPreserveID);
    softBypassParam = apvts.getRawParameterValue (softBypassID);
    bassAnchorParam = apvts.getRawParameterValue (bassAnchorID);
    characterParam = apvts.getRawParameterValue (characterID);
    phaseModeParam = apvts.getRawParameterValue (phaseModeID);
    startPhaseParam = apvts.getRawParameterValue (startPhaseID);
    directionParam = apvts.getRawParameterValue (directionID);
}

void DNAOrbitAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    previousTransportPlaying = false;
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    bypassScratchBuffer.setSize (2, samplesPerBlock, false, false, true);
    engine.primeParameters (currentParameterSnapshot());
}

void DNAOrbitAudioProcessor::releaseResources()
{
    previousTransportPlaying = false;
    engine.reset();
}

bool DNAOrbitAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto outSet = layouts.getMainOutputChannelSet();
    if (outSet != juce::AudioChannelSet::stereo())
        return false;

    const auto inSet = layouts.getMainInputChannelSet();
    return inSet == juce::AudioChannelSet::mono()
        || inSet == juce::AudioChannelSet::stereo();
}

dnaorbit::dsp::HelixEngine::Parameters DNAOrbitAudioProcessor::currentParameterSnapshot() noexcept
{
    using namespace dnaorbit::params;

    dnaorbit::dsp::HelixEngine::Parameters p;
    p.rateHz = rateHzParam != nullptr ? rateHzParam->load() : rateDefaultHz;
    p.radius01 = radiusParam->load() / 100.0f;
    p.depth01 = depthParam->load() / 100.0f;
    p.symmetry01 = symmetryParam->load() / 100.0f;
    p.twistMs = twistParam->load();
    p.core01 = coreParam->load() / 100.0f;
    p.nullCore = nullCoreParam->load() > 0.5f;
    p.mix01 = mixParam->load() / 100.0f;
    p.outputDb = outputParam->load();
    p.autoGain = autoGainParam->load() > 0.5f;
    p.stereoPreserve01 = stereoPreserveParam->load() / 100.0f;
    p.softBypass = softBypassParam->load() > 0.5f;
    p.bassAnchorHz = bassAnchorParam->load();
    p.character = (int) characterParam->load();
    p.startPhaseDegrees = startPhaseParam->load();
    p.reverseDirection = directionParam->load() > 0.5f;

    const bool syncEnabled = syncParam != nullptr && syncParam->load() > 0.5f;
    const int divisionIndex = divisionParam != nullptr ? (int) divisionParam->load() : 2;
    p.phaseMode = syncEnabled ? (int) phaseModeParam->load() : phaseFree;

    int numerator = 4;
    int denominator = 4;
    double bpm = 0.0;

    if (auto* currentPlayHead = getPlayHead())
    {
        if (const auto position = currentPlayHead->getPosition())
        {
            p.transportPlaying = position->getIsPlaying();

            if (const auto hostBpm = position->getBpm())
                bpm = *hostBpm;

            if (const auto signature = position->getTimeSignature())
            {
                numerator = signature->numerator;
                denominator = signature->denominator;
            }

            if (const auto ppq = position->getPpqPosition())
            {
                p.hostPositionValid = std::isfinite (*ppq);
                p.hostPpqPosition = p.hostPositionValid ? *ppq : 0.0;
            }
        }
    }

    p.transportJustStarted = p.transportPlaying && ! previousTransportPlaying;
    previousTransportPlaying = p.transportPlaying;
    p.cycleBeats = divisionIndexToBeats (divisionIndex, numerator, denominator);

    if (syncEnabled && bpm > 0.0)
        p.rateHz = (float) syncedRateHz (bpm, divisionIndex, numerator, denominator);

    // Host Lock requires both a valid song position and a valid tempo-derived
    // cycle. If either is unavailable the engine falls back to Free safely.
    if (! syncEnabled || ! p.hostPositionValid || bpm <= 0.0)
    {
        if (p.phaseMode == phaseHostLock)
            p.phaseMode = phaseFree;
    }

    return p;
}

void DNAOrbitAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setParameters (currentParameterSnapshot());
    engine.process (buffer, totalNumInputChannels);
}

void DNAOrbitAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    if (numSamples <= bypassScratchBuffer.getNumSamples())
    {
        for (int ch = 0; ch < 2; ++ch)
            bypassScratchBuffer.copyFrom (ch, 0, buffer,
                                          juce::jmin (ch, totalNumInputChannels - 1),
                                          0, numSamples);

        juce::AudioBuffer<float> scratchView (bypassScratchBuffer.getArrayOfWritePointers(),
                                              2, numSamples);
        engine.setParameters (currentParameterSnapshot());
        engine.process (scratchView, totalNumInputChannels);
    }

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

juce::AudioProcessorEditor* DNAOrbitAudioProcessor::createEditor()
{
    return new DNAOrbitAudioProcessorEditor (*this);
}

void DNAOrbitAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (dnaorbit::params::schemaVersionPropertyID,
                       dnaorbit::params::currentStateSchemaVersion, nullptr);
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DNAOrbitAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr || ! xmlState->hasTagName (apvts.state.getType()))
        return;

    using namespace dnaorbit::params;
    loadedSchemaVersion = xmlState->getIntAttribute (schemaVersionPropertyID,
                                                      legacyUnversionedSchema);

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    migrateLegacyUiState (apvts.state);

    auto setActual = [this] (const char* id, float value)
    {
        if (auto* parameter = apvts.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    };

    if (loadedSchemaVersion < stereoPreserveIntroducedInSchema)
        setActual (stereoPreserveID, stereoPreserveLegacyPercent);

    if (loadedSchemaVersion < musicalDspIntroducedInSchema)
    {
        setActual (bassAnchorID, bassAnchorLegacyHz);
        setActual (characterID, (float) characterNatural);
        setActual (phaseModeID, (float) phaseFree);
        setActual (startPhaseID, 0.0f);
        setActual (directionID, (float) clockwise);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DNAOrbitAudioProcessor();
}
