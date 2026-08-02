#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "Parameters.h"

namespace
{
    class CommercialParameterStateTests : public juce::UnitTest
    {
    public:
        CommercialParameterStateTests()
            : juce::UnitTest ("CommercialParameterState", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("Stereo Preserve and Soft Bypass restore non-default values");
            {
                DNAOrbitAudioProcessor source;

                auto* preserve = source.apvts.getParameter (dnaorbit::params::stereoPreserveID);
                auto* bypass = source.apvts.getParameter (dnaorbit::params::softBypassID);
                expect (preserve != nullptr && bypass != nullptr, "Test setup: commercial parameters must exist");

                if (preserve == nullptr || bypass == nullptr)
                    return;

                preserve->setValueNotifyingHost (preserve->convertTo0to1 (83.0f));
                bypass->setValueNotifyingHost (1.0f);

                juce::MemoryBlock state;
                source.getStateInformation (state);

                DNAOrbitAudioProcessor restored;
                restored.setStateInformation (state.getData(), (int) state.getSize());

                expectWithinAbsoluteError (
                    restored.apvts.getRawParameterValue (dnaorbit::params::stereoPreserveID)->load(),
                    83.0f, 0.01f, "Stereo Preserve must restore its actual value");
                expect (restored.apvts.getRawParameterValue (dnaorbit::params::softBypassID)->load() > 0.5f,
                        "Soft Bypass ON must survive project save/restore");
            }

            beginTest ("A state missing Soft Bypass falls back safely to Off");
            {
                DNAOrbitAudioProcessor source;
                source.apvts.getParameter (dnaorbit::params::softBypassID)->setValueNotifyingHost (1.0f);

                juce::MemoryBlock state;
                source.getStateInformation (state);
                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    state.getData(), (int) state.getSize()));
                expect (xml != nullptr, "Test setup: saved state must parse");

                if (xml == nullptr)
                    return;

                juce::XmlElement* softBypassNode = nullptr;
                for (auto* child : xml->getChildIterator())
                {
                    if (child->hasTagName ("PARAM")
                        && child->getStringAttribute ("id") == dnaorbit::params::softBypassID)
                    {
                        softBypassNode = child;
                        break;
                    }
                }

                expect (softBypassNode != nullptr, "Test setup: Soft Bypass node must exist");
                if (softBypassNode != nullptr)
                    xml->removeChildElement (softBypassNode, true);

                juce::MemoryBlock oldState;
                juce::AudioProcessor::copyXmlToBinary (*xml, oldState);

                DNAOrbitAudioProcessor restored;
                restored.setStateInformation (oldState.getData(), (int) oldState.getSize());
                expect (restored.apvts.getRawParameterValue (dnaorbit::params::softBypassID)->load() < 0.5f,
                        "A project saved before Soft Bypass existed must default safely to Off");
            }
        }
    };

    static CommercialParameterStateTests commercialParameterStateTests;
}
