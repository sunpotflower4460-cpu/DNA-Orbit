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
            beginTest ("Every factory preset sets the complete audio state deterministically");
            {
                for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
                {
                    const auto& preset = dnaorbit::presets::presets[i];
                    DNAOrbitAudioProcessor processorA;
                    DNAOrbitAudioProcessor processorB;

                    processorB.apvts.getParameter (dnaorbit::params::syncID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::divisionID)->setValueNotifyingHost (0.9f);
                    processorB.apvts.getParameter (dnaorbit::params::outputID)->setValueNotifyingHost (0.1f);
                    processorB.apvts.getParameter (dnaorbit::params::autoGainID)->setValueNotifyingHost (0.0f);
                    processorB.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.9f);
                    processorB.apvts.getParameter (dnaorbit::params::stereoPreserveID)->setValueNotifyingHost (0.05f);
                    processorB.apvts.getParameter (dnaorbit::params::softBypassID)->setValueNotifyingHost (1.0f);

                    dnaorbit::presets::apply (processorA.apvts, preset);
                    dnaorbit::presets::apply (processorB.apvts, preset);

                    for (const auto* id : { dnaorbit::params::rateID, dnaorbit::params::syncID,
                                             dnaorbit::params::divisionID, dnaorbit::params::radiusID,
                                             dnaorbit::params::depthID, dnaorbit::params::symmetryID,
                                             dnaorbit::params::twistID, dnaorbit::params::coreID,
                                             dnaorbit::params::nullCoreID, dnaorbit::params::mixID,
                                             dnaorbit::params::outputID, dnaorbit::params::autoGainID,
                                             dnaorbit::params::stereoPreserveID, dnaorbit::params::softBypassID })
                    {
                        expectWithinAbsoluteError (processorA.apvts.getRawParameterValue (id)->load(),
                                                    processorB.apvts.getRawParameterValue (id)->load(), 1.0e-3f,
                                                    juce::String ("Preset ") + preset.name + " must set " + id
                                                        + " the same way regardless of prior state");
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

                    expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset),
                            juce::String ("Preset ") + preset.name + " must match its own state right after being applied");
                }
            }

            beginTest ("matchesCurrentState goes false once a parameter is nudged, and Revert restores it");
            {
                DNAOrbitAudioProcessor processor;
                const auto& preset = dnaorbit::presets::presets[0];
                dnaorbit::presets::apply (processor.apvts, preset);
                expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset));

                processor.apvts.getParameter (dnaorbit::params::softBypassID)->setValueNotifyingHost (1.0f);
                expect (! dnaorbit::presets::matchesCurrentState (processor.apvts, preset),
                        "Nudging Soft Bypass away from the preset must be detected as modified");

                dnaorbit::presets::apply (processor.apvts, preset);
                expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset),
                        "Re-applying the preset must restore the matched state");
            }
        }
    };

    static PresetsTests presetsTests;
}
