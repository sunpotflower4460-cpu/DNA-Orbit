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
            bool  autoGain   = true;
            bool  softBypass = false;

            /**
             * Amount of the original Side signal retained as a stationary
             * stereo bed around the centred DNA orbit. The two moving strands
             * themselves are always driven by the shared Mid signal, so at
             * Symmetry 100% their centre cannot be biased by unrelated L/R
             * programme energy. 0 reproduces the schema-1 Mid-only path; 1
             * restores the input Side at unity (L += Side, R -= Side).
             *
             * The engine-level default remains 0 for source compatibility.
             * The product default for fresh instances is declared in
             * Parameters.h and legacy saved projects migrate to 0.
             */
            float stereoPreserve01 = 0.0f;
        };

        void prepare (double newSampleRate, int maximumBlockSize, int maxChannelsHint);
        void reset();

        /** Call once per block before process(). Ramps smoothly toward newParams. */
        void setParameters (const Parameters& newParams) noexcept;

        /**
         * Call once, right after prepare(), with the current parameter snapshot.
         * Unlike setParameters(), this snaps every smoothed value immediately
         * instead of ramping - otherwise every parameter would audibly fade in
         * from its SmoothedValue default (0) over the first smoothing window
         * after every prepare() call.
         */
        void primeParameters (const Parameters& newParams) noexcept;

        /**
         * Processes buffer in place. `buffer` must contain exactly 2 channels;
         * the first `numInputChannels` channels hold valid input data (1 for
         * mono-in, 2 for stereo-in) and any remaining channels are assumed to
         * already be cleared by the caller.
         *
         * If called before prepare() (a host contract violation, but hosts do
         * have bugs), this is a safe no-op rather than touching the delay
         * lines: juce::dsp::DelayLine's internal buffer has zero channels
         * until prepare() sizes it, and AudioBuffer::setSample's bounds check
         * is assertion-only, so writing to channel 0 of a zero-channel buffer
         * would dereference an invalid pointer in a Release build.
         */
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
