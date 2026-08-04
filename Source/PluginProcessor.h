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
    /**
     * How long the plugin keeps producing audible output after its input
     * goes silent. Hosts use this to decide how long to keep pulling audio
     * on an offline bounce or freeze, so under-reporting truncates the tail.
     *
     * 0.05 was correct for the original engine (max back-delay 8ms + max
     * Twist 20ms), but Bass Anchor made it wrong: a 4th-order
     * Linkwitz-Riley crossover just above the 20Hz "Off" threshold rings
     * for 68-73ms, well past 50ms. Measured worst case across 44.1/48/96/
     * 192kHz is pinned by Tests/TailLengthTests.cpp, which fails if a future
     * change (a longer Character delay, a lower Bass Anchor minimum) pushes
     * the real tail past what is declared here. See ADR-013.
     */
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Declared before apvts (construction order follows declaration order):
    // apvts takes a pointer to this so every parameter change becomes a
    // single undoable transaction, giving the editor's Undo/Redo buttons
    // host-independent Ctrl+Z/Ctrl+Shift+Z without any extra bookkeeping.
    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    dnaorbit::dsp::HelixEngine& getEngine() noexcept { return engine; }

    /**
     * Schema version the currently-loaded state was saved under (see
     * Parameters::currentStateSchemaVersion). A fresh instance with nothing
     * loaded reports the current version, i.e. "nothing to migrate". Future
     * parameters that need a different default for state saved before they
     * existed (e.g. Stereo Preserve in a later phase) should compare against
     * this rather than re-deriving version logic themselves.
     */
    int getLoadedSchemaVersion() const noexcept { return loadedSchemaVersion; }

private:
    dnaorbit::dsp::HelixEngine engine;

    /**
     * Scratch space for processBlockBypassed(): sized once in prepareToPlay()
     * so the audio thread never allocates. Lets the engine's internal state
     * (orbit phase, smoothers, filters) keep advancing while bypassed without
     * touching the audible dry passthrough - see processBlockBypassed().
     */
    juce::AudioBuffer<float> bypassScratchBuffer;

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
    std::atomic<float>* stereoPreserveParam = nullptr;
    std::atomic<float>* bassAnchorHzParam   = nullptr;
    std::atomic<float>* characterParam      = nullptr;
    std::atomic<float>* phaseModeParam      = nullptr;
    std::atomic<float>* startPhaseParam     = nullptr;
    std::atomic<float>* directionParam      = nullptr;
    std::atomic<float>* softBypassParam     = nullptr;
    std::atomic<float>* monoPreviewParam    = nullptr;

    int loadedSchemaVersion = dnaorbit::params::currentStateSchemaVersion;

    float resolveRateHz() const noexcept;
    dnaorbit::dsp::HelixEngine::Parameters currentParameterSnapshot() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DNAOrbitAudioProcessor)
};
