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

            /**
             * 0 = both strands (and Core) are fed from a shared Mid downmix
             * (the schema-1 behaviour: anti-phase stereo input collapses Wet
             * to silence). 1 = Strand A/B are fed directly from L/R. Defaults
             * to 0 here - the schema-1-compatible, source-independent
             * default - so any caller that forgets to set it explicitly gets
             * the old behaviour rather than a silent change; the actual
             * product default of 70% lives in Parameters.h and is applied by
             * PluginProcessor.
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
            /**
             * Unwrapped total orbit phase of strand A, wrapped only at a large
             * multiple of 2*pi. The UI reconstructs its history from this: a wrapped
             * angle would be ambiguous to unwrap after a message-thread stall (at
             * 4 Hz a 125 ms stall already exceeds pi), which would make the helix
             * jump or briefly run backwards.
             */
            double phaseA = 0.0;
            /**
             * Relative phase thetaB - thetaA, wrapped to [0, 2*pi). Publishing the
             * relative phase rather than thetaB keeps the antipodal invariant exact:
             * phi == pi means the strand midpoint is zero, whatever else drifts.
             */
            float phi = 0.0f;

            float thetaA = 0.0f;
            float thetaB = 0.0f;
            float radius01 = 0.0f;
            float centroidX = 0.0f;
            float centroidZ = 0.0f;
            float centroidDistance = 0.0f;
            float symmetry01 = 1.0f;
            bool  nullCoreOn = false;

            /** Output RMS (0..1-ish), used to make the visuals react to the audio. */
            float outputRms = 0.0f;
            /** L/R correlation of the output: +1 = mono, 0 = uncorrelated, -1 = out of phase. */
            float correlation = 1.0f;
        };

        VisualState getVisualState() const noexcept;

    private:
        void applyParameters (const Parameters& p, bool snapImmediately) noexcept;

        double sampleRate = 44100.0;
        bool   isPrepared = false;

        // Orbit state (double precision to avoid long-run drift).
        double thetaA = 0.0;
        double thetaB = orbitmath::pi;

        /**
         * Unwrapped phase accumulator for the UI, wrapped at a large multiple of
         * 2*pi so it never loses precision (double epsilon at this magnitude is
         * ~4e-12 rad) and so the UI can unwrap it unambiguously.
         */
        double phaseAccumA = 0.0;
        static constexpr double phaseModulus = orbitmath::twoPi * 4096.0;
        bool   symmetryLocked = true;

        // Fixed-duration linear resync ramp used when Symmetry returns to 100%
        // (see process()). Bounded and deterministic, unlike an exponential
        // tail, so it reliably completes within resyncDurationSeconds.
        double resyncStartError = 0.0;
        int    resyncSamplesRemaining = 0;
        int    resyncSamplesTotal = 1;

        // Smoothed parameters.
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
        static constexpr double resyncDurationSeconds = 0.2; // within the 100-300ms spec window
        static constexpr float  maxWetMakeupGain = 4.0f;     // +12 dB ceiling

        // Published for the UI thread; written once per block.
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

        // std::atomic<double> is not guaranteed lock-free by the standard; on a
        // target without native 8-byte atomics it would silently fall back to a
        // lock, breaking the audio thread's no-locking guarantee. True on every
        // platform this plugin currently ships for (x86-64, ARM64); fails loudly
        // at compile time rather than silently at runtime if that ever changes.
        static_assert (std::atomic<double>::is_always_lock_free,
                       "uiPhaseA must be lock-free on every platform this plugin ships for");

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HelixEngine)
    };
}
