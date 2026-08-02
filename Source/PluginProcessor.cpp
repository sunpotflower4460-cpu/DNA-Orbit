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
    rateHzParam   = apvts.getRawParameterValue (dnaorbit::params::rateID);
    syncParam     = apvts.getRawParameterValue (dnaorbit::params::syncID);
    divisionParam = apvts.getRawParameterValue (dnaorbit::params::divisionID);
    radiusParam   = apvts.getRawParameterValue (dnaorbit::params::radiusID);
    depthParam    = apvts.getRawParameterValue (dnaorbit::params::depthID);
    symmetryParam = apvts.getRawParameterValue (dnaorbit::params::symmetryID);
    twistParam    = apvts.getRawParameterValue (dnaorbit::params::twistID);
    coreParam     = apvts.getRawParameterValue (dnaorbit::params::coreID);
    nullCoreParam = apvts.getRawParameterValue (dnaorbit::params::nullCoreID);
    mixParam      = apvts.getRawParameterValue (dnaorbit::params::mixID);
    outputParam   = apvts.getRawParameterValue (dnaorbit::params::outputID);
    autoGainParam = apvts.getRawParameterValue (dnaorbit::params::autoGainID);
    stereoPreserveParam = apvts.getRawParameterValue (dnaorbit::params::stereoPreserveID);
    softBypassParam = apvts.getRawParameterValue (dnaorbit::params::softBypassID);
}

void DNAOrbitAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    bypassScratchBuffer.setSize (2, samplesPerBlock, false, false, true);
    engine.primeParameters (currentParameterSnapshot());
}

void DNAOrbitAudioProcessor::releaseResources()
{
    engine.reset();
}

bool DNAOrbitAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto outSet = layouts.getMainOutputChannelSet();
    if (outSet != juce::AudioChannelSet::stereo())
        return false;

    const auto inSet = layouts.getMainInputChannelSet();
    if (inSet != juce::AudioChannelSet::mono() && inSet != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

float DNAOrbitAudioProcessor::resolveRateHz() const noexcept
{
    const bool syncEnabled = syncParam != nullptr && syncParam->load() > 0.5f;
    const float freeRateHz = rateHzParam != nullptr ? rateHzParam->load() : dnaorbit::params::rateDefaultHz;

    if (! syncEnabled)
        return freeRateHz;

    if (auto* currentPlayHead = getPlayHead())
    {
        if (const auto position = currentPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                if (*bpm > 0.0)
                {
                    const int divisionIndex = divisionParam != nullptr ? (int) divisionParam->load() : 2;
                    return (float) dnaorbit::params::syncedRateHz (*bpm, divisionIndex);
                }
            }
        }
    }

    return freeRateHz;
}

dnaorbit::dsp::HelixEngine::Parameters DNAOrbitAudioProcessor::currentParameterSnapshot() const noexcept
{
    dnaorbit::dsp::HelixEngine::Parameters p;
    p.rateHz     = resolveRateHz();
    p.radius01   = radiusParam->load()   / 100.0f;
    p.depth01    = depthParam->load()    / 100.0f;
    p.symmetry01 = symmetryParam->load() / 100.0f;
    p.twistMs    = twistParam->load();
    p.core01     = coreParam->load()     / 100.0f;
    p.nullCore   = nullCoreParam->load() > 0.5f;
    p.mix01      = mixParam->load()      / 100.0f;
    p.outputDb   = outputParam->load();
    p.autoGain   = autoGainParam->load() > 0.5f;
    p.stereoPreserve01 = stereoPreserveParam->load() / 100.0f;
    p.softBypass = softBypassParam->load() > 0.5f;
    return p;
}

void DNAOrbitAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.setParameters (currentParameterSnapshot());
    engine.process (buffer, totalNumInputChannels);
}

void DNAOrbitAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    if (numSamples <= bypassScratchBuffer.getNumSamples())
    {
        for (int ch = 0; ch < 2; ++ch)
            bypassScratchBuffer.copyFrom (ch, 0, buffer, juce::jmin (ch, totalNumInputChannels - 1), 0, numSamples);

        juce::AudioBuffer<float> scratchView (bypassScratchBuffer.getArrayOfWritePointers(), 2, numSamples);
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

    loadedSchemaVersion = xmlState->getIntAttribute (dnaorbit::params::schemaVersionPropertyID,
                                                      dnaorbit::params::legacyUnversionedSchema);

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    migrateLegacyUiState (apvts.state);

    if (loadedSchemaVersion < dnaorbit::params::stereoPreserveIntroducedInSchema)
    {
        if (auto* stereoPreserve = apvts.getParameter (dnaorbit::params::stereoPreserveID))
            stereoPreserve->setValueNotifyingHost (
                stereoPreserve->convertTo0to1 (dnaorbit::params::stereoPreserveLegacyPercent));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DNAOrbitAudioProcessor();
}
