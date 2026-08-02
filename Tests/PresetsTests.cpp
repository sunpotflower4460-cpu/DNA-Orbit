#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "Presets.h"

namespace
{
    class PresetsTests : public juce::UnitTest
    {
    public:
        PresetsTests() : juce::UnitTest ("Presets", "DNAOrbit") {}

        void runTest() override
        {
            using namespace dnaorbit::params;

            beginTest ("Every factory preset sets the complete audio state deterministically");
            {
                for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
                {
                    const auto& preset = dnaorbit::presets::presets[i];
                    DNAOrbitAudioProcessor processorA;
                    DNAOrbitAudioProcessor processorB;

                    auto setB = [&processorB] (const char* id, float actual)
                    {
                        if (auto* parameter = processorB.apvts.getParameter (id))
                            parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
                    };

                    setB (syncID, 1.0f);
                    setB (divisionID, 5.0f);
                    setB (outputID, -9.0f);
                    setB (autoGainID, 0.0f);
                    setB (mixID, 91.0f);
                    setB (stereoPreserveID, 5.0f);
                    setB (softBypassID, 1.0f);
                    setB (bassAnchorID, 420.0f);
                    setB (characterID, (float) characterDeep);
                    setB (phaseModeID, (float) phaseRetrigger);
                    setB (startPhaseID, 247.0f);
                    setB (directionID, (float) counterClockwise);

                    dnaorbit::presets::apply (processorA.apvts, preset);
                    dnaorbit::presets::apply (processorB.apvts, preset);

                    for (const auto* id : {
                             rateID, syncID, divisionID, radiusID, depthID,
                             symmetryID, twistID, coreID, nullCoreID, mixID,
                             outputID, autoGainID, stereoPreserveID, softBypassID,
                             bassAnchorID, characterID, phaseModeID, startPhaseID,
                             directionID })
                    {
                        expectWithinAbsoluteError (
                            processorA.apvts.getRawParameterValue (id)->load(),
                            processorB.apvts.getRawParameterValue (id)->load(),
                            1.0e-3f,
                            juce::String ("Preset ") + preset.name + " must set " + id
                                + " independently of prior state");
                    }
                }
            }

            beginTest ("matchesCurrentState is true immediately after applying a preset");
            {
                DNAOrbitAudioProcessor processor;
                for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
                {
                    const auto& preset = dnaorbit::presets::presets[i];
                    dnaorbit::presets::apply (processor.apvts, preset);
                    expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset));
                }
            }

            beginTest ("Changing a new DSP parameter marks the preset modified and Revert restores it");
            {
                DNAOrbitAudioProcessor processor;
                const auto& preset = dnaorbit::presets::presets[0];
                dnaorbit::presets::apply (processor.apvts, preset);
                expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset));

                auto* bass = processor.apvts.getParameter (bassAnchorID);
                bass->setValueNotifyingHost (bass->convertTo0to1 (350.0f));
                expect (! dnaorbit::presets::matchesCurrentState (processor.apvts, preset));

                dnaorbit::presets::apply (processor.apvts, preset);
                expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset));
            }
        }
    };

    static PresetsTests presetsTests;
}
