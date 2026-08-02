/**
 * Headless UI screenshot harness.
 *
 * The tool drives the processor directly while pumping the message loop so the
 * visualiser and editor timers advance without a DAW or audio device.
 *
 * Usage: DNAOrbitRenderShots <output-directory>
 */

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    void setParam (juce::AudioProcessorValueTreeState& apvts,
                   const char* id, float actual)
    {
        if (auto* parameter = apvts.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
    }

    struct Scenario
    {
        const char* fileName;
        int width;
        int height;
        int page;
        float symmetry;
        float rateHz;
        bool sync;
        bool nullCore;
        bool softBypass;
        double secondsToRun;
    };

    bool renderScenario (const Scenario& scenario, const juce::File& outputDir)
    {
        DNAOrbitAudioProcessor processor;

        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        processor.prepareToPlay (sampleRate, blockSize);

        setParam (processor.apvts, dnaorbit::params::symmetryID, scenario.symmetry);
        setParam (processor.apvts, dnaorbit::params::rateID, scenario.rateHz);
        setParam (processor.apvts, dnaorbit::params::syncID, scenario.sync ? 1.0f : 0.0f);
        setParam (processor.apvts, dnaorbit::params::nullCoreID, scenario.nullCore ? 1.0f : 0.0f);
        setParam (processor.apvts, dnaorbit::params::softBypassID, scenario.softBypass ? 1.0f : 0.0f);
        setParam (processor.apvts, dnaorbit::params::bassAnchorID, 120.0f);
        setParam (processor.apvts, dnaorbit::params::stereoPreserveID, 70.0f);
        setParam (processor.apvts, dnaorbit::params::phaseModeID,
                  (float) dnaorbit::params::phaseHostLock);

        processor.apvts.state
            .getOrCreateChildWithName (dnaorbit::params::uiStateNodeID, nullptr)
            .setProperty (dnaorbit::params::editorPagePropertyID, scenario.page, nullptr);

        std::unique_ptr<juce::AudioProcessorEditor> editor { processor.createEditor() };
        editor->setSize (scenario.width, scenario.height);
        editor->setVisible (true);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        juce::Random random { 1234 };
        double phaseA = 0.0;
        double phaseB = 0.0;

        const int blocks = juce::jmax (1, (int) std::ceil (
            scenario.secondsToRun * sampleRate / blockSize));

        for (int block = 0; block < blocks; ++block)
        {
            for (int n = 0; n < blockSize; ++n)
            {
                const float mid = 0.20f * (float) std::sin (phaseA)
                                + 0.035f * (random.nextFloat() * 2.0f - 1.0f);
                const float side = 0.055f * (float) std::sin (phaseB);
                phaseA += juce::MathConstants<double>::twoPi * 220.0 / sampleRate;
                phaseB += juce::MathConstants<double>::twoPi * 337.0 / sampleRate;
                buffer.setSample (0, n, mid + side);
                buffer.setSample (1, n, mid - side);
            }

            processor.processBlock (buffer, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (8);
        }

        juce::MessageManager::getInstance()->runDispatchLoopUntil (140);

        const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), false);
        if (! image.isValid()
            || image.getWidth() != scenario.width
            || image.getHeight() != scenario.height)
        {
            std::printf ("FAILED invalid snapshot %s\n", scenario.fileName);
            return false;
        }

        const auto file = outputDir.getChildFile (scenario.fileName);
        file.deleteFile();

        juce::FileOutputStream stream { file };
        if (! stream.openedOk())
        {
            std::printf ("FAILED to open %s\n", file.getFullPathName().toRawUTF8());
            return false;
        }

        juce::PNGImageFormat png;
        if (! png.writeImageToStream (image, stream))
        {
            std::printf ("FAILED to encode %s\n", file.getFullPathName().toRawUTF8());
            return false;
        }

        std::printf ("wrote %s (%dx%d)\n",
                     file.getFullPathName().toRawUTF8(),
                     scenario.width, scenario.height);
        return true;
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File outputDir = argc > 1
        ? juce::File (juce::String (argv[1]))
        : juce::File::getCurrentWorkingDirectory();

    if (! outputDir.createDirectory())
    {
        std::printf ("FAILED to create %s\n",
                     outputDir.getFullPathName().toRawUTF8());
        return 1;
    }

    for (const auto& stale : outputDir.findChildFiles (
             juce::File::findFiles, false, "*.png"))
        stale.deleteFile();

    const Scenario scenarios[] = {
        { "ui_basic_min_free_820x650.png",       820, 650, 0, 100.0f, 0.50f, false, false, false, 3.0 },
        { "ui_basic_min_sync_820x650.png",       820, 650, 0, 100.0f, 0.50f, true,  false, false, 3.0 },
        { "ui_detail_min_820x650.png",           820, 650, 1, 100.0f, 0.50f, true,  false, false, 3.0 },
        { "ui_basic_standard_960x700.png",       960, 700, 0, 100.0f, 0.35f, false, false, false, 3.0 },
        { "ui_detail_standard_960x700.png",      960, 700, 1,  70.0f, 0.65f, true,  false, false, 6.0 },
        { "ui_detail_wide_1440x900.png",        1440, 900, 1,  45.0f, 1.10f, true,  false, false, 7.0 },
        { "ui_detail_nullcore_960x700.png",      960, 700, 1, 100.0f, 0.50f, true,  true,  false, 3.0 },
        { "ui_basic_softbypass_960x700.png",     960, 700, 0, 100.0f, 0.50f, false, false, true,  3.0 }
    };

    bool allOk = true;
    for (const auto& scenario : scenarios)
        allOk = renderScenario (scenario, outputDir) && allOk;

    return allOk ? 0 : 1;
}
