#include "HelixEngine.h"

namespace dnaorbit::dsp
{
    namespace
    {
        float sanitizeParam (float value, float fallback) noexcept
        {
            return std::isfinite (value) ? value : fallback;
        }

        float interpolate (float a, float b, float amount) noexcept
        {
            return a + (b - a) * amount;
        }

        double positiveFmod (double value, double modulus) noexcept
        {
            double result = std::fmod (value, modulus);
            if (result < 0.0)
                result += modulus;
            return result;
        }
    }

    void HelixEngine::prepare (double newSampleRate, int maximumBlockSize, int /*maxChannelsHint*/)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        constexpr double standardSmoothing = 0.05;
        constexpr double nullCoreSmoothing = 0.12;

        radiusSmoothed.reset (sampleRate, standardSmoothing);
        depthSmoothed.reset (sampleRate, standardSmoothing);
        symmetrySmoothed.reset (sampleRate, standardSmoothing);
        twistSmoothed.reset (sampleRate, standardSmoothing);
        coreSmoothed.reset (sampleRate, standardSmoothing);
        mixSmoothed.reset (sampleRate, standardSmoothing);
        outputGainSmoothed.reset (sampleRate, standardSmoothing);
        nullCoreMixSmoothed.reset (sampleRate, nullCoreSmoothing);
        rateHzSmoothed.reset (sampleRate, standardSmoothing);
        autoGainAmountSmoothed.reset (sampleRate, nullCoreSmoothing);
        stereoPreserveSmoothed.reset (sampleRate, standardSmoothing);
        softBypassSmoothed.reset (sampleRate, softBypassSeconds);
        bassAnchorSmoothed.reset (sampleRate, standardSmoothing);
        characterSmoothed.reset (sampleRate, standardSmoothing);

        crossoverL.prepare (sampleRate);
        crossoverR.prepare (sampleRate);
        lowPassA.prepare (sampleRate);
        lowPassB.prepare (sampleRate);

        const auto maxDelaySamples = (int) std::ceil (0.08 * sampleRate) + 16;
        delayA.setMaximumDelayInSamples (maxDelaySamples);
        delayB.setMaximumDelayInSamples (maxDelaySamples);
        maxDelaySamplesStored = (float) maxDelaySamples;

        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maximumBlockSize, 1 };
        delayA.prepare (spec);
        delayB.prepare (spec);

        resyncSamplesTotal = juce::jmax (1, (int) std::round (resyncDurationSeconds * sampleRate));
        hostCorrectionSamplesTotal = juce::jmax (1, (int) std::round (hostCorrectionSeconds * sampleRate));
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
        dryWetCorrelationEstimate = 0.0f;
        hostLockWasActive = false;
        expectedNextHostPhaseA = 0.0;
        expectedNextHostPhaseB = orbitmath::pi;
        hostCorrectionStartA = 0.0;
        hostCorrectionStartB = 0.0;
        hostCorrectionSamplesRemaining = 0;
        controlSamplesRemaining = 0;

        crossoverL.reset();
        crossoverR.reset();
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
        softBypassSmoothed.setCurrentAndTargetValue (softBypassSmoothed.getCurrentValue());
        bassAnchorSmoothed.setCurrentAndTargetValue (bassAnchorSmoothed.getCurrentValue());
        characterSmoothed.setCurrentAndTargetValue (characterSmoothed.getCurrentValue());

        uiThetaA.store (0.0f, std::memory_order_relaxed);
        uiThetaB.store ((float) orbitmath::pi, std::memory_order_relaxed);
        uiCentroidX.store (0.0f, std::memory_order_relaxed);
        uiCentroidZ.store (0.0f, std::memory_order_relaxed);
        uiCentroidDistance.store (0.0f, std::memory_order_relaxed);
        uiOutputRms.store (0.0f, std::memory_order_relaxed);
        uiCorrelation.store (1.0f, std::memory_order_relaxed);
        uiPhaseA.store (0.0, std::memory_order_relaxed);
        uiPhi.store ((float) orbitmath::pi, std::memory_order_relaxed);
        uiHostPhaseLocked.store (false, std::memory_order_relaxed);
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
        const float rateHz = std::clamp (sanitizeParam (p.rateHz, 0.12f), 0.0f, 20.0f);
        const float radius = std::clamp (sanitizeParam (p.radius01, 0.8f), 0.0f, 1.0f);
        const float depth = std::clamp (sanitizeParam (p.depth01, 0.55f), 0.0f, 1.0f);
        const float symmetry = std::clamp (sanitizeParam (p.symmetry01, 1.0f), 0.0f, 1.0f);
        const float twist = std::clamp (sanitizeParam (p.twistMs, 5.0f), 0.0f, 20.0f);
        const float core = std::clamp (sanitizeParam (p.core01, 0.0f), 0.0f, 1.0f);
        const float mix = std::clamp (sanitizeParam (p.mix01, 0.35f), 0.0f, 1.0f);
        const float outputGain = dbToGain (std::clamp (sanitizeParam (p.outputDb, 0.0f), -60.0f, 24.0f));
        const float preserve = std::clamp (sanitizeParam (p.stereoPreserve01, 0.0f), 0.0f, 1.0f);
        const float anchor = std::clamp (sanitizeParam (p.bassAnchorHz, 20.0f), 20.0f, 500.0f);
        const float character = std::clamp ((float) p.character, 0.0f, 2.0f);

        auto set = [snapImmediately] (auto& smoother, float value)
        {
            if (snapImmediately)
                smoother.setCurrentAndTargetValue (value);
            else
                smoother.setTargetValue (value);
        };

        set (rateHzSmoothed, rateHz);
        set (radiusSmoothed, radius);
        set (depthSmoothed, depth);
        set (symmetrySmoothed, symmetry);
        set (twistSmoothed, twist);
        set (coreSmoothed, core);
        set (mixSmoothed, mix);
        set (outputGainSmoothed, outputGain);
        set (autoGainAmountSmoothed, p.autoGain ? 1.0f : 0.0f);
        set (nullCoreMixSmoothed, p.nullCore ? 1.0f : 0.0f);
        set (stereoPreserveSmoothed, preserve);
        set (softBypassSmoothed, p.softBypass ? 1.0f : 0.0f);
        set (bassAnchorSmoothed, anchor);
        set (characterSmoothed, character);

        phaseModeTarget = std::clamp (p.phaseMode, 0, 2);
        startPhaseDegreesTarget = std::clamp (sanitizeParam (p.startPhaseDegrees, 0.0f), 0.0f, 360.0f);
        reverseDirectionTarget = p.reverseDirection;
        transportPlayingTarget = p.transportPlaying;
        transportJustStartedTarget = p.transportJustStarted;
        hostPositionValidTarget = p.hostPositionValid && std::isfinite (p.hostPpqPosition);
        hostPpqPositionTarget = hostPositionValidTarget ? p.hostPpqPosition : 0.0;
        cycleBeatsTarget = std::isfinite (p.cycleBeats) && p.cycleBeats > 0.0 ? p.cycleBeats : 4.0;

        uiNullCoreOn.store (p.nullCore, std::memory_order_relaxed);
        uiBassAnchorHz.store (anchor, std::memory_order_relaxed);
    }

    void HelixEngine::process (juce::AudioBuffer<float>& buffer, int numInputChannels) noexcept
    {
        if (! isPrepared || buffer.getNumChannels() < 2)
            return;

        const int numSamples = buffer.getNumSamples();
        auto* outL = buffer.getWritePointer (0);
        auto* outR = buffer.getWritePointer (1);
        const bool stereoIn = numInputChannels >= 2;
        const float* inL = buffer.getReadPointer (0);
        const float* inR = stereoIn ? buffer.getReadPointer (1) : nullptr;

        const double directionSign = reverseDirectionTarget ? -1.0 : 1.0;
        const double startRadians = (double) startPhaseDegreesTarget * orbitmath::pi / 180.0;
        const bool useHostLock = phaseModeTarget == 2 && hostPositionValidTarget && cycleBeatsTarget > 0.0;
        const bool retriggerNow = phaseModeTarget == 1 && transportJustStartedTarget;
        const double hostRate = (double) rateHzSmoothed.getTargetValue();
        const double hostIncrement = transportPlayingTarget
            ? orbitmath::angularIncrement (hostRate, sampleRate)
            : 0.0;
        const double hostPpqCycles = hostPpqPositionTarget / cycleBeatsTarget;
        const double hostRawStartA = startRadians + directionSign * orbitmath::twoPi * hostPpqCycles;
        const double hostBlockPhaseA = orbitmath::wrapTwoPi (hostRawStartA);

        const float targetSymmetry = symmetrySmoothed.getTargetValue();
        const bool targetLocked = targetSymmetry >= (float) symmetryLockThreshold;
        const double targetDiff = orbitmath::rateDifferenceFactor ((double) targetSymmetry, maxRateDifference);
        const double targetBScale = targetLocked ? 1.0 : 1.0 + targetDiff;
        const double hostRawStartB = startRadians + orbitmath::pi
                                   + directionSign * orbitmath::twoPi * hostPpqCycles * targetBScale;
        const double hostBlockPhaseB = orbitmath::wrapTwoPi (hostRawStartB);

        if (useHostLock)
        {
            if (! hostLockWasActive || transportJustStartedTarget)
            {
                thetaA = hostBlockPhaseA;
                thetaB = hostBlockPhaseB;
                symmetryLocked = targetLocked;
                hostCorrectionStartA = 0.0;
                hostCorrectionStartB = 0.0;
                hostCorrectionSamplesRemaining = 0;
            }
            else
            {
                const double discontinuityA = std::abs (
                    orbitmath::shortestAngleDelta (expectedNextHostPhaseA, hostBlockPhaseA));
                const double discontinuityB = std::abs (
                    orbitmath::shortestAngleDelta (expectedNextHostPhaseB, hostBlockPhaseB));
                const double toleranceA = std::max (0.02, std::abs (hostIncrement) * 8.0);
                const double toleranceB = std::max (0.02, std::abs (hostIncrement * targetBScale) * 8.0);

                if (discontinuityA > toleranceA || discontinuityB > toleranceB)
                {
                    hostCorrectionStartA = orbitmath::shortestAngleDelta (hostBlockPhaseA, thetaA);
                    hostCorrectionStartB = orbitmath::shortestAngleDelta (hostBlockPhaseB, thetaB);
                    hostCorrectionSamplesRemaining = hostCorrectionSamplesTotal;
                }
            }

            expectedNextHostPhaseA = orbitmath::wrapTwoPi (
                hostBlockPhaseA + directionSign * hostIncrement * (double) numSamples);
            expectedNextHostPhaseB = orbitmath::wrapTwoPi (
                hostBlockPhaseB + directionSign * hostIncrement * targetBScale * (double) numSamples);
            hostLockWasActive = true;
        }
        else
        {
            hostLockWasActive = false;
            hostCorrectionSamplesRemaining = 0;

            if (retriggerNow)
            {
                thetaA = orbitmath::wrapTwoPi (startRadians);
                thetaB = orbitmath::wrapTwoPi (thetaA + orbitmath::pi);
                phaseAccumA = positiveFmod (startRadians, phaseModulus);
                symmetryLocked = true;
                resyncSamplesRemaining = 0;
            }
        }

        uiHostPhaseLocked.store (useHostLock, std::memory_order_relaxed);

        double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
        double sumDryPower = 0.0, sumWetPower = 0.0, sumDryWet = 0.0;

        for (int n = 0; n < numSamples; ++n)
        {
            const float radius = radiusSmoothed.getNextValue();
            const float depth = depthSmoothed.getNextValue();
            const float symmetry = symmetrySmoothed.getNextValue();
            const float twistMs = twistSmoothed.getNextValue();
            const float core = coreSmoothed.getNextValue();
            const float mix = mixSmoothed.getNextValue();
            const float outGain = outputGainSmoothed.getNextValue();
            const float nullCoreAmount = nullCoreMixSmoothed.getNextValue();
            const float rateHz = rateHzSmoothed.getNextValue();
            const float autoGainAmount = autoGainAmountSmoothed.getNextValue();
            const float stereoPreserve = stereoPreserveSmoothed.getNextValue();
            const float softBypass = softBypassSmoothed.getNextValue();
            const float bassAnchorHz = bassAnchorSmoothed.getNextValue();
            const float character = characterSmoothed.getNextValue();

            const float sampleInL = std::isfinite (inL[n]) ? inL[n] : 0.0f;
            const float sampleInR = stereoIn && std::isfinite (inR[n]) ? inR[n] : sampleInL;
            const float dryL = sampleInL;
            const float dryR = sampleInR;
            const double incA = orbitmath::angularIncrement ((double) rateHz, sampleRate);
            double correctionFraction = 0.0;

            if (useHostLock)
            {
                if (hostCorrectionSamplesRemaining > 0)
                    correctionFraction = (double) hostCorrectionSamplesRemaining
                                       / (double) hostCorrectionSamplesTotal;

                const double nominalA = orbitmath::wrapTwoPi (
                    hostBlockPhaseA + directionSign * hostIncrement * (double) n);
                thetaA = orbitmath::wrapTwoPi (
                    nominalA + hostCorrectionStartA * correctionFraction);
                phaseAccumA = positiveFmod (
                    hostRawStartA + directionSign * hostIncrement * (double) n
                    + hostCorrectionStartA * correctionFraction,
                    phaseModulus);
            }
            else if (! (retriggerNow && n == 0))
            {
                thetaA = orbitmath::wrapTwoPi (thetaA + directionSign * incA);
                phaseAccumA = positiveFmod (phaseAccumA + directionSign * incA, phaseModulus);
            }

            const bool wantsLocked = symmetry >= (float) symmetryLockThreshold;
            const bool effectiveLocked = useHostLock ? targetLocked : wantsLocked;

            if (useHostLock)
            {
                const double nominalB = orbitmath::wrapTwoPi (
                    hostBlockPhaseB + directionSign * hostIncrement * targetBScale * (double) n);
                thetaB = orbitmath::wrapTwoPi (
                    nominalB + hostCorrectionStartB * correctionFraction);
                symmetryLocked = targetLocked;
                resyncSamplesRemaining = 0;

                if (hostCorrectionSamplesRemaining > 0)
                    --hostCorrectionSamplesRemaining;
            }
            else if (wantsLocked)
            {
                const double desired = orbitmath::wrapTwoPi (thetaA + orbitmath::pi);

                if (symmetryLocked)
                {
                    thetaB = desired;
                }
                else
                {
                    if (resyncSamplesRemaining <= 0)
                    {
                        resyncStartError = orbitmath::shortestAngleDelta (thetaB, desired);
                        resyncSamplesRemaining = resyncSamplesTotal;
                    }

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
                resyncSamplesRemaining = 0;
                const double diff = orbitmath::rateDifferenceFactor ((double) symmetry, maxRateDifference);
                const double incB = orbitmath::angularIncrement (
                    (double) rateHz * (1.0 + diff), sampleRate);
                thetaB = orbitmath::wrapTwoPi (thetaB + directionSign * incB);
            }

            const double sinA = std::sin (thetaA);
            const double cosA = std::cos (thetaA);
            const double sinB = effectiveLocked ? -sinA : std::sin (thetaB);
            const double cosB = effectiveLocked ? -cosA : std::cos (thetaB);
            const double backAmountA = 0.5 * (1.0 - cosA);
            const double backAmountB = 0.5 * (1.0 - cosB);

            const bool updateControls = controlSamplesRemaining <= 0;
            if (updateControls)
            {
                crossoverL.setCutoffHz (bassAnchorHz);
                crossoverR.setCutoffHz (bassAnchorHz);

                const float c = std::clamp (character, 0.0f, 2.0f);
                if (c <= 1.0f)
                {
                    cachedMaxBackAttenDb = interpolate (4.0f, 6.0f, c);
                    cachedBackCutoffHz = interpolate (5000.0f, 8000.0f, c);
                    cachedMaxBackDelayMs = interpolate (8.0f, 10.0f, c);
                }
                else
                {
                    const float t = c - 1.0f;
                    cachedMaxBackAttenDb = interpolate (6.0f, 8.0f, t);
                    cachedBackCutoffHz = interpolate (8000.0f, 3500.0f, t);
                    cachedMaxBackDelayMs = interpolate (10.0f, 14.0f, t);
                }
            }

            float lowL = 0.0f, highL = 0.0f, lowR = 0.0f, highR = 0.0f;
            crossoverL.processSample (sampleInL, lowL, highL);
            crossoverR.processSample (sampleInR, lowR, highR);

            const float lowMid = stereoIn ? 0.5f * (lowL + lowR) : lowL;
            const float mid = stereoIn ? 0.5f * (highL + highR) : highL;
            const float side = stereoIn ? 0.5f * (highL - highR) : 0.0f;

            const float backDelayMsA = cachedMaxBackDelayMs * (float) backAmountA * depth;
            const float backDelayMsB = cachedMaxBackDelayMs * (float) backAmountB * depth;
            const float delaySamplesA = std::clamp (
                backDelayMsA * 0.001f * (float) sampleRate,
                0.0f, maxDelaySamplesStored - 1.0f);
            const float delaySamplesB = std::clamp (
                (backDelayMsB + twistMs) * 0.001f * (float) sampleRate,
                0.0f, maxDelaySamplesStored - 1.0f);

            if (updateControls)
            {
                cachedBackGainA = dbToGain (-cachedMaxBackAttenDb * (float) backAmountA * depth);
                cachedBackGainB = dbToGain (-cachedMaxBackAttenDb * (float) backAmountB * depth);

                const float cutoffA = frontCutoffHz
                    + (cachedBackCutoffHz - frontCutoffHz) * (float) backAmountA * depth;
                const float cutoffB = frontCutoffHz
                    + (cachedBackCutoffHz - frontCutoffHz) * (float) backAmountB * depth;
                lowPassA.setCutoffHz (cutoffA);
                lowPassB.setCutoffHz (cutoffB);

                const float msPerSample = 1000.0f / (float) sampleRate;
                const float delayMsA = delaySamplesA * msPerSample;
                const float delayMsB = delaySamplesB * msPerSample;
                auto coherence = [] (float deltaMs)
                {
                    return std::exp (-std::abs (deltaMs) / 4.0f);
                };
                cachedCoherenceAB = coherence (delayMsA - delayMsB);
                cachedCoherenceAC = coherence (delayMsA);
                cachedCoherenceBC = coherence (delayMsB);
                controlSamplesRemaining = controlIntervalSamples;
            }
            --controlSamplesRemaining;

            const float filteredA = lowPassA.processSample (mid * cachedBackGainA);
            delayA.setDelay (delaySamplesA);
            delayA.pushSample (0, filteredA);
            const float delayedA = delayA.popSample (0);
            const auto gainsA = orbitmath::equalPowerPan (radius * sinA);
            const float strandAL = delayedA * (float) gainsA.left * strandGain;
            const float strandAR = delayedA * (float) gainsA.right * strandGain;

            const float filteredB = lowPassB.processSample (mid * cachedBackGainB);
            delayB.setDelay (delaySamplesB);
            delayB.pushSample (0, filteredB);
            const float delayedB = delayB.popSample (0);
            const auto gainsB = orbitmath::equalPowerPan (radius * sinB);
            const float strandBL = delayedB * (float) gainsB.left * strandGain;
            const float strandBR = delayedB * (float) gainsB.right * strandGain;

            float wetL = strandAL + strandBL + mid * core;
            float wetR = strandAR + strandBR + mid * core;

            const float aL = strandGain * cachedBackGainA * (float) gainsA.left;
            const float aR = strandGain * cachedBackGainA * (float) gainsA.right;
            const float bL = strandGain * cachedBackGainB * (float) gainsB.left;
            const float bR = strandGain * cachedBackGainB * (float) gainsB.right;
            const float powerL = aL * aL + bL * bL + core * core
                               + 2.0f * (aL * bL * cachedCoherenceAB
                                       + aL * core * cachedCoherenceAC
                                       + bL * core * cachedCoherenceBC);
            const float powerR = aR * aR + bR * bR + core * core
                               + 2.0f * (aR * bR * cachedCoherenceAB
                                       + aR * core * cachedCoherenceAC
                                       + bR * core * cachedCoherenceBC);

            const float wetPower = powerL + powerR;
            const float makeupTarget = wetPower > 1.0e-9f
                ? std::clamp (std::sqrt (2.0f / wetPower),
                              1.0f / maxWetMakeupGain, maxWetMakeupGain)
                : 1.0f;
            const float wetMakeup = 1.0f + (makeupTarget - 1.0f) * autoGainAmount;
            wetL *= wetMakeup;
            wetR *= wetMakeup;

            wetL += lowMid;
            wetR += lowMid;
            const float sideBed = side * stereoPreserve;
            wetL += sideBed;
            wetR -= sideBed;

            const auto nulled = nullCoreProcess (wetL, wetR);
            wetL += (nulled.left - wetL) * nullCoreAmount;
            wetR += (nulled.right - wetR) * nullCoreAmount;

            sumDryPower += (double) dryL * dryL + (double) dryR * dryR;
            sumWetPower += (double) wetL * wetL + (double) wetR * wetR;
            sumDryWet += (double) dryL * wetL + (double) dryR * wetR;

            const auto dryWet = equalPowerMix (mix);
            const float predictedPower = std::clamp (
                dryWet.dry * dryWet.dry + dryWet.wet * dryWet.wet
                    + 2.0f * dryWetCorrelationEstimate * dryWet.dry * dryWet.wet,
                minMixPredictedPower, maxMixPredictedPower);
            const float mixNormTarget = 1.0f / std::sqrt (predictedPower);
            const float mixNorm = 1.0f + (mixNormTarget - 1.0f) * autoGainAmount;

            const float processedL = (dryWet.dry * dryL + dryWet.wet * wetL) * mixNorm * outGain;
            const float processedR = (dryWet.dry * dryR + dryWet.wet * wetR) * mixNorm * outGain;
            float finalL = processedL + (dryL - processedL) * softBypass;
            float finalR = processedR + (dryR - processedR) * softBypass;

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
                uiPhi.store ((float) orbitmath::wrapTwoPi (thetaB - thetaA),
                             std::memory_order_relaxed);
                uiBassAnchorHz.store (bassAnchorHz, std::memory_order_relaxed);
            }
        }

        if (numSamples > 0)
        {
            const double blockSeconds = (double) numSamples / sampleRate;
            const float correlationAlpha = (float) (1.0 - std::exp (
                -blockSeconds / mixCorrelationTimeSeconds));
            float measuredDryWet = 0.0f;
            const double dryWetDenom = std::sqrt (sumDryPower * sumWetPower);
            if (dryWetDenom > 1.0e-12)
                measuredDryWet = (float) std::clamp (sumDryWet / dryWetDenom, -1.0, 1.0);

            dryWetCorrelationEstimate += correlationAlpha
                                       * (measuredDryWet - dryWetCorrelationEstimate);
            if (! std::isfinite (dryWetCorrelationEstimate))
                dryWetCorrelationEstimate = 0.0f;

            const double invN = 1.0 / (double) numSamples;
            const double meanLL = sumLL * invN;
            const double meanRR = sumRR * invN;
            const float rms = (float) std::sqrt (0.5 * (meanLL + meanRR));
            uiOutputRms.store (std::isfinite (rms) ? rms : 0.0f,
                               std::memory_order_relaxed);

            const double denom = std::sqrt (meanLL * meanRR);
            const float correlation = denom > 1.0e-12
                ? (float) std::clamp ((sumLR * invN) / denom, -1.0, 1.0)
                : 1.0f;
            uiCorrelation.store (std::isfinite (correlation) ? correlation : 1.0f,
                                 std::memory_order_relaxed);
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
        state.hostPhaseLocked = uiHostPhaseLocked.load (std::memory_order_relaxed);
        state.bassAnchorHz = uiBassAnchorHz.load (std::memory_order_relaxed);
        state.symmetry01 = uiSymmetry.load (std::memory_order_relaxed);
        state.outputRms = uiOutputRms.load (std::memory_order_relaxed);
        state.correlation = uiCorrelation.load (std::memory_order_relaxed);
        state.phaseA = uiPhaseA.load (std::memory_order_relaxed);
        state.phi = uiPhi.load (std::memory_order_relaxed);
        return state;
    }
}
