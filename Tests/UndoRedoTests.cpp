#include <juce_core/juce_core.h>
#include "PluginProcessor.h"
#include "Presets.h"

using namespace dnaorbit;

namespace
{
    // AudioProcessorValueTreeState mirrors parameter changes into its
    // ValueTree - the thing the UndoManager actually records - on an
    // internal ~10Hz juce::Timer (AudioProcessorValueTreeState::
    // timerCallback() in JUCE), not synchronously with
    // setValueNotifyingHost(). Undoing/redoing a ValueTree property change
    // itself IS synchronous (ValueTree::Listener fires immediately, which
    // is what pushes the reverted value back into the live parameter), but
    // there is no way to force that timer to fire early from outside APVTS,
    // and JUCE_MODAL_LOOPS_PERMITTED is off for plugin targets, so this
    // test harness cannot pump a message loop to wait for it either.
    //
    // These tests therefore drive the same ValueTree node the timer would
    // eventually write to, directly - exercising the exact mechanism Undo/
    // Redo depends on (a "PARAM" child of apvts.state with "id"/"value"
    // properties; the same schema Tests/StateTests.cpp already relies on
    // via the XML round trip) without needing the timer. What this does
    // NOT cover is the end-to-end path from a real UI drag through to a
    // flushed Undo step, which needs a real host/message loop - see
    // MANUAL_REQUIRED.md.
    juce::ValueTree findParamNode (juce::AudioProcessorValueTreeState& apvts, const char* paramID)
    {
        return apvts.state.getChildWithProperty (juce::Identifier ("id"), juce::String (paramID));
    }

    class UndoRedoTests : public juce::UnitTest
    {
    public:
        UndoRedoTests() : juce::UnitTest ("UndoRedo", "DNAOrbit") {}

        void runTest() override
        {
            beginTest ("apvts is wired to the processor's own UndoManager");
            {
                DNAOrbitAudioProcessor processor;
                expect (processor.apvts.undoManager == &processor.undoManager,
                        "APVTS must be constructed with the processor's UndoManager, or Undo/Redo can never work");
            }

            beginTest ("Undo reverts a single parameter change, Redo restores it");
            {
                DNAOrbitAudioProcessor processor;
                auto* mixParam = processor.apvts.getParameter (params::mixID);
                const float beforeDenorm = mixParam->convertFrom0to1 (mixParam->getValue());

                auto node = findParamNode (processor.apvts, params::mixID);
                expect (node.isValid(), "Test setup: the mix parameter's PARAM node must already exist in apvts.state");

                processor.undoManager.beginNewTransaction ("Mix");
                node.setProperty ("value", 90.0f, &processor.undoManager);

                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), 90.0f, 1.0e-3f,
                                            "Sanity: writing the ValueTree node must update the live parameter");

                processor.undoManager.undo();
                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), beforeDenorm, 1.0e-3f,
                                            "Undo must restore the parameter's value from before the change");

                processor.undoManager.redo();
                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), 90.0f, 1.0e-3f,
                                            "Redo must restore the changed value");
            }

            beginTest ("Multiple parameter changes grouped into one transaction undo/redo together");
            {
                // This is the mechanism Presets::apply()'s undoManagerForOneStep
                // argument relies on to make "pick a preset" a single Ctrl+Z,
                // rather than one Ctrl+Z per parameter the preset touches.
                DNAOrbitAudioProcessor processor;
                auto* mixParam = processor.apvts.getParameter (params::mixID);
                auto* radiusParam = processor.apvts.getParameter (params::radiusID);
                const float mixBefore = mixParam->convertFrom0to1 (mixParam->getValue());
                const float radiusBefore = radiusParam->convertFrom0to1 (radiusParam->getValue());

                auto mixNode = findParamNode (processor.apvts, params::mixID);
                auto radiusNode = findParamNode (processor.apvts, params::radiusID);

                processor.undoManager.beginNewTransaction ("Preset");
                mixNode.setProperty ("value", 12.0f, &processor.undoManager);
                radiusNode.setProperty ("value", 34.0f, &processor.undoManager);

                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), 12.0f, 1.0e-3f);
                expectWithinAbsoluteError (radiusParam->convertFrom0to1 (radiusParam->getValue()), 34.0f, 1.0e-3f);

                processor.undoManager.undo();

                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), mixBefore, 1.0e-3f,
                                            "A single Undo must revert every change made within the same transaction");
                expectWithinAbsoluteError (radiusParam->convertFrom0to1 (radiusParam->getValue()), radiusBefore, 1.0e-3f,
                                            "A single Undo must revert every change made within the same transaction");

                processor.undoManager.redo();
                expectWithinAbsoluteError (mixParam->convertFrom0to1 (mixParam->getValue()), 12.0f, 1.0e-3f,
                                            "A single Redo must restore every change made within the same transaction");
                expectWithinAbsoluteError (radiusParam->convertFrom0to1 (radiusParam->getValue()), 34.0f, 1.0e-3f,
                                            "A single Redo must restore every change made within the same transaction");
            }

            beginTest ("Presets::apply opens exactly one transaction when given an UndoManager");
            {
                // A narrower, black-box check that doesn't depend on the
                // flush timer: beginNewTransaction() changes the manager's
                // current transaction name, which is directly observable.
                DNAOrbitAudioProcessor processor;
                const auto& preset = presets::presets[1];

                processor.undoManager.beginNewTransaction ("before");
                presets::apply (processor.apvts, preset, &processor.undoManager);

                expect (processor.undoManager.getCurrentTransactionName() == juce::String (juce::CharPointer_UTF8 (preset.name)),
                        "Presets::apply must open a transaction named after the preset when given an UndoManager");
            }
        }
    };

    static UndoRedoTests undoRedoTests;
}
