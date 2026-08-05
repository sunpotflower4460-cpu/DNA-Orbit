/**
 * Developer/demo tool: renders dry/wet WAV pairs of synthesized sources
 * through real factory presets, so the effect can be listened to rather
 * than only reasoned about from the code.
 *
 * Three sources, each matched to the preset built for that material: a
 * sustained pad, a "vocal-ish" tone, and a strummed guitar chord. All
 * three start mono-centred (L == R), so every bit of width and movement
 * in the wet file is the effect's doing and nothing else.
 *
 * These are synthesized proxies, not recordings. They are useful for
 * hearing what the effect *does* to a given kind of material - sustained
 * versus decaying, harmonically simple versus dense - and useless as
 * evidence about how the plugin sits in a real mix. Real-material
 * listening stays on the MANUAL_REQUIRED.md list.
 *
 * Clip lengths are derived from each preset's own revolution time rather
 * than picked round: below roughly two full revolutions the motion reads
 * as a fixed off-centre image instead of as motion.
 *
 * Not part of the shipped plugin - build with -DDNA_ORBIT_BUILD_TOOLS=ON.
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>

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

    /**
     * A strummed open-position E-minor chord, mono-centred.
     *
     * Uses Karplus-Strong rather than stacked sines: a plucked string's
     * character is a noise burst filtered by its own round trip, and no
     * amount of added harmonics reproduces that from sine tones. Each
     * string is a delay line the length of one period, excited once with
     * noise and then fed back through a two-point average, which is what
     * produces the bright attack decaying to a mellow tail.
     *
     * Still a synthesised proxy, not a recording - it has no pick noise,
     * no fret buzz, no amp or cabinet, and every strum is identical. It is
     * good enough to hear what the *effect* does to a plucked, decaying,
     * harmonically dense source; it is not evidence about how the plugin
     * sits on a real guitar track.
     */
    void fillGuitar (juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        const int numSamples = buffer.getNumSamples();
        buffer.clear();

        // Open Em: E2 B2 E3 G3 B3 E4 - the fullest-sounding standard shape.
        const double stringHz[] = { 82.41, 123.47, 164.81, 196.00, 246.94, 329.63 };
        constexpr int numStrings = 6;

        // Strum every 3.5s, alternating down/up so the sample does not
        // sound like one gesture looped.
        const double strumTimes[] = { 0.15, 3.65, 7.15, 10.65, 14.15 };

        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);

        juce::Random random { 1234 }; // fixed seed: the render is reproducible

        for (double strumTime : strumTimes)
        {
            const bool downStroke = ((int) (strumTime * 2.0)) % 2 == 0;

            for (int s = 0; s < numStrings; ++s)
            {
                // Low-to-high on a downstroke, reversed on an upstroke,
                // with ~18ms between strings - roughly a real strum rate.
                const int order = downStroke ? s : (numStrings - 1 - s);
                const double startSeconds = strumTime + order * 0.018;
                const int startSample = (int) (startSeconds * sampleRate);
                if (startSample >= numSamples)
                    continue;

                const int period = juce::jmax (2, (int) std::round (sampleRate / stringHz[s]));
                std::vector<float> delayLine ((size_t) period);
                for (auto& v : delayLine)
                    v = random.nextFloat() * 2.0f - 1.0f;

                // Wound low strings ring longer than plain high ones.
                const float decay = s < 3 ? 0.9985f : 0.9965f;
                // Upper strings sit slightly back so the chord is not
                // top-heavy once six of them overlap.
                const float level = (s < 3 ? 0.20f : 0.15f) * (downStroke ? 1.0f : 0.85f);

                int index = 0;
                float previous = 0.0f;
                for (int n = startSample; n < numSamples; ++n)
                {
                    const float current = delayLine[(size_t) index];
                    const float filtered = 0.5f * (current + previous) * decay;
                    delayLine[(size_t) index] = filtered;
                    previous = current;
                    index = (index + 1) % period;

                    const float sample = current * level;
                    l[n] += sample;
                    r[n] += sample;

                    // Stop once this string has decayed out of the mix,
                    // rather than burning cycles on inaudible tails.
                    if (n > startSample + (int) sampleRate * 4 && std::abs (current) < 1.0e-5f)
                        break;
                }
            }
        }

        // Gentle fade-out only; the attacks must stay intact.
        const int fadeOutSamples = (int) (0.5 * sampleRate);
        for (int n = numSamples - fadeOutSamples; n < numSamples; ++n)
        {
            const float env = (float) (numSamples - n) / (float) fadeOutSamples;
            l[n] *= env;
            r[n] *= env;
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
    // Durations are set from each preset's own revolution time, not picked
    // round: at 0.18Hz one orbit takes 5.6s, so an 8s clip only ever showed
    // 1.4 turns and the earlier vocal clip (6s at 0.13Hz) did not even
    // complete one. Roughly two full revolutions is the minimum for the
    // motion to read as motion rather than as a fixed off-centre image.
    renderPair (outputDir, "pad_rotate", fillPad, dnaorbit::presets::presets[1], sampleRate, 12.0);

    // presets[0] = "ボーカルを広げる" (widen a vocal), 0.13Hz -> 7.7s/rev
    renderPair (outputDir, "vocal_widen", fillVocalish, dnaorbit::presets::presets[0], sampleRate, 16.0);

    // presets[2] = "ギターに揺らぎ" (add movement to a guitar), 0.11Hz -> 9.1s/rev
    renderPair (outputDir, "guitar_sway", fillGuitar, dnaorbit::presets::presets[2], sampleRate, 18.0);

    std::printf ("Done. Files written to: %s\n", outputDir.getFullPathName().toRawUTF8());
    return 0;
}
