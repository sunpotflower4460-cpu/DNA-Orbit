/**
 * Developer/demo tool: renders short WAV files of a synthesized source
 * (a pad and a sustained "vocal-ish" tone) both dry and processed through
 * the real HelixEngine using two factory presets, so the effect can
 * actually be listened to rather than only reasoned about from the code.
 * Not part of the shipped plugin - build with -DDNA_ORBIT_BUILD_TOOLS=ON.
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

#include "Presets.h"
#include "dsp/HelixEngine.h"

using namespace dnaorbit::dsp;

namespace
{
    HelixEngine::Parameters toEngineParameters (const dnaorbit::presets::Preset& preset)
    {
        HelixEngine::Parameters p;
        p.rateHz           = preset.rateHz;
        p.radius01         = preset.radius / 100.0f;
        p.depth01          = preset.depth / 100.0f;
        p.symmetry01       = preset.symmetry / 100.0f;
        p.twistMs          = preset.twist;
        p.core01           = preset.core / 100.0f;
        p.nullCore         = preset.nullCore;
        p.mix01            = preset.mix / 100.0f;
        p.outputDb         = preset.output;
        p.autoGain         = preset.autoGain;
        p.stereoPreserve01 = preset.stereoPreserve / 100.0f;
        p.bassAnchorHz     = preset.bassAnchorHz;
        p.character        = preset.character;
        p.phaseMode        = preset.phaseMode;
        p.startPhaseDeg    = preset.startPhaseDeg;
        p.clockwise        = preset.clockwise;
        p.softBypass       = preset.softBypass;
        p.monoPreview      = preset.monoPreview;
        return p;
    }

    // A simple additive pad (a four-note chord), mono-centred (L==R) so any
    // width/motion heard afterwards is entirely the effect's doing. Fades
    // in/out to avoid clicks at the start/end of the clip.
    void fillPad (juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        const int numSamples = buffer.getNumSamples();
        const double freqs[] = { 261.63, 329.63, 392.00, 523.25 }; // C4 E4 G4 C5
        const int fadeInSamples  = (int) (0.6 * sampleRate);
        const int fadeOutSamples = (int) (1.0 * sampleRate);

        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            double sample = 0.0;
            for (double f : freqs)
                sample += 0.09 * std::sin (juce::MathConstants<double>::twoPi * f * (double) n / sampleRate);

            float env = 1.0f;
            if (n < fadeInSamples)
                env = (float) n / (float) fadeInSamples;
            else if (n > numSamples - fadeOutSamples)
                env = (float) (numSamples - n) / (float) fadeOutSamples;

            const float s = (float) sample * env;
            l[n] = s;
            r[n] = s;
        }
    }

    // A single sustained "sung" note - fundamental + two harmonics, gentle
    // vibrato - mono-centred, same fade envelope.
    void fillVocalish (juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        const int numSamples = buffer.getNumSamples();
        const double fundamental = 220.0; // A3
        const double vibratoHz = 5.5;
        const double vibratoDepthCents = 15.0;
        const int fadeInSamples  = (int) (0.4 * sampleRate);
        const int fadeOutSamples = (int) (0.8 * sampleRate);

        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);

        double phase1 = 0.0, phase2 = 0.0, phase3 = 0.0;

        for (int n = 0; n < numSamples; ++n)
        {
            const double t = (double) n / sampleRate;
            const double vibratoRatio = std::pow (2.0,
                (vibratoDepthCents * std::sin (juce::MathConstants<double>::twoPi * vibratoHz * t)) / 1200.0);
            const double f0 = fundamental * vibratoRatio;

            phase1 += juce::MathConstants<double>::twoPi * f0 / sampleRate;
            phase2 += juce::MathConstants<double>::twoPi * f0 * 2.0 / sampleRate;
            phase3 += juce::MathConstants<double>::twoPi * f0 * 3.0 / sampleRate;

            const double sample = 0.22 * std::sin (phase1) + 0.10 * std::sin (phase2) + 0.05 * std::sin (phase3);

            float env = 1.0f;
            if (n < fadeInSamples)
                env = (float) n / (float) fadeInSamples;
            else if (n > numSamples - fadeOutSamples)
                env = (float) (numSamples - n) / (float) fadeOutSamples;

            const float s = (float) sample * env;
            l[n] = s;
            r[n] = s;
        }
    }

    void writeWav (const juce::File& file, const juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        file.deleteFile();
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());

        if (stream == nullptr)
        {
            std::fprintf (stderr, "Could not open %s for writing\n", file.getFullPathName().toRawUTF8());
            return;
        }

        const auto options = juce::AudioFormatWriterOptions()
                                  .withSampleRate (sampleRate)
                                  .withNumChannels (buffer.getNumChannels())
                                  .withBitsPerSample (16);
        auto writer = wavFormat.createWriterFor (stream, options);

        if (writer == nullptr)
        {
            std::fprintf (stderr, "Could not create a WAV writer for %s\n", file.getFullPathName().toRawUTF8());
            return;
        }

        writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
        std::printf ("wrote %s\n", file.getFullPathName().toRawUTF8());
    }

    using SourceFiller = void (*) (juce::AudioBuffer<float>&, double);

    void renderPair (const juce::File& outputDir, const juce::String& baseName, SourceFiller fillSource,
                      const dnaorbit::presets::Preset& preset, double sampleRate, double durationSeconds)
    {
        const int numSamples = (int) (durationSeconds * sampleRate);
        constexpr int blockSize = 512;

        juce::AudioBuffer<float> source (2, numSamples);
        fillSource (source, sampleRate);
        writeWav (outputDir.getChildFile (baseName + "_1_dry.wav"), source, sampleRate);

        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);
        auto params = toEngineParameters (preset);
        engine.primeParameters (params); // snap immediately - a short clip has no time to fade in

        juce::AudioBuffer<float> wet (2, numSamples);
        for (int start = 0; start < numSamples; start += blockSize)
        {
            const int n = std::min (blockSize, numSamples - start);
            juce::AudioBuffer<float> block (2, n);
            block.copyFrom (0, 0, source, 0, start, n);
            block.copyFrom (1, 0, source, 1, start, n);
            engine.process (block, 2);
            wet.copyFrom (0, start, block, 0, 0, n);
            wet.copyFrom (1, start, block, 1, 0, n);
        }

        writeWav (outputDir.getChildFile (baseName + "_2_wet.wav"), wet, sampleRate);
    }
}

int main (int argc, char** argv)
{
    const juce::File outputDir = argc > 1
        ? juce::File (juce::String (argv[1]))
        : juce::File::getCurrentWorkingDirectory().getChildFile ("AudioSamples");
    outputDir.createDirectory();

    constexpr double sampleRate = 48000.0;

    // presets[1] = "パッドを回す" (rotate a pad)
    renderPair (outputDir, "pad_rotate", fillPad, dnaorbit::presets::presets[1], sampleRate, 8.0);

    // presets[0] = "ボーカルを広げる" (widen a vocal)
    renderPair (outputDir, "vocal_widen", fillVocalish, dnaorbit::presets::presets[0], sampleRate, 6.0);

    std::printf ("Done. Files written to: %s\n", outputDir.getFullPathName().toRawUTF8());
    return 0;
}
