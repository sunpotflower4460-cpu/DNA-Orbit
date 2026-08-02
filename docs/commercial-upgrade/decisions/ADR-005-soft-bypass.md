# ADR-005: Internal-state-preserving Soft Bypass

- Status: Accepted
- Date: 2026-08-02

## Context

Host bypass is controlled by each DAW and may switch immediately. Phase 1 fixed the more serious state-freeze problem by continuing to process a scratch copy while the host bypass output remains exact Dry.

For musical A/B comparison and automation, DNA Orbit also needs a plug-in-owned bypass whose transition is guaranteed to be smooth and whose internal orbit never stops.

## Decision

Add an automatable `softBypass` boolean parameter.

- Off: normal processed output.
- On: linearly crossfade from processed output to exact input Dry over 60 ms.
- Internal orbit, delay lines, filters, parameter smoothers and visual state continue running at all times.
- At the fully bypassed endpoint, Output trim is bypassed too; the result is exact Dry.
- Factory presets explicitly set Soft Bypass off, so preset application stays deterministic.

The crossfade is linear rather than equal-power. Dry and processed signals are commonly correlated, so equal-power bypass could create a temporary gain bump. The primary requirement is a transparent endpoint and a click-free transition, not constant power between unrelated signals.

## Interaction with host bypass

Host bypass remains an exact immediate Dry passthrough while processing a scratch copy to advance state. Soft Bypass is the preferred control for audible A/B and automation because its transition is under the plug-in's control.

## Automated acceptance criteria

- Primed Soft Bypass outputs exact Dry sample-for-sample, even when Output is not 0 dB.
- A transition settles to Dry within 100 ms.
- Abrupt parameter automation does not create a large sample discontinuity.
- Orbit phase continues advancing while fully soft-bypassed.
- Factory presets reset Soft Bypass to off.

## Manual acceptance criteria

In Cubase, Logic, Live and REAPER:

- automate Soft Bypass across vocal, pad and drum material;
- confirm no click or level flare;
- compare against host bypass;
- confirm returning from bypass resumes the current orbit rather than an old position.
