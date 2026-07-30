#include "PluginProcessor.h"
#include "PluginEditor.h"

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
}

void DNAOrbitAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Without this, every SmoothedValue starts this session at its default
    // current value of 0 and only reaches the host's actual settings by
    // ramping toward them once processBlock() calls setParameters() - so
    // Output, Mix, and everything else would audibly fade in from silence
    // over the first smoothing window after every prepareToPlay() (plugin
    // load, sample-rate or buffer-size change).
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

    // Host tempo unavailable: fall back safely to the Free Rate.
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
    return p;
}

void DNAOrbitAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const auto p = currentParameterSnapshot();

    engine.setParameters (p);
    engine.process (buffer, totalNumInputChannels);
}

void DNAOrbitAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // For mono-in/stereo-out, duplicate the input so bypass still yields a
    // sensible stereo signal that matches the input. Stereo-in/stereo-out is
    // already an untouched pass-through.
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, buffer.getNumSamples());
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

    if (xmlState == nullptr)
        return;

    if (! xmlState->hasTagName (apvts.state.getType()))
        return;

    // A project saved before this property existed has no schemaVersion at
    // all - that is, by definition, schema 1 (today's format), so missing
    // defaults to 1 rather than 0. Read before replaceState: the source XML
    // is the ground truth for what was actually saved, not any value already
    // sitting on the live apvts.state.
    loadedSchemaVersion = xmlState->getIntAttribute (dnaorbit::params::schemaVersionPropertyID,
                                                      dnaorbit::params::currentStateSchemaVersion);

    apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DNAOrbitAudioProcessor();
}
