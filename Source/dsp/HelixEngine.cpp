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
        rateHzSmoothed.reset (sampleRate, smoothParamSeconds);
        autoGainAmountSmoothed.reset (sampleRate, nullCoreSeconds);
        stereoPreserveSmoothed.reset (sampleRate, smoothParamSeconds);

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

        isPrepared = true;
        reset();
    }

    void HelixEngine::reset()
    {
        thetaA = 0.0;
        thetaB = orbitmath::pi;
        phaseAccumA = 0.0;
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
        rateHzSmoothed.setCurrentAndTargetValue (rateHzSmoothed.getCurrentValue());
        autoGainAmountSmoothed.setCurrentAndTargetValue (autoGainAmountSmoothed.getCurrentValue());
        stereoPreserveSmoothed.setCurrentAndTargetValue (stereoPreserveSmoothed.getCurrentValue());

        uiThetaA.store (0.0f, std::memory_order_relaxed);
        uiThetaB.store ((float) orbitmath::pi, std::memory_order_relaxed);
        uiCentroidX.store (0.0f, std::memory_order_relaxed);
        uiCentroidZ.store (0.0f, std::memory_order_relaxed);
        uiCentroidDistance.store (0.0f, std::memory_order_relaxed);
        uiOutputRms.store (0.0f, std::memory_order_relaxed);
        uiCorrelation.store (1.0f, std::memory_order_relaxed);
        uiPhaseA.store (0.0, std::memory_order_relaxed);
        uiPhi.store ((float) orbitmath::pi, std::memory_order_relaxed);
    }

    namespace
    {
        /**
         * Falls back to a safe default for any non-finite value. A host that
         * ever automates a parameter to NaN/Inf (malformed automation data, a
         * buggy upstream plugin corrupting shared state, etc.) would otherwise
         * latch thetaA/thetaB - which depend only on rateHz, not audio - to
         * NaN permanently, since nothing else in the engine would ever
         * overwrite them back to a finite value.
         */
        float sanitizeParam (float value, float fallback) noexcept
        {
            return std::isfinite (value) ? value : fallback;
        }
    }

    void HelixEngine::setParameters (const Parameters& p) noexcept
    {
        applyParameters (p, false);
    }

    void HelixEngine::primeParameters (const Parameters& p) noexcept
    {
        applyParameters (p, true);
    }

    void HelixEngine::applyParameters (const Parameters& p, bool snapImmediately) noexcept
    {
        const float rateHz     = sanitizeParam (p.rateHz, 0.12f);
        const float radius01   = sanitizeParam (p.radius01, 0.8f);
        const float depth01    = sanitizeParam (p.depth01, 0.55f);
        const float symmetry01 = sanitizeParam (p.symmetry01, 1.0f);
        const float twistMs    = sanitizeParam (p.twistMs, 5.0f);
        const float core01     = sanitizeParam (p.core01, 0.0f);
        const float mix01      = sanitizeParam (p.mix01, 0.35f);
        const float outputDb   = sanitizeParam (p.outputDb, 0.0f);
        const float stereoPreserve01 = std::clamp (sanitizeParam (p.stereoPreserve01, 0.0f), 0.0f, 1.0f);

        nullCoreTarget = p.nullCore;

        const float autoGainAmount = p.autoGain ? 1.0f : 0.0f;
        const float nullCoreAmount = p.nullCore ? 1.0f : 0.0f;
        const float outputGain = dbToGain (outputDb);

        if (snapImmediately)
        {
            // Called once right after prepare(): every SmoothedValue defaults to
            // a current value of 0, so without this every parameter - including
            // Output and Mix - would audibly ramp in from silence over the
            // first 50-120ms after every prepareToPlay() (plugin load, sample
            // rate change), regardless of what the host had them set to.
            rateHzSmoothed.setCurrentAndTargetValue (rateHz);
            autoGainAmountSmoothed.setCurrentAndTargetValue (autoGainAmount);
            radiusSmoothed.setCurrentAndTargetValue (radius01);
            depthSmoothed.setCurrentAndTargetValue (depth01);
            symmetrySmoothed.setCurrentAndTargetValue (symmetry01);
            twistSmoothed.setCurrentAndTargetValue (twistMs);
            coreSmoothed.setCurrentAndTargetValue (core01);
            mixSmoothed.setCurrentAndTargetValue (mix01);
            outputGainSmoothed.setCurrentAndTargetValue (outputGain);
            nullCoreMixSmoothed.setCurrentAndTargetValue (nullCoreAmount);
            stereoPreserveSmoothed.setCurrentAndTargetValue (stereoPreserve01);
        }
        else
        {
            rateHzSmoothed.setTargetValue (rateHz);
            autoGainAmountSmoothed.setTargetValue (autoGainAmount);
            radiusSmoothed.setTargetValue (radius01);
            depthSmoothed.setTargetValue (depth01);
            symmetrySmoothed.setTargetValue (symmetry01);
            twistSmoothed.setTargetValue (twistMs);
            coreSmoothed.setTargetValue (core01);
            mixSmoothed.setTargetValue (mix01);
            outputGainSmoothed.setTargetValue (outputGain);
            nullCoreMixSmoothed.setTargetValue (nullCoreAmount);
            stereoPreserveSmoothed.setTargetValue (stereoPreserve01);
        }

        uiNullCoreOn.store (p.nullCore, std::memory_order_relaxed);
    }

    void HelixEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept
    {
        // A host that violates the prepare-before-process contract would
        // otherwise hit the delay lines' zero-channel internal buffer - see the
        // comment on process() in the header. Leaving the buffer untouched
        // here behaves like a pass-through, which is a safe fallback for a
        // situation that should never occur in the first place.
        if (! isPrepared)
            return;

        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getWritePointer (1);
        const bool stereoIn = numInputChannels >= 2;
        const float* inL = buffer.getReadPointer (0);
        const float* inR = stereoIn ? buffer.getReadPointer (1) : nullptr;

        // Block accumulators for the UI meters (computed on the output).
        double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;

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
            const float rateHz   = rateHzSmoothed.getNextValue();
            const float autoGainAmount = autoGainAmountSmoothed.getNextValue();
            const float stereoPreserve = stereoPreserveSmoothed.getNextValue();

            // Sanitized at the single point audio enters the engine: the two
            // one-pole filters below are recursive (state depends on the
            // previous sample), so a single non-finite input sample - a bad
            // upstream plugin, a glitching host - would otherwise latch their
            // state to NaN forever, with nothing downstream ever able to clear
            // it. Substituting silence here keeps every later stage finite by
            // induction, since reset() guarantees the filter/delay state
            // starts finite and every operation on finite, bounded values
            // (sin/cos/exp/clamp) stays finite.
            const float sampleInL = std::isfinite (inL[n]) ? inL[n] : 0.0f;
            const float sampleInR = stereoIn ? (std::isfinite (inR[n]) ? inR[n] : 0.0f) : sampleInL;
            const float dryL = sampleInL;
            const float dryR = sampleInR;

            // --- Stereo Preserve: centred DNA orbit + stationary Side bed ---------
            // The moving DNA itself is always driven by the common Mid signal.
            // Therefore both antipodal strands carry the same programme material:
            // Symmetry 100% means not only a geometric midpoint of zero, but also
            // avoids content-dependent left/right bias caused by feeding unrelated
            // L/R material into the two moving strands.
            //
            // The original stereo identity is retained separately as a pure Side
            // bed (L += p*S, R -= p*S). A pure Side signal has equal energy on both
            // output channels and zero Mid, so it preserves width without moving
            // the DNA's centre. At p=0 this is exactly the schema-1 signal path.
            const float mid  = stereoIn ? 0.5f * (sampleInL + sampleInR) : sampleInL;
            const float side = stereoIn ? 0.5f * (sampleInL - sampleInR) : 0.0f;

            // --- Orbit angle update -------------------------------------------------
            const double incA = orbitmath::angularIncrement ((double) rateHz, sampleRate);
            thetaA = orbitmath::wrapTwoPi (thetaA + incA);
            phaseAccumA = std::fmod (phaseAccumA + incA, phaseModulus);

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
                const double incB = orbitmath::angularIncrement ((double) rateHz * (1.0 + diff), sampleRate);
                thetaB = orbitmath::wrapTwoPi (thetaB + incB);
            }

            // --- Strand A: position, depth cues, pan --------------------------------
            const double backAmountA = orbitmath::backAmount (thetaA);
            const float gainDbA = -maxBackAttenDb * (float) backAmountA * depth;
            const float backGainA = dbToGain (gainDbA);
            const float cutoffA = frontCutoffHz + (backCutoffHz - frontCutoffHz) * (float) backAmountA * depth;
            lowPassA.setCutoffHz (cutoffA);
            const float filteredA = lowPassA.processSample (mid * backGainA);

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
            const float backGainB = dbToGain (gainDbB);
            const float cutoffB = frontCutoffHz + (backCutoffHz - frontCutoffHz) * (float) backAmountB * depth;
            lowPassB.setCutoffHz (cutoffB);
            const float filteredB = lowPassB.processSample (mid * backGainB);

            const float backDelayMsB = maxBackDelayMs * (float) backAmountB * depth;
            const float delaySamplesB = std::clamp (((backDelayMsB + twistMs) * 0.001f) * (float) sampleRate, 0.0f, maxDelaySamplesStored - 1.0f);
            delayB.setDelay (delaySamplesB);
            delayB.pushSample (0, filteredB);
            const float delayedB = delayB.popSample (0);

            const double panB = radius * std::sin (thetaB);
            const auto gainsB = orbitmath::equalPowerPan (panB);
            const float strandBL = delayedB * (float) gainsB.left * strandGain;
            const float strandBR = delayedB * (float) gainsB.right * strandGain;

            // --- Centred orbit wet + Core --------------------------------------------
            float wetL = strandAL + strandBL + mid * core;
            float wetR = strandAR + strandBR + mid * core;

            // --- Geometry-based Mid-orbit level matching -----------------------------
            // This prediction remains valid because the two strands and Core are
            // deliberately driven by the same Mid source. The Side bed is added only
            // after this compensation, so original stereo width is neither boosted by
            // the orbit make-up gain nor included in a false same-source assumption.
            const float msPerSample = 1000.0f / (float) sampleRate;
            const auto coherence = [] (float deltaMs) { return std::exp (-std::abs (deltaMs) / 4.0f); };

            const float delayMsA = delaySamplesA * msPerSample;
            const float delayMsB = delaySamplesB * msPerSample;
            const float cAB = coherence (delayMsA - delayMsB);
            const float cAC = coherence (delayMsA);
            const float cBC = coherence (delayMsB);

            const float aL = strandGain * backGainA * (float) gainsA.left;
            const float aR = strandGain * backGainA * (float) gainsA.right;
            const float bL = strandGain * backGainB * (float) gainsB.left;
            const float bR = strandGain * backGainB * (float) gainsB.right;

            const float powerL = aL * aL + bL * bL + core * core
                               + 2.0f * (aL * bL * cAB + aL * core * cAC + bL * core * cBC);
            const float powerR = aR * aR + bR * bR + core * core
                               + 2.0f * (aR * bR * cAB + aR * core * cAC + bR * core * cBC);

            const float wetPower = powerL + powerR;
            const float makeupTarget = wetPower > 1.0e-9f
                                     ? std::clamp (std::sqrt (2.0f / wetPower), 1.0f / maxWetMakeupGain, maxWetMakeupGain)
                                     : 1.0f;
            const float wetMakeup = 1.0f + (makeupTarget - 1.0f) * autoGainAmount;
            wetL *= wetMakeup;
            wetR *= wetMakeup;

            // Preserve the original Side independently of the orbit. This fixes the
            // anti-phase-silence failure while keeping the moving DNA content-centred.
            const float sideBed = side * stereoPreserve;
            wetL += sideBed;
            wetR -= sideBed;

            // NULL CORE remains an explicitly Side-only creative mode. Apply it after
            // the preserve bed so the definition stays literal for the complete Wet.
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

            sumLL += (double) finalL * finalL;
            sumRR += (double) finalR * finalR;
            sumLR += (double) finalL * finalR;

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
                uiPhaseA.store (phaseAccumA, std::memory_order_relaxed);
                uiPhi.store ((float) orbitmath::wrapTwoPi (thetaB - thetaA), std::memory_order_relaxed);
            }
        }

        // --- Publish meters for the UI (once per block) ------------------------------
        if (numSamples > 0)
        {
            const double invN = 1.0 / (double) numSamples;
            const double meanLL = sumLL * invN;
            const double meanRR = sumRR * invN;

            const float rms = (float) std::sqrt (0.5 * (meanLL + meanRR));
            uiOutputRms.store (std::isfinite (rms) ? rms : 0.0f, std::memory_order_relaxed);

            // Normalised L/R correlation: +1 mono, 0 uncorrelated, -1 out of phase.
            // Undefined for silence, so hold at +1 (mono-safe) rather than dividing by zero.
            const double denom = std::sqrt (meanLL * meanRR);
            const float correlation = denom > 1.0e-12
                                     ? (float) std::clamp ((sumLR * invN) / denom, -1.0, 1.0)
                                     : 1.0f;
            uiCorrelation.store (std::isfinite (correlation) ? correlation : 1.0f, std::memory_order_relaxed);
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
        state.outputRms = uiOutputRms.load (std::memory_order_relaxed);
        state.correlation = uiCorrelation.load (std::memory_order_relaxed);
        state.phaseA = uiPhaseA.load (std::memory_order_relaxed);
        state.phi = uiPhi.load (std::memory_order_relaxed);
        return state;
    }
}
