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
            beginTest ("Every factory preset sets all parameters deterministically, regardless of prior state");
            {
                for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
                {
                    const auto& preset = dnaorbit::presets::presets[i];

                    // Start from two wildly different prior states, so a
                    // "deterministic" preset must land in the same place from
                    // either one - this is the whole point of Phase 1's
                    // rewrite (the old preset table left sync/division/
                    // output/autoGain untouched, so the result depended on
                    // whatever the user had before).
                    DNAOrbitAudioProcessor processorA;
                    DNAOrbitAudioProcessor processorB;

                    processorB.apvts.getParameter (dnaorbit::params::syncID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::divisionID)->setValueNotifyingHost (0.9f);
                    processorB.apvts.getParameter (dnaorbit::params::outputID)->setValueNotifyingHost (0.1f);
                    processorB.apvts.getParameter (dnaorbit::params::autoGainID)->setValueNotifyingHost (0.0f);
                    processorB.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.9f);
                    processorB.apvts.getParameter (dnaorbit::params::stereoPreserveID)->setValueNotifyingHost (0.05f);
                    processorB.apvts.getParameter (dnaorbit::params::bassAnchorHzID)->setValueNotifyingHost (0.9f);
                    processorB.apvts.getParameter (dnaorbit::params::characterID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::phaseModeID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::startPhaseID)->setValueNotifyingHost (0.7f);
                    processorB.apvts.getParameter (dnaorbit::params::directionID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::softBypassID)->setValueNotifyingHost (1.0f);
                    processorB.apvts.getParameter (dnaorbit::params::monoPreviewID)->setValueNotifyingHost (1.0f);

                    dnaorbit::presets::apply (processorA.apvts, preset);
                    dnaorbit::presets::apply (processorB.apvts, preset);

                    for (const auto* id : { dnaorbit::params::rateID, dnaorbit::params::syncID,
                                             dnaorbit::params::divisionID, dnaorbit::params::radiusID,
                                             dnaorbit::params::depthID, dnaorbit::params::symmetryID,
                                             dnaorbit::params::twistID, dnaorbit::params::coreID,
                                             dnaorbit::params::nullCoreID, dnaorbit::params::mixID,
                                             dnaorbit::params::outputID, dnaorbit::params::autoGainID,
                                             dnaorbit::params::stereoPreserveID, dnaorbit::params::bassAnchorHzID,
                                             dnaorbit::params::characterID, dnaorbit::params::phaseModeID,
                                             dnaorbit::params::startPhaseID, dnaorbit::params::directionID,
                                             dnaorbit::params::softBypassID, dnaorbit::params::monoPreviewID })
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

                // Nudge Mix far enough that it cannot be mistaken for float round-off.
                processor.apvts.getParameter (dnaorbit::params::mixID)->setValueNotifyingHost (0.99f);
                expect (! dnaorbit::presets::matchesCurrentState (processor.apvts, preset),
                        "Nudging a parameter away from the preset must be detected as modified");

                // Revert = re-applying the same preset.
                dnaorbit::presets::apply (processor.apvts, preset);
                expect (dnaorbit::presets::matchesCurrentState (processor.apvts, preset),
                        "Re-applying the preset (Revert) must restore the matched state");
            }
        }
    };

    static PresetsTests presetsTests;
}
