/**
 * Developer tool: measures how *audible* a given parameter set actually is,
 * so preset intensity can be tuned from measurements rather than guesswork.
 *
 * Reports, for a mono-centred source (L==R in, so every bit of width/motion
 * in the output is the effect's doing):
 *
 *   panMove   - standard deviation of the short-window L/R balance, in dB.
 *               The headline "how much does the image actually move" number:
 *               a mono-centred source through a bypassed plugin gives 0.0,
 *               and bigger means a more audible sweep. Deliberately the
 *               standard deviation and not the peak-to-peak swing: a
 *               harmonically static source plus the strand delays produces
 *               comb notches that can momentarily null one channel, which
 *               sends a peak-to-peak reading to 15dB+ on a single outlier
 *               window while nothing audibly moves. The standard deviation
 *               is dominated by the sustained motion instead.
 *   width     - mean (1 - correlation) over the clip. 0 = mono, 1 = fully
 *               decorrelated. Complements panSwing: an effect can be wide
 *               without moving, or move without being wide.
 *   rmsDb     - output level relative to the dry input, in dB. Auto Gain is
 *               supposed to hold this near 0, so this column is the check
 *               that "more audible" did not simply become "louder".
 *   monoDb    - level of the mono fold-down relative to the dry input's own
 *               mono fold-down, in dB. Guards against buying width by
 *               sacrificing mono compatibility.
 *
 * Not part of the shipped plugin - build with -DDNA_ORBIT_BUILD_TOOLS=ON.
 */

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
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

    /** Four-note pad, mono-centred, no fades (this is analysis, not listening). */
    void fillPad (juce::AudioBuffer<float>& buffer, double sampleRate)
    {
        const int numSamples = buffer.getNumSamples();
        const double freqs[] = { 261.63, 329.63, 392.00, 523.25 };

        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            double sample = 0.0;
            for (double f : freqs)
                sample += 0.09 * std::sin (juce::MathConstants<double>::twoPi * f * (double) n / sampleRate);

            l[n] = (float) sample;
            r[n] = (float) sample;
        }
    }

    struct Metrics
    {
        double panMoveDb = 0.0;
        double width = 0.0;
        double rmsDb = 0.0;
        double monoDb = 0.0;
    };

    Metrics measure (const HelixEngine::Parameters& params, double sampleRate, double durationSeconds)
    {
        const int numSamples = (int) (durationSeconds * sampleRate);
        constexpr int blockSize = 512;

        juce::AudioBuffer<float> source (2, numSamples);
        fillPad (source, sampleRate);

        HelixEngine engine;
        engine.prepare (sampleRate, blockSize, 2);
        auto p = params;
        engine.primeParameters (p);

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

        // Skip the first second: the smoothers and the Bass Anchor filters
        // need to settle, and a startup transient would inflate panSwing.
        const int analysisStart = (int) sampleRate;
        const int analysisCount = numSamples - analysisStart;

        const auto* wl = wet.getReadPointer (0);
        const auto* wr = wet.getReadPointer (1);
        const auto* sl = source.getReadPointer (0);

        // --- panMove: standard deviation of the L/R balance over 50ms windows ---
        const int windowSamples = (int) (0.05 * sampleRate);
        std::vector<double> balances;

        for (int start = analysisStart; start + windowSamples <= numSamples; start += windowSamples)
        {
            double sumL = 0.0, sumR = 0.0;
            for (int n = start; n < start + windowSamples; ++n)
            {
                sumL += (double) wl[n] * wl[n];
                sumR += (double) wr[n] * wr[n];
            }
            const double rmsL = std::sqrt (sumL / windowSamples);
            const double rmsR = std::sqrt (sumR / windowSamples);

            // Both channels essentially silent: no meaningful balance to report.
            if (rmsL < 1.0e-6 && rmsR < 1.0e-6)
                continue;

            balances.push_back (20.0 * std::log10 ((rmsL + 1.0e-9) / (rmsR + 1.0e-9)));
        }

        Metrics m;
        if (balances.size() > 1)
        {
            double mean = 0.0;
            for (double b : balances)
                mean += b;
            mean /= (double) balances.size();

            double variance = 0.0;
            for (double b : balances)
                variance += (b - mean) * (b - mean);
            variance /= (double) balances.size();

            m.panMoveDb = std::sqrt (variance);
        }

        // --- width, rms, mono ----------------------------------------------------
        double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
        double sumWetSq = 0.0, sumDrySq = 0.0;
        double sumWetMonoSq = 0.0, sumDryMonoSq = 0.0;

        for (int n = analysisStart; n < numSamples; ++n)
        {
            sumLL += (double) wl[n] * wl[n];
            sumRR += (double) wr[n] * wr[n];
            sumLR += (double) wl[n] * wr[n];

            sumWetSq += 0.5 * ((double) wl[n] * wl[n] + (double) wr[n] * wr[n]);
            sumDrySq += (double) sl[n] * sl[n]; // source is L==R

            const double wetMono = 0.5 * ((double) wl[n] + wr[n]);
            sumWetMonoSq += wetMono * wetMono;
            sumDryMonoSq += (double) sl[n] * sl[n];
        }

        const double denom = std::sqrt (sumLL * sumRR);
        const double correlation = denom > 1.0e-12 ? juce::jlimit (-1.0, 1.0, sumLR / denom) : 1.0;
        m.width = 1.0 - correlation;

        const double wetRms = std::sqrt (sumWetSq / analysisCount);
        const double dryRms = std::sqrt (sumDrySq / analysisCount);
        m.rmsDb = 20.0 * std::log10 ((wetRms + 1.0e-12) / (dryRms + 1.0e-12));

        const double wetMonoRms = std::sqrt (sumWetMonoSq / analysisCount);
        const double dryMonoRms = std::sqrt (sumDryMonoSq / analysisCount);
        m.monoDb = 20.0 * std::log10 ((wetMonoRms + 1.0e-12) / (dryMonoRms + 1.0e-12));

        return m;
    }

    void printRow (const juce::String& label, const Metrics& m)
    {
        std::printf ("%-34s %9.2f %8.3f %8.2f %8.2f\n",
                      label.toRawUTF8(), m.panMoveDb, m.width, m.rmsDb, m.monoDb);
    }

    void printHeader (const char* title)
    {
        std::printf ("\n%s\n", title);
        std::printf ("%-34s %9s %8s %8s %8s\n", "", "panMove", "width", "rmsDb", "monoDb");
        std::printf ("%-34s %9s %8s %8s %8s\n", "", "(dB sd)", "(1-corr)", "vs dry", "vs dry");
        std::printf ("--------------------------------------------------------------------------\n");
    }
}

int main()
{
    constexpr double sampleRate = 48000.0;
    // Long enough to cover several full orbits even at the slowest preset
    // rate (0.04 Hz = 25 s/rev would need far longer, so slow presets are
    // measured over whatever fraction fits - noted in the output).
    constexpr double duration = 30.0;

    printHeader ("Current factory presets (as shipped)");
    for (int i = 0; i < dnaorbit::presets::numPresets; ++i)
    {
        const auto& preset = dnaorbit::presets::presets[i];
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow (juce::String (i) + ": mix=" + juce::String ((int) preset.mix)
                      + "% depth=" + juce::String ((int) preset.depth)
                      + "% rate=" + juce::String (preset.rateHz, 2),
                  m);
    }

    // --- Mix sweep, holding everything else at preset 1 ("パッドを回す") ---------
    printHeader ("Mix sweep (other params = preset 1)");
    for (float mix : { 30.0f, 45.0f, 55.0f, 65.0f, 75.0f, 85.0f, 100.0f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = mix;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("mix = " + juce::String ((int) mix) + "%", m);
    }

    // --- Depth sweep ------------------------------------------------------------
    printHeader ("Depth sweep (preset 1, mix held at 65%)");
    for (float depth : { 45.0f, 55.0f, 65.0f, 80.0f, 95.0f, 100.0f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = 65.0f;
        preset.depth = depth;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("depth = " + juce::String ((int) depth) + "%", m);
    }

    // --- Stereo Preserve sweep --------------------------------------------------
    // A mono-centred source has no Side content, so the bed is silent and
    // this should not move any of the numbers. Included as a control: if it
    // does move them, an assumption somewhere is wrong.
    printHeader ("Stereo Preserve sweep (control: mono source => expect no change)");
    for (float sp : { 0.0f, 35.0f, 70.0f, 100.0f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = 65.0f;
        preset.stereoPreserve = sp;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("stereoPreserve = " + juce::String ((int) sp) + "%", m);
    }

    // --- Twist sweep ------------------------------------------------------------
    printHeader ("Twist sweep (preset 1, mix held at 65%)");
    for (float twist : { 0.0f, 4.0f, 7.0f, 12.0f, 20.0f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = 65.0f;
        preset.twist = twist;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("twist = " + juce::String (twist, 0) + "ms", m);
    }

    // --- Rate sweep -------------------------------------------------------------
    // Rate should barely move any of these numbers (they are all
    // time-averaged over the whole clip, and the orbit's *shape* does not
    // depend on speed) - but it dominates whether a listener perceives a
    // full revolution inside a musical phrase, which no averaged metric can
    // capture. Included so that flatness here is on the record.
    printHeader ("Rate sweep (preset 1, mix held at 65%)");
    for (float rate : { 0.08f, 0.18f, 0.30f, 0.50f, 0.80f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = 65.0f;
        preset.rateHz = rate;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("rate = " + juce::String (rate, 2) + "Hz ("
                      + juce::String (1.0f / rate, 1) + "s/rev)", m);
    }

    // --- Core sweep -------------------------------------------------------------
    // Core adds a centred copy into Wet, so raising it should *reduce*
    // panSwing (more centre, less swing) - the check that Core is the
    // "anchor the middle" control it claims to be.
    printHeader ("Core sweep (preset 1, mix held at 65%)");
    for (float core : { 0.0f, 10.0f, 25.0f, 50.0f })
    {
        auto preset = dnaorbit::presets::presets[1];
        preset.mix = 65.0f;
        preset.core = core;
        const auto m = measure (toEngineParameters (preset), sampleRate, duration);
        printRow ("core = " + juce::String ((int) core) + "%", m);
    }

    std::printf ("\n");
    return 0;
}
