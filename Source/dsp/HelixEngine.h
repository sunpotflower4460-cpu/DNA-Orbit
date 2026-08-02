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
     * Core DSP engine for DNA Orbit: two Mid-driven strands rotating around a
     * shared centre axis, a stationary Side-preservation bed, optional Core,
     * and the experimental NULL CORE mode.
     *
     * All parameter changes are smoothed; the audio thread never allocates,
     * locks, performs I/O, or touches the UI directly.
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
            bool  autoGain   = true;
            bool  softBypass = false;

            /**
             * Amount of the original Side signal retained as a stationary
             * stereo bed around the centred DNA orbit. The moving strands
             * always receive the shared Mid signal. 0 reproduces the original
             * Mid-only path; 1 restores the input Side at unity.
             */
            float stereoPreserve01 = 0.0f;
        };

        void prepare (double newSampleRate, int maximumBlockSize, int maxChannelsHint);
        void reset();
        void setParameters (const Parameters& newParams) noexcept;
        void primeParameters (const Parameters& newParams) noexcept;
        void process (juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept;

        struct VisualState
        {
            double phaseA = 0.0;
            float phi = 0.0f;
            float thetaA = 0.0f;
            float thetaB = 0.0f;
            float radius01 = 0.0f;
            float centroidX = 0.0f;
            float centroidZ = 0.0f;
            float centroidDistance = 0.0f;
            float symmetry01 = 1.0f;
            bool  nullCoreOn = false;
            float outputRms = 0.0f;
            float correlation = 1.0f;
        };

        VisualState getVisualState() const noexcept;

    private:
        void applyParameters (const Parameters& p, bool snapImmediately) noexcept;

        double sampleRate = 44100.0;
        bool   isPrepared = false;
        double thetaA = 0.0;
        double thetaB = orbitmath::pi;
        double phaseAccumA = 0.0;
        static constexpr double phaseModulus = orbitmath::twoPi * 4096.0;
        bool   symmetryLocked = true;
        double resyncStartError = 0.0;
        int    resyncSamplesRemaining = 0;
        int    resyncSamplesTotal = 1;

        /** Previous-block Dry/Wet correlation, smoothed slowly to avoid pumping. */
        float dryWetCorrelationEstimate = 0.0f;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> radiusSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> symmetrySmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> twistSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> coreSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGainSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> nullCoreMixSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> rateHzSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> autoGainAmountSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> stereoPreserveSmoothed;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> softBypassSmoothed;

        bool  nullCoreTarget = false;

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
        static constexpr double resyncDurationSeconds = 0.2;
        static constexpr float  maxWetMakeupGain = 4.0f;
        static constexpr double softBypassSeconds = 0.06;
        static constexpr double mixCorrelationTimeSeconds = 0.25;
        static constexpr float minMixPredictedPower = 0.5f; // max +3.01 dB
        static constexpr float maxMixPredictedPower = 2.0f; // max -3.01 dB

        std::atomic<float> uiThetaA { 0.0f };
        std::atomic<float> uiThetaB { static_cast<float> (orbitmath::pi) };
        std::atomic<float> uiRadius { 0.8f };
        std::atomic<float> uiCentroidX { 0.0f };
        std::atomic<float> uiCentroidZ { 0.0f };
        std::atomic<float> uiCentroidDistance { 0.0f };
        std::atomic<float> uiSymmetry { 1.0f };
        std::atomic<bool>  uiNullCoreOn { false };
        std::atomic<float> uiOutputRms { 0.0f };
        std::atomic<float> uiCorrelation { 1.0f };
        std::atomic<double> uiPhaseA { 0.0 };
        std::atomic<float> uiPhi { (float) orbitmath::pi };

        static_assert (std::atomic<double>::is_always_lock_free,
                       "uiPhaseA must be lock-free on every platform this plugin ships for");

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelixEngine)
    };
}
