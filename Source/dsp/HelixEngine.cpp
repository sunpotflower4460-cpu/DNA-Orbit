#include "HelixEngine.h"

namespace dnaorbit::dsp
{
    void HelixEngine::prepare (double newSampleRate, int maximumBlockSize, int /*maxChannelsHint*/)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;

        const double smoothParamSeconds = 0.05;   // 50 ms for standard parameters
        const double nullCoreSeconds    = 0.12;   // 120 ms mode-switch crossfade

        radiusSmoothed.reset (sampleRate, smoothParamSeconds);
        depthSmoothed.reset (sampleRate, smoothParamSeconds);
        symmetrySmoothed.reset (sampleRate, smoothParamSeconds);
        twistSmoothed.reset (sampleRate, smoothParamSeconds);
        coreSmoothed.reset (sampleRate, smoothParamSeconds);
        mixSmoothed.reset (sampleRate, smoothParamSeconds);
        outputGainSmoothed.reset (sampleRate, smoothParamSeconds);
        nullCoreMixSmoothed.reset (sampleRate, nullCoreSeconds);

        lowPassA.prepare (sampleRate);
        lowPassB.prepare (sampleRate);

        // Enough headroom for max back-delay (8ms) + max twist (20ms) + margin.
        const auto maxDelaySamples = (int) std::ceil (0.06 * sampleRate) + 16;
        delayA.setMaximumDelayInSamples (maxDelaySamples);
        delayB.setMaximumDelayInSamples (maxDelaySamples);
        maxDelaySamplesStored = (float) maxDelaySamples;

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maximumBlockSize, 1 };
        delayA.prepare (spec);
        delayB.prepare (spec);

        resyncSamplesTotal = juce::jmax (1, (int) std::round (resyncDurationSeconds * sampleRate));

        reset();
    }

    void HelixEngine::reset()
    {
        thetaA = 0.0;
        thetaB = orbitmath::pi;
        symmetryLocked = true;
        resyncStartError = 0.0;
        resyncSamplesRemaining = 0;

        lowPassA.reset();
        lowPassB.reset();
        delayA.reset();
        delayB.reset();

        radiusSmoothed.setCurrentAndTargetValue (radiusSmoothed.getCurrentValue());
        depthSmoothed.setCurrentAndTargetValue (depthSmoothed.getCurrentValue());
        symmetrySmoothed.setCurrentAndTargetValue (symmetrySmoothed.getCurrentValue());
        twistSmoothed.setCurrentAndTargetValue (twistSmoothed.getCurrentValue());
        coreSmoothed.setCurrentAndTargetValue (coreSmoothed.getCurrentValue());
        mixSmoothed.setCurrentAndTargetValue (mixSmoothed.getCurrentValue());
        outputGainSmoothed.setCurrentAndTargetValue (outputGainSmoothed.getCurrentValue());
        nullCoreMixSmoothed.setCurrentAndTargetValue (nullCoreMixSmoothed.getCurrentValue());

        uiThetaA.store (0.0f, std::memory_order_relaxed);
        uiThetaB.store ((float) orbitmath::pi, std::memory_order_relaxed);
        uiCentroidX.store (0.0f, std::memory_order_relaxed);
        uiCentroidZ.store (0.0f, std::memory_order_relaxed);
        uiCentroidDistance.store (0.0f, std::memory_order_relaxed);
    }

    void HelixEngine::setParameters (const Parameters& p) noexcept
    {
        rateHzTarget = p.rateHz;
        nullCoreTarget = p.nullCore;

        radiusSmoothed.setTargetValue (p.radius01);
        depthSmoothed.setTargetValue (p.depth01);
        symmetrySmoothed.setTargetValue (p.symmetry01);
        twistSmoothed.setTargetValue (p.twistMs);
        coreSmoothed.setTargetValue (p.core01);
        mixSmoothed.setTargetValue (p.mix01);
        outputGainSmoothed.setTargetValue (dbToGain (p.outputDb));
        nullCoreMixSmoothed.setTargetValue (p.nullCore ? 1.0f : 0.0f);

        uiNullCoreOn.store (p.nullCore, std::memory_order_relaxed);
    }

    void HelixEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept
    {
        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getWritePointer (1);
        const bool stereoIn = numInputChannels >= 2;
        const float* inL = buffer.getReadPointer (0);
        const float* inR = stereoIn ? buffer.getReadPointer (1) : nullptr;

        for (int n = 0; n < numSamples; ++n)
        {
            const float radius   = radiusSmoothed.getNextValue();
            const float depth    = depthSmoothed.getNextValue();
            const float symmetry = symmetrySmoothed.getNextValue();
            const float twistMs  = twistSmoothed.getNextValue();
            const float core     = coreSmoothed.getNextValue();
            const float mix      = mixSmoothed.getNextValue();
            const float outGain  = outputGainSmoothed.getNextValue();
            const float nullCoreAmount = nullCoreMixSmoothed.getNextValue();

            const float sampleInL = inL[n];
            const float sampleInR = stereoIn ? inR[n] : sampleInL;
            const float wetSource = stereoIn ? 0.5f * (sampleInL + sampleInR) : sampleInL;
            const float dryL = sampleInL;
            const float dryR = sampleInR;

            // --- Orbit angle update -------------------------------------------------
            const double incA = orbitmath::angularIncrement ((double) rateHzTarget, sampleRate);
            thetaA = orbitmath::wrapTwoPi (thetaA + incA);

            const bool wantsLocked = symmetry >= (float) symmetryLockThreshold;

            if (wantsLocked)
            {
                const double desired = orbitmath::wrapTwoPi (thetaA + orbitmath::pi);

                if (symmetryLocked)
                {
                    thetaB = desired;
                }
                else
                {
                    // Just transitioned from drifting to locked: capture the
                    // current error once and ramp it to zero linearly over a
                    // fixed, bounded duration (resyncSamplesTotal), so the
                    // resync reliably completes within the target window
                    // instead of trailing off exponentially forever.
                    if (resyncSamplesRemaining <= 0)
                    {
                        resyncStartError = orbitmath::shortestAngleDelta (thetaB, desired);
                        resyncSamplesRemaining = resyncSamplesTotal;
                    }

                    // shortestAngleDelta(thetaB, desired) == desired - thetaB (shortest path), so
                    // thetaB == desired - resyncStartError reproduces the original thetaB at fraction 1.
                    const double fraction = (double) resyncSamplesRemaining / (double) resyncSamplesTotal;
                    thetaB = orbitmath::wrapTwoPi (desired - resyncStartError * fraction);
                    --resyncSamplesRemaining;

                    if (resyncSamplesRemaining <= 0)
                    {
                        thetaB = desired;
                        symmetryLocked = true;
                    }
                }
            }
            else
            {
                symmetryLocked = false;
                resyncSamplesRemaining = 0; // force a fresh error capture next time we relock
                const double diff = orbitmath::rateDifferenceFactor ((double) symmetry, maxRateDifference);
                const double incB = orbitmath::angularIncrement ((double) rateHzTarget * (1.0 + diff), sampleRate);
                thetaB = orbitmath::wrapTwoPi (thetaB + incB);
            }

            // --- Strand A: position, depth cues, pan --------------------------------
            const double backAmountA = orbitmath::backAmount (thetaA);
            const float gainDbA = -maxBackAttenDb * (float) backAmountA * depth;
            const float cutoffA = frontCutoffHz + (backCutoffHz - frontCutoffHz) * (float) backAmountA * depth;
            lowPassA.setCutoffHz (cutoffA);
            const float filteredA = lowPassA.processSample (wetSource * dbToGain (gainDbA));

            const float backDelayMsA = maxBackDelayMs * (float) backAmountA * depth;
            const float delaySamplesA = std::clamp ((backDelayMsA * 0.001f) * (float) sampleRate, 0.0f, maxDelaySamplesStored - 1.0f);
            delayA.setDelay (delaySamplesA);
            delayA.pushSample (0, filteredA);
            const float delayedA = delayA.popSample (0);

            const double panA = radius * std::sin (thetaA);
            const auto gainsA = orbitmath::equalPowerPan (panA);
            const float strandAL = delayedA * (float) gainsA.left * strandGain;
            const float strandAR = delayedA * (float) gainsA.right * strandGain;

            // --- Strand B: position, depth cues, twist decorrelation, pan -----------
            const double backAmountB = orbitmath::backAmount (thetaB);
            const float gainDbB = -maxBackAttenDb * (float) backAmountB * depth;
            const float cutoffB = frontCutoffHz + (backCutoffHz - frontCutoffHz) * (float) backAmountB * depth;
            lowPassB.setCutoffHz (cutoffB);
            const float filteredB = lowPassB.processSample (wetSource * dbToGain (gainDbB));

            const float backDelayMsB = maxBackDelayMs * (float) backAmountB * depth;
            const float delaySamplesB = std::clamp (((backDelayMsB + twistMs) * 0.001f) * (float) sampleRate, 0.0f, maxDelaySamplesStored - 1.0f);
            delayB.setDelay (delaySamplesB);
            delayB.pushSample (0, filteredB);
            const float delayedB = delayB.popSample (0);

            const double panB = radius * std::sin (thetaB);
            const auto gainsB = orbitmath::equalPowerPan (panB);
            const float strandBL = delayedB * (float) gainsB.left * strandGain;
            const float strandBR = delayedB * (float) gainsB.right * strandGain;

            // --- Wet sum, Core, NULL CORE --------------------------------------------
            float wetL = strandAL + strandBL;
            float wetR = strandAR + strandBR;

            const float coreSignal = wetSource * core;
            wetL += coreSignal;
            wetR += coreSignal;

            const auto nulled = nullCoreProcess (wetL, wetR);
            wetL += (nulled.left  - wetL) * nullCoreAmount;
            wetR += (nulled.right - wetR) * nullCoreAmount;

            // --- Dry/Wet mix, output gain ---------------------------------------------
            const auto dryWet = equalPowerMix (mix);
            float finalL = dryWet.dry * dryL + dryWet.wet * wetL;
            float finalR = dryWet.dry * dryR + dryWet.wet * wetR;

            finalL *= outGain;
            finalR *= outGain;

            constexpr float antiDenormal = 1.0e-20f;
            finalL += antiDenormal; finalL -= antiDenormal;
            finalR += antiDenormal; finalR -= antiDenormal;

            outL[n] = finalL;
            outR[n] = finalR;

            if (n == numSamples - 1)
            {
                const auto posA = orbitmath::computePosition (thetaA, radius);
                const auto posB = orbitmath::computePosition (thetaB, radius);
                const auto centroid = orbitmath::computeCentroid (posA, posB);

                uiThetaA.store ((float) thetaA, std::memory_order_relaxed);
                uiThetaB.store ((float) thetaB, std::memory_order_relaxed);
                uiRadius.store (radius, std::memory_order_relaxed);
                uiCentroidX.store ((float) centroid.x, std::memory_order_relaxed);
                uiCentroidZ.store ((float) centroid.z, std::memory_order_relaxed);
                uiCentroidDistance.store ((float) centroid.distance, std::memory_order_relaxed);
                uiSymmetry.store (symmetry, std::memory_order_relaxed);
            }
        }
    }

    HelixEngine::VisualState HelixEngine::getVisualState() const noexcept
    {
        VisualState state;
        state.thetaA = uiThetaA.load (std::memory_order_relaxed);
        state.thetaB = uiThetaB.load (std::memory_order_relaxed);
        state.radius01 = uiRadius.load (std::memory_order_relaxed);
        state.centroidX = uiCentroidX.load (std::memory_order_relaxed);
        state.centroidZ = uiCentroidZ.load (std::memory_order_relaxed);
        state.centroidDistance = uiCentroidDistance.load (std::memory_order_relaxed);
        state.nullCoreOn = uiNullCoreOn.load (std::memory_order_relaxed);
        state.symmetry01 = uiSymmetry.load (std::memory_order_relaxed);
        return state;
    }
}
