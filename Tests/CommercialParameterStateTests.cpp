#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "Parameters.h"

namespace
{
    void removeParameterNode (juce::XmlElement& xml, const char* id)
    {
        juce::XmlElement* found = nullptr;
        for (auto* child : xml.getChildIterator())
        {
            if (child->hasTagName ("PARAM") && child->getStringAttribute ("id") == id)
            {
                found = child;
                break;
            }
        }
        if (found != nullptr)
            xml.removeChildElement (found, true);
    }

    class CommercialParameterStateTests : public juce::UnitTest
    {
    public:
        CommercialParameterStateTests()
            : juce::UnitTest ("CommercialParameterState", "DNAOrbit") {}

        void runTest() override
        {
            using namespace dnaorbit::params;

            beginTest ("Commercial parameters restore non-default values");
            {
                DNAOrbitAudioProcessor source;

                auto set = [&source] (const char* id, float actual)
                {
                    auto* parameter = source.apvts.getParameter (id);
                    if (parameter != nullptr)
                        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
                };

                set (stereoPreserveID, 83.0f);
                set (softBypassID, 1.0f);
                set (bassAnchorID, 187.0f);
                set (characterID, (float) characterDeep);
                set (phaseModeID, (float) phaseRetrigger);
                set (startPhaseID, 123.0f);
                set (directionID, (float) counterClockwise);

                juce::MemoryBlock state;
                source.getStateInformation (state);

                DNAOrbitAudioProcessor restored;
                restored.setStateInformation (state.getData(), (int) state.getSize());

                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (stereoPreserveID)->load(), 83.0f, 0.01f);
                expect (restored.apvts.getRawParameterValue (softBypassID)->load() > 0.5f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (bassAnchorID)->load(), 187.0f, 0.1f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (characterID)->load(),
                                           (float) characterDeep, 0.01f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (phaseModeID)->load(),
                                           (float) phaseRetrigger, 0.01f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (startPhaseID)->load(), 123.0f, 0.1f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (directionID)->load(),
                                           (float) counterClockwise, 0.01f);
            }

            beginTest ("A state missing Soft Bypass falls back safely to Off");
            {
                DNAOrbitAudioProcessor source;
                source.apvts.getParameter (softBypassID)->setValueNotifyingHost (1.0f);

                juce::MemoryBlock state;
                source.getStateInformation (state);
                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    state.getData(), (int) state.getSize()));
                expect (xml != nullptr);
                if (xml == nullptr)
                    return;

                removeParameterNode (*xml, softBypassID);
                juce::MemoryBlock oldState;
                juce::AudioProcessor::copyXmlToBinary (*xml, oldState);

                DNAOrbitAudioProcessor restored;
                restored.setStateInformation (oldState.getData(), (int) oldState.getSize());
                expect (restored.apvts.getRawParameterValue (softBypassID)->load() < 0.5f);
            }

            beginTest ("Fresh schema-3 instances use musical defaults");
            {
                DNAOrbitAudioProcessor processor;
                expectWithinAbsoluteError (processor.apvts.getRawParameterValue (bassAnchorID)->load(),
                                           bassAnchorDefaultHz, 0.01f);
                expectWithinAbsoluteError (processor.apvts.getRawParameterValue (characterID)->load(),
                                           (float) characterNatural, 0.01f);
                expectWithinAbsoluteError (processor.apvts.getRawParameterValue (phaseModeID)->load(),
                                           (float) phaseHostLock, 0.01f);
            }

            beginTest ("Schema-2 projects keep their previous sound and free-running phase");
            {
                DNAOrbitAudioProcessor source;
                juce::MemoryBlock state;
                source.getStateInformation (state);
                std::unique_ptr<juce::XmlElement> xml (juce::AudioProcessor::getXmlFromBinary (
                    state.getData(), (int) state.getSize()));
                expect (xml != nullptr);
                if (xml == nullptr)
                    return;

                xml->setAttribute (schemaVersionPropertyID, 2);
                for (const auto* id : { bassAnchorID, characterID, phaseModeID,
                                        startPhaseID, directionID })
                    removeParameterNode (*xml, id);

                juce::MemoryBlock schema2State;
                juce::AudioProcessor::copyXmlToBinary (*xml, schema2State);

                DNAOrbitAudioProcessor restored;
                restored.setStateInformation (schema2State.getData(), (int) schema2State.getSize());

                expect (restored.getLoadedSchemaVersion() == 2);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (bassAnchorID)->load(),
                                           bassAnchorLegacyHz, 0.01f,
                                           "Old projects must load with Bass Anchor effectively off");
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (characterID)->load(),
                                           (float) characterNatural, 0.01f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (phaseModeID)->load(),
                                           (float) phaseFree, 0.01f,
                                           "Old tempo-sync projects must retain free phase behaviour");
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (startPhaseID)->load(), 0.0f, 0.01f);
                expectWithinAbsoluteError (restored.apvts.getRawParameterValue (directionID)->load(),
                                           (float) clockwise, 0.01f);
            }
        }
    };

    static CommercialParameterStateTests commercialParameterStateTests;
}
