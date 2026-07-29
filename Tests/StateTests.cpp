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
        }
    };

    static StateTests stateTests;
}
