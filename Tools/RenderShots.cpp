/**
 * Headless screenshot harness.
 *
 * The Standalone build cannot animate in a container with no audio device: with
 * no audio callback, processBlock never runs, the orbit phase never advances and
 * the visualiser is frozen. This tool drives the processor directly from the
 * message thread while pumping the event loop, so the timers fire and the view
 * animates exactly as it would in a DAW - which makes it possible to actually
 * SEE and verify the locked-vs-drifting centre line.
 *
 * Usage: RenderShots <output-directory>
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    void setParam (juce::AudioProcessorValueTreeState& apvts, const char* id, float actual)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (actual));
    }

    struct Scenario
    {
        const char* fileName;
        float symmetry;
        float rateHz;
        bool  nullCore;
        int   page;          // 0 = basic, 1 = detail
        double secondsToRun;
    };

    void renderScenario (const Scenario& scenario, const juce::File& outputDir)
    {
        DNAOrbitAudioProcessor processor;

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        processor.prepareToPlay (sampleRate, blockSize);

        setParam (processor.apvts, dnaorbit::params::symmetryID, scenario.symmetry);
        setParam (processor.apvts, dnaorbit::params::rateID, scenario.rateHz);
        if (auto* nullCore = processor.apvts.getParameter (dnaorbit::params::nullCoreID))
            nullCore->setValueNotifyingHost (scenario.nullCore ? 1.0f : 0.0f);

        processor.apvts.state.setProperty ("editorPage", scenario.page, nullptr);

        std::unique_ptr<juce::AudioProcessorEditor> editor { processor.createEditor() };
        editor->setSize (900, 620);
        editor->setVisible (true);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        juce::Random random { 1234 };
        double phase = 0.0;

        const int blocks = (int) (scenario.secondsToRun * sampleRate / blockSize);

        for (int i = 0; i < blocks; ++i)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const float sample = 0.25f * (float) std::sin (phase)
                                   + 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                phase += juce::MathConstants<double>::twoPi * 220.0 / sampleRate;
                buffer.setSample (0, n, sample);
                buffer.setSample (1, n, sample);
            }

            processor.processBlock (buffer, midi);

            // Let the editor's timers run so the history advances.
            juce::MessageManager::getInstance()->runDispatchLoopUntil (8);
        }

        juce::MessageManager::getInstance()->runDispatchLoopUntil (120);

        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), false);
        const auto file = outputDir.getChildFile (scenario.fileName);
        file.deleteFile();

        juce::FileOutputStream stream { file };
        if (stream.openedOk())
        {
            juce::PNGImageFormat png;
            png.writeImageToStream (image, stream);
            std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
        }
        else
        {
            std::printf ("FAILED to write %s\n", file.getFullPathName().toRawUTF8());
        }

    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outputDir = argc > 1 ? juce::File (juce::String (argv[1]))
                                          : juce::File::getCurrentWorkingDirectory();
    outputDir.createDirectory();

    const Scenario scenarios[] = {
        { "shot_basic_locked.png",  100.0f, 0.50f, false, 0, 3.0 },
        { "shot_detail_locked.png", 100.0f, 0.50f, false, 1, 3.0 },
        { "shot_drift_70.png",       70.0f, 2.00f, false, 0, 8.0 },
        { "shot_drift_40.png",       40.0f, 2.00f, false, 0, 8.0 },
        { "shot_nullcore.png",      100.0f, 0.50f, true,  1, 3.0 },
    };

    for (const auto& scenario : scenarios)
        renderScenario (scenario, outputDir);

    return 0;
}
