#include <juce_audio_basics/juce_audio_basics.h>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "dsp/HelixEngine.h"

namespace
{
    struct BenchmarkCase
    {
        double sampleRate;
        int blockSize;
        int instances;
    };

    struct Instance
    {
        dnaorbit::dsp::HelixEngine engine;
        juce::AudioBuffer<float> buffer;
        double midPhase = 0.0;
        double sidePhase = 0.0;

        Instance (double sampleRate, int blockSize)
            : buffer (2, blockSize)
        {
            engine.prepare (sampleRate, blockSize, 2);

            dnaorbit::dsp::HelixEngine::Parameters p;
            p.rateHz = 0.35f;
            p.radius01 = 0.9f;
            p.depth01 = 0.75f;
            p.symmetry01 = 0.72f;
            p.twistMs = 9.0f;
            p.core01 = 0.12f;
            p.mix01 = 0.5f;
            p.autoGain = true;
            p.stereoPreserve01 = 0.7f;
            p.bassAnchorHz = 120.0f;
            p.character = 2;
            p.phaseMode = 0;
            engine.primeParameters (p);
            engine.setParameters (p);
        }

        void fillInput (double sampleRate)
        {
            auto* left = buffer.getWritePointer (0);
            auto* right = buffer.getWritePointer (1);
            const double midIncrement = juce::MathConstants<double>::twoPi * 220.0 / sampleRate;
            const double sideIncrement = juce::MathConstants<double>::twoPi * 337.0 / sampleRate;

            for (int n = 0; n < buffer.getNumSamples(); ++n)
            {
                const float mid = 0.22f * (float) std::sin (midPhase);
                const float side = 0.08f * (float) std::sin (sidePhase);
                left[n] = mid + side;
                right[n] = mid - side;

                midPhase += midIncrement;
                sidePhase += sideIncrement;
                if (midPhase >= juce::MathConstants<double>::twoPi)
                    midPhase -= juce::MathConstants<double>::twoPi;
                if (sidePhase >= juce::MathConstants<double>::twoPi)
                    sidePhase -= juce::MathConstants<double>::twoPi;
            }
        }
    };

    void runCase (const BenchmarkCase& c, double simulatedSeconds)
    {
        const int blocks = std::max (1, (int) std::ceil (
            simulatedSeconds * c.sampleRate / (double) c.blockSize));

        std::vector<std::unique_ptr<Instance>> instances;
        instances.reserve ((size_t) c.instances);
        for (int i = 0; i < c.instances; ++i)
            instances.push_back (std::make_unique<Instance> (c.sampleRate, c.blockSize));

        for (int block = 0; block < 100; ++block)
            for (auto& instance : instances)
            {
                instance->fillInput (c.sampleRate);
                instance->engine.process (instance->buffer, 2);
            }

        const auto start = std::chrono::steady_clock::now();
        for (int block = 0; block < blocks; ++block)
            for (auto& instance : instances)
            {
                instance->fillInput (c.sampleRate);
                instance->engine.process (instance->buffer, 2);
            }
        const auto finish = std::chrono::steady_clock::now();

        const double elapsed = std::chrono::duration<double> (finish - start).count();
        const double samplesProcessed = (double) blocks * c.blockSize * c.instances;
        const double nsPerSamplePerInstance = elapsed * 1.0e9 / samplesProcessed;
        const double realtimePercentAllInstances = elapsed / simulatedSeconds * 100.0;

        std::cout << std::fixed << std::setprecision (3)
                  << std::setw (7) << c.sampleRate << " Hz  "
                  << std::setw (4) << c.blockSize << " samples  "
                  << std::setw (2) << c.instances << " instance(s)  "
                  << std::setw (9) << nsPerSamplePerInstance << " ns/sample/instance  "
                  << std::setw (8) << realtimePercentAllInstances << " % realtime\n";
    }
}

int main (int argc, char** argv)
{
    double seconds = 2.0;
    if (argc > 1)
        seconds = juce::jlimit (0.25, 30.0, juce::String (argv[1]).getDoubleValue());

    const std::vector<BenchmarkCase> cases {
        { 44100.0,  64, 1 }, { 44100.0,  64, 10 },
        { 48000.0, 128, 1 }, { 48000.0, 128, 10 },
        { 96000.0, 128, 1 }, { 96000.0, 128, 10 },
        { 192000.0, 256, 1 }, { 192000.0, 256, 10 },
        { 48000.0,  32, 1 }, { 48000.0, 512, 1 }
    };

    std::cout << "DNA Orbit DSP benchmark\n"
              << "Simulated seconds per case: " << seconds << "\n"
              << "Release builds only; compare on the same machine and power mode.\n\n";

    for (const auto& c : cases)
        runCase (c, seconds);

    return 0;
}
