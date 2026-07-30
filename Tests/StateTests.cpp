#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "Parameters.h"

namespace
{
    class StateTests : public juce::UnitTest
    {
    public:
        StateTests() : juce::UnitTest ("State", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("All parameters round-trip through save/restore");
            {
                DNAOrbitAudioProcessor processorA;

                // Push every parameter to a distinctive, non-default value.
                processorA.apvts.getParameter (dnaorbit::params::rateID)->setValueNotifyingHost (0.7f);
                processorA.apvts.getParameter (dnaorbit::params::syncID)->setValueNotifyingHost (1.0f);
                processorA.apvts.getParameter (dnaorbit::params::divisionID)->setValueNotifyingHost (0.8f);
                processorA.apvts.getParameter (dnaorbit::params::radiusID)->setValueNotifyingHost (0.33f);
                processorA.apvts.getParameter (dnaorbit::params::depthID)->setValueNotifyingHost (0.66f);
                processorA.apvts.getParameter (dnaorbit::params::symmetryID)->setValueNotifyingHost (0.42f);
                processorA.apvts.getParameter (dnaorbit::params::twistID)->setValueNotifyingHost (0.9f);
                processorA.apvts.getParameter (dnaorbit::params::coreID)->setValueNotifyingHost (0.25f);
                processorA.apvts.getParameter (dnaorbit::params::nullCoreID)->setValueNotifyingHost (1.0f);
                processorA.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.61f);
                processorA.apvts.getParameter (dnaorbit::params::outputID)->setValueNotifyingHost (0.15f);

                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                DNAOrbitAudioProcessor processorB;
                processorB.setStateInformation (savedState.getData(), (int) savedState.getSize());

                for (auto* param : processorA.apvts.processor.getParameters())
                {
                    auto* rangedA = dynamic_cast<juce::RangedAudioParameter*> (param);
                    if (rangedA == nullptr)
                        continue;

                    auto* rangedB = processorB.apvts.getParameter (rangedA->paramID);
                    expect (rangedB != nullptr, "Restored processor must have parameter " + rangedA->paramID);

                    if (rangedB != nullptr)
                        expectWithinAbsoluteError (rangedB->getValue(), rangedA->getValue(), 1.0e-4f,
                                                    "Parameter " + rangedA->paramID + " must round-trip exactly");
                }
            }

            beginTest ("Invalid or empty state data does not crash setStateInformation");
            {
                DNAOrbitAudioProcessor processor;

                processor.setStateInformation (nullptr, 0);

                const char garbage[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
                processor.setStateInformation (garbage, (int) sizeof (garbage));

                juce::MemoryBlock empty;
                processor.setStateInformation (empty.getData(), 0);

                // If we got here without crashing, and the processor is still usable, we're good.
                processor.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.5f);
                expectWithinAbsoluteError (processor.apvts.getParameter (dnaorbit::params::mixID)->getValue(), 0.5f, 1.0e-5f);
            }

            beginTest ("Reloading a project (save -> restore into the same instance) does not change audible defaults");
            {
                DNAOrbitAudioProcessor processor;
                processor.prepareToPlay (48000.0, 256);

                juce::MemoryBlock savedState;
                processor.getStateInformation (savedState);
                processor.setStateInformation (savedState.getData(), (int) savedState.getSize());

                expectWithinAbsoluteError (processor.apvts.getRawParameterValue (dnaorbit::params::mixID)->load(), 35.0f, 1.0e-3f);
                expectWithinAbsoluteError (processor.apvts.getRawParameterValue (dnaorbit::params::symmetryID)->load(), 100.0f, 1.0e-3f);

                processor.releaseResources();
            }

            beginTest ("Bus layouts: mono-in/stereo-out and stereo-in/stereo-out are supported; others are rejected");
            {
                DNAOrbitAudioProcessor processor;

                auto makeLayout = [] (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
                {
                    juce::AudioProcessor::BusesLayout layout;
                    layout.inputBuses.add (in);
                    layout.outputBuses.add (out);
                    return layout;
                };

                expect (processor.isBusesLayoutSupported (
                    makeLayout (juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo())));
                expect (processor.isBusesLayoutSupported (
                    makeLayout (juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo())));
                expect (! processor.isBusesLayoutSupported (
                    makeLayout (juce::AudioChannelSet::stereo(), juce::AudioChannelSet::mono())));
                expect (! processor.isBusesLayoutSupported (
                    makeLayout (juce::AudioChannelSet::create5point1(), juce::AudioChannelSet::stereo())));
            }

            beginTest ("Bypass leaves stereo-in/stereo-out input unchanged");
            {
                DNAOrbitAudioProcessor processor;
                processor.prepareToPlay (48000.0, 256);

                juce::AudioBuffer<float> buffer (2, 256);
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < 256; ++n)
                        data[n] = 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / 48000.0);
                }

                juce::AudioBuffer<float> original;
                original.makeCopyOf (buffer);

                juce::MidiBuffer midi;
                processor.processBlockBypassed (buffer, midi);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 256; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), original.getSample (ch, n), 1.0e-6f);

                processor.releaseResources();
            }

            beginTest ("Bypass also handles mono-in/stereo-out by duplicating the mono input");
            {
                // isBusesLayoutSupported explicitly allows mono-in/stereo-out, but
                // the only bypass test above only ever exercised the trivial
                // stereo-in/stereo-out case. processBlockBypassed's mono
                // duplication logic (PluginProcessor.cpp) needs its own coverage.
                //
                // A freshly constructed processor defaults to stereo-in/stereo-out
                // (see BusesProperties() in the constructor), so the bus layout
                // must actually be switched to mono-in first - getTotalNumInputChannels()
                // reflects the negotiated layout, not the buffer's raw size.
                DNAOrbitAudioProcessor processor;

                juce::AudioProcessor::BusesLayout monoInStereoOut;
                monoInStereoOut.inputBuses.add (juce::AudioChannelSet::mono());
                monoInStereoOut.outputBuses.add (juce::AudioChannelSet::stereo());
                expect (processor.setBusesLayout (monoInStereoOut), "Test setup: must be able to switch to mono-in/stereo-out");

                processor.prepareToPlay (48000.0, 256);

                juce::AudioBuffer<float> buffer (2, 256); // ch1 simulates "not part of input"
                auto* mono = buffer.getWritePointer (0);
                for (int n = 0; n < 256; ++n)
                    mono[n] = 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / 48000.0);
                buffer.clear (1, 0, 256); // what the host leaves in the "extra" channel

                juce::MidiBuffer midi;
                processor.processBlockBypassed (buffer, midi);

                for (int n = 0; n < 256; ++n)
                    expectWithinAbsoluteError (buffer.getSample (1, n), buffer.getSample (0, n), 1.0e-6f,
                                                "Bypassed mono-in/stereo-out must duplicate the input to both channels");

                processor.releaseResources();
            }

            beginTest ("A saved state from an older plugin version (missing a parameter) loads without crashing");
            {
                // Simulates a real scenario: a user's project was saved before
                // autoGain existed, then they update the plugin. APVTS must fall
                // back to that parameter's declared default rather than crashing
                // or leaving it uninitialised.
                DNAOrbitAudioProcessor processorA;
                processorA.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.61f);
                processorA.apvts.getParameter (dnaorbit::params::autoGainID)->setValueNotifyingHost (0.0f); // non-default, so a wrongly-kept value would be detectable

                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    savedState.getData(), (int) savedState.getSize()));
                expect (xml != nullptr, "Test setup: state must parse as XML");

                if (xml != nullptr)
                {
                    juce::XmlElement* autoGainNode = nullptr;
                    for (auto* child : xml->getChildIterator())
                    {
                        if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == dnaorbit::params::autoGainID)
                        {
                            autoGainNode = child;
                            break;
                        }
                    }

                    expect (autoGainNode != nullptr, "Test setup: autoGain PARAM node must exist to remove");
                    if (autoGainNode != nullptr)
                        xml->removeChildElement (autoGainNode, true);

                    juce::MemoryBlock trimmedState;
                    juce::AudioProcessor::copyXmlToBinary (*xml, trimmedState);

                    DNAOrbitAudioProcessor processorB;
                    processorB.setStateInformation (trimmedState.getData(), (int) trimmedState.getSize());

                    // Falls back to the parameter's declared default (true), not a
                    // crash and not a leftover zero-initialised value.
                    expect (processorB.apvts.getRawParameterValue (dnaorbit::params::autoGainID)->load() > 0.5f,
                            "Missing autoGain node must fall back to its declared default (on)");

                    // Every other parameter must still load normally.
                    expectWithinAbsoluteError (processorB.apvts.getRawParameterValue (dnaorbit::params::mixID)->load(),
                                                61.0f, 1.0e-2f);
                }
            }

            beginTest ("getStateInformation before prepareToPlay does not crash");
            {
                DNAOrbitAudioProcessor processor;
                juce::MemoryBlock state;
                processor.getStateInformation (state);
                expectGreaterThan ((int) state.getSize(), 0);
            }

            beginTest ("Repeated prepareToPlay (sample-rate change mid-playback) does not crash and keeps processing");
            {
                DNAOrbitAudioProcessor processor;
                processor.prepareToPlay (44100.0, 256);

                juce::AudioBuffer<float> buffer (2, 256);
                juce::MidiBuffer midi;
                buffer.clear();
                processor.processBlock (buffer, midi);

                // A DAW changing sample rate or block size mid-session calls
                // prepareToPlay again without an intervening releaseResources.
                processor.prepareToPlay (96000.0, 512);

                juce::AudioBuffer<float> buffer2 (2, 512);
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer2.getWritePointer (ch);
                    for (int n = 0; n < 512; ++n)
                        data[n] = 0.3f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / 96000.0);
                }

                processor.processBlock (buffer2, midi);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 512; ++n)
                        expect (std::isfinite (buffer2.getSample (ch, n)));

                processor.releaseResources();
            }

            beginTest ("A fresh instance with nothing loaded reports the current schema version");
            {
                DNAOrbitAudioProcessor processor;
                expect (processor.getLoadedSchemaVersion() == dnaorbit::params::currentStateSchemaVersion,
                        "A never-loaded instance has nothing to migrate, so it should report the current version");
            }

            beginTest ("Saving writes the current schema version; loading it back reports the same version");
            {
                DNAOrbitAudioProcessor processorA;
                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                DNAOrbitAudioProcessor processorB;
                processorB.setStateInformation (savedState.getData(), (int) savedState.getSize());

                expect (processorB.getLoadedSchemaVersion() == dnaorbit::params::currentStateSchemaVersion,
                        "Loading a just-saved state must report the version that was actually saved");
            }

            beginTest ("A state saved before schema versioning existed is treated as schema 1, not crashing or defaulting to 0");
            {
                // Simulates every real project saved by a build before this
                // property existed: its XML has no dnaOrbitSchemaVersion
                // attribute at all.
                DNAOrbitAudioProcessor processorA;
                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    savedState.getData(), (int) savedState.getSize()));
                expect (xml != nullptr);

                if (xml != nullptr)
                {
                    xml->removeAttribute (dnaorbit::params::schemaVersionPropertyID);

                    juce::MemoryBlock unversionedState;
                    juce::AudioProcessor::copyXmlToBinary (*xml, unversionedState);

                    DNAOrbitAudioProcessor processorB;
                    processorB.setStateInformation (unversionedState.getData(), (int) unversionedState.getSize());

                    expect (processorB.getLoadedSchemaVersion() == 1,
                            "A state with no version attribute predates versioning, which is schema 1 by definition");
                }
            }

            beginTest ("Bypass keeps the engine's orbit phase advancing instead of freezing it");
            {
                DNAOrbitAudioProcessor processor;
                processor.prepareToPlay (48000.0, 256);

                juce::AudioBuffer<float> buffer (2, 256);
                buffer.clear();
                juce::MidiBuffer midi;

                processor.processBlockBypassed (buffer, midi);
                const float thetaAfterFirst = processor.getEngine().getVisualState().thetaA;

                processor.processBlockBypassed (buffer, midi);
                const float thetaAfterSecond = processor.getEngine().getVisualState().thetaA;

                expect (std::abs (thetaAfterSecond - thetaAfterFirst) > 1.0e-5f,
                        "Orbit phase must keep advancing across bypassed blocks, not freeze at its pre-bypass value");

                processor.releaseResources();
            }

            beginTest ("Bypass with a block larger than prepareToPlay negotiated falls back safely, still dry");
            {
                // A host handing us a bigger block than it negotiated is a
                // contract violation, but the fallback (skip the state
                // advance, keep the dry passthrough) must never crash or
                // touch the audible signal.
                DNAOrbitAudioProcessor processor;
                processor.prepareToPlay (48000.0, 256);

                juce::AudioBuffer<float> buffer (2, 512);
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < 512; ++n)
                        data[n] = 0.4f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * n / 48000.0);
                }

                juce::AudioBuffer<float> original;
                original.makeCopyOf (buffer);

                juce::MidiBuffer midi;
                processor.processBlockBypassed (buffer, midi);

                for (int ch = 0; ch < 2; ++ch)
                    for (int n = 0; n < 512; ++n)
                        expectWithinAbsoluteError (buffer.getSample (ch, n), original.getSample (ch, n), 1.0e-6f);

                processor.releaseResources();
            }

            beginTest ("A pre-Phase-1 save with flat editorPage/Width/Height properties migrates into the uiState child node");
            {
                // Every project saved before editor state got its own child
                // node has these three directly on the root - simulate that
                // exact shape rather than assuming today's getStateInformation
                // already writes the new layout.
                DNAOrbitAudioProcessor processorA;
                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    savedState.getData(), (int) savedState.getSize()));
                expect (xml != nullptr);

                if (xml != nullptr)
                {
                    xml->setAttribute ("editorPage", 1);
                    xml->setAttribute ("editorWidth", 1234);
                    xml->setAttribute ("editorHeight", 789);

                    juce::MemoryBlock legacyState;
                    juce::AudioProcessor::copyXmlToBinary (*xml, legacyState);

                    DNAOrbitAudioProcessor processorB;
                    processorB.setStateInformation (legacyState.getData(), (int) legacyState.getSize());

                    expect (! processorB.apvts.state.hasProperty ("editorPage"),
                            "Legacy flat editorPage must be removed from the root after migration");
                    expect (! processorB.apvts.state.hasProperty ("editorWidth"),
                            "Legacy flat editorWidth must be removed from the root after migration");
                    expect (! processorB.apvts.state.hasProperty ("editorHeight"),
                            "Legacy flat editorHeight must be removed from the root after migration");

                    auto uiState = processorB.apvts.state.getChildWithName ("uiState");
                    expect (uiState.isValid(), "Migration must create the uiState child node");
                    expect ((int) uiState.getProperty ("editorPage", -1) == 1,
                            "editorPage must survive the migration with its saved value");
                    expect ((int) uiState.getProperty ("editorWidth", -1) == 1234,
                            "editorWidth must survive the migration with its saved value");
                    expect ((int) uiState.getProperty ("editorHeight", -1) == 789,
                            "editorHeight must survive the migration with its saved value");
                }
            }

            beginTest ("A save with no legacy UI properties at all still gets a uiState node without crashing");
            {
                DNAOrbitAudioProcessor processorA;
                juce::MemoryBlock savedState;
                processorA.getStateInformation (savedState);

                DNAOrbitAudioProcessor processorB;
                processorB.setStateInformation (savedState.getData(), (int) savedState.getSize());

                expect (processorB.apvts.state.getChildWithName ("uiState").isValid());
            }
        }
    };

    static StateTests stateTests;
}
