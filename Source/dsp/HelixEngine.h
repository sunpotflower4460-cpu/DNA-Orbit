#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

#include "OrbitMath.h"
#include "OnePoleLowPass.h"
#include "StereoUtilities.h"

namespace dnaorbit::dsp
{
    /**
     * Core DSP engine for DNA Orbit: two strands rotating around a shared
     * centre axis, combined with an optional Core signal and the
     * experimental NULL CORE (Wet mid-removal) mode.
     *
     * All parameter changes are smoothed; the audio thread never allocates,
     * locks, or touches the UI directly. Visual state is published through
     * std::atomic members that the editor polls on a timer.
     */
    class HelixEngine
    {
    public:
        HelixEngine() = default;

        struct Parameters
        {
            float rateHz     = 0.12f;
            float radius01   = 0.80f;
            float depth01    = 0.55f;
            float symmetry01 = 1.00f;
            float twistMs    = 5.0f;
            float core01     = 0.0f;
            bool  nullCore   = false;
            float mix01      = 0.35f;
            float outputDb   = 0.0f;
        };

        void prepare (double newSampleRate, int maximumBlockSize, int maxChannelsHint);
        void reset();

        /** Call once per block before process(). */
        void setParameters (const Parameters& newParams) noexcept;

        /**
         * Processes buffer in place. `buffer` must contain exactly 2 channels;
         * the first `numInputChannels` channels hold valid input data (1 for
         * mono-in, 2 for stereo-in) and any remaining channels are assumed to
         * already be cleared by the caller.
         */
        void process (juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept;

        struct VisualState
        {
            float thetaA = 0.0f;
            float thetaB = 0.0f;
            float radius01 = 0.0f;
            float centroidX = 0.0f;
            float centroidZ = 0.0f;
            float centroidDistance = 0.0f;
            float symmetry01 = 1.0f;
            bool  nullCoreOn = false;
        };

        VisualState getVisualState() const noexcept;

    private:
        double sampleRate = 44100.0;

        // Orbit state (double precision to avoid long-run drift).
        double thetaA = 0.0;
        double thetaB = orbitmath::pi;
        bool   symmetryLocked = true;

        // Smoothed parameters.
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radiusSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> symmetrySmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> twistSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> coreSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> nullCoreMixSmoothed;

        float rateHzTarget = 0.12f;
        bool  nullCoreTarget = false;

        // Per-strand processing chains.
        OnePoleLowPass lowPassA, lowPassB;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayA { 1 << 14 };
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayB { 1 << 14 };
        float maxDelaySamplesStored = 0.0f;

        static constexpr float strandGain       = 0.5f;
        static constexpr float maxBackAttenDb    = 4.0f;
        static constexpr float frontCutoffHz     = 19000.0f;
        static constexpr float backCutoffHz      = 5000.0f;
        static constexpr float maxBackDelayMs    = 8.0f;
        static constexpr double maxRateDifference = 0.03;
        static constexpr double symmetryLockThreshold = 0.999;
        static constexpr double resyncTimeConstantSeconds = 0.15;
        static constexpr double resyncEpsilonRadians = 1.0e-4;

        // Published for the UI thread; written once per block.
        std::atomic<float> uiThetaA { 0.0f };
        std::atomic<float> uiThetaB { static_cast<float> (orbitmath::pi) };
        std::atomic<float> uiRadius { 0.8f };
        std::atomic<float> uiCentroidX { 0.0f };
        std::atomic<float> uiCentroidZ { 0.0f };
        std::atomic<float> uiCentroidDistance { 0.0f };
        std::atomic<float> uiSymmetry { 1.0f };
        std::atomic<bool>  uiNullCoreOn { false };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelixEngine)
    };
}
