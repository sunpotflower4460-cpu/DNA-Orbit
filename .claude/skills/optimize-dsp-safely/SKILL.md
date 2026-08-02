---
name: optimize-dsp-safely
description: Optimize DNA Orbit CPU or memory without silently changing sound, timing, state, or host behavior. Use for profiling, control-rate changes, trigonometric/filter/delay optimization, SIMD, buffer changes, and multi-instance performance work.
argument-hint: "<suspected hotspot or performance goal>"
---

# Evidence-based DSP optimization

Performance goal or suspected hotspot:

`$ARGUMENTS`

Read `AGENTS.md`, the DSP/audio-quality rules, `docs/claude-code/AUDIO_QUALITY_STANDARD.md`, and current benchmark code.

## 1. Freeze behavior

Before optimizing, identify and test:

- exact endpoints and compatibility paths;
- signal transfer/invariants affected by the hotspot;
- transport, reset, automation, and state behavior;
- representative sample rates, block sizes, channel relationships, and parameters;
- a reference output or high-precision implementation.

## 2. Establish baseline

Use a Release build on the same machine and power mode. Record:

- commit and compiler flags;
- CPU and OS;
- sample rate/block size;
- 1 and 10 instances;
- relevant modes/parameters;
- ns/sample/instance and real-time percentage;
- profiler evidence when available;
- audio-only and editor-open results separately if UI is relevant.

Do not optimize solely from source inspection when profiling can identify the real cost.

## 3. Choose the least risky optimization

Prefer, in order:

1. remove redundant work without changing arithmetic order materially;
2. cache values that are truly unchanged;
3. precompute outside the callback;
4. exploit exact mathematical symmetry;
5. improve data layout or avoid copies;
6. use bounded block processing/SIMD with reference comparison;
7. reduce update rate only with interpolation and quantified error;
8. change algorithm only when quality and compatibility evidence justify it.

Never introduce allocation, locks, unbounded setup, hidden latency, or state divergence.

## 4. Validate sound and behavior

After each optimization:

- compare exact paths with zero tolerance where applicable;
- report max/RMS residual against baseline/reference;
- measure magnitude, phase, group delay, modulation sidebands, or nonlinear artifacts as relevant;
- test automation and block segmentation;
- run duplicate renders;
- conduct level-matched listening if residual is not mathematically negligible;
- run real-time safety and hostile review.

## 5. Re-measure

Use identical baseline conditions and report:

- absolute and percentage CPU change;
- memory/latency change;
- scaling at 192 kHz and multiple instances;
- variance across repeated runs;
- any sound/numerical difference;
- whether the gain is large enough to justify complexity.

## Decision

Return adopt / reject / iterate / inconclusive. Revert an optimization that creates unexplained sound difference, fragile state, unreadable complexity without meaningful gain, or platform-specific risk outside product requirements.
