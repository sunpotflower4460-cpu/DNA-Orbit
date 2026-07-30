#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/HelixEngine.h"

class DNAOrbitAudioProcessor : public juce::AudioProcessor
{
public:
    DNAOrbitAudioProcessor();
    ~DNAOrbitAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    dnaorbit::dsp::HelixEngine& getEngine() noexcept { return engine; }

private:
    dnaorbit::dsp::HelixEngine engine;

    std::atomic<float>* rateHzParam     = nullptr;
    std::atomic<float>* syncParam       = nullptr;
    std::atomic<float>* divisionParam   = nullptr;
    std::atomic<float>* radiusParam     = nullptr;
    std::atomic<float>* depthParam      = nullptr;
    std::atomic<float>* symmetryParam   = nullptr;
    std::atomic<float>* twistParam      = nullptr;
    std::atomic<float>* coreParam       = nullptr;
    std::atomic<float>* nullCoreParam   = nullptr;
    std::atomic<float>* mixParam        = nullptr;
    std::atomic<float>* outputParam     = nullptr;
    std::atomic<float>* autoGainParam   = nullptr;

    float resolveRateHz() const noexcept;
    dnaorbit::dsp::HelixEngine::Parameters currentParameterSnapshot() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessor)
};
