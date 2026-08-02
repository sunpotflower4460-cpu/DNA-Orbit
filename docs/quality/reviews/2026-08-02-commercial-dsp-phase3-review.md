# Independent review: commercial DSP phase 3

- Review ID: REV-20260802-commercial-dsp-phase3
- Date: 2026-08-02
- Reviewer role: quality, physics/audio, realtime, and validation synthesis
- Reviewer identity / agent: ChatGPT source review; not external listening or owner approval
- Change / PR: CHG-20260802-commercial-dsp-phase3 / PR #2
- Commit reviewed: evolving draft branch
- Risk tier: R4
- Owner approval: PENDING

## Review scope

Reviewed the intended centered stereo architecture, Soft Bypass, correlation-aware Mix, Bass Anchor, Character, Host Lock, state/preset/test design, realtime constraints, and validation plan. Compilation, measurements, listening, validators, and real DAWs are outside verified scope.

## Constitutional alignment

- Pillars strengthened: P1–P6, P9, P10.
- Pillars at risk: P4 if complexity is called higher quality without listening; P6 if migration differs in compiled behavior; P7 if advanced controls overwhelm the default experience.
- Product-identity drift: low by intent because changes remain tied to orbit, centre, stereo stability, and transport repeatability.
- Whole-product versus local optimisation: generally whole-product, but each feature still needs evidence of audible/user value.

## Findings

### Blocker

- No release decision until the complete branch compiles and the mandatory local/host/listening gates pass.

### High

- Full JUCE compile and all new tests remain unrun in this environment.
- New defaults and Character values are engineering proposals until level-matched listening confirms them.
- Host Lock behavior needs real-host loop/seek/offline verification.
- Correlation Mix and crossover behavior need actual measurement results, not only test intent.

### Medium

- The branch combines several major features, increasing diagnosis cost if first local build has multiple failures.
- Current PR is behind/conflicted with main according to GitHub metadata and must be reconciled safely before merge.

### Low

- Benchmark baselines should be retained across future optimization work.

### Questions

- Whether 120 Hz and current Character curves are the final owner-preferred defaults.
- Whether visual history needs a harder reset policy on host discontinuities.

## Evidence examined

Source code, parameter/state/preset definitions, tests, ADR-004 through ADR-008, README/manual gates, benchmark and validation scripts, and PR description.

## Adversarial cases

- legacy state opens with a subtly different sound;
- full bypass differs at one endpoint under output trim;
- anti-phase or channel-imbalanced material defeats centred motion;
- block-size or stopped-callback behavior changes Host Lock;
- automation creates a click despite static tests;
- CPU optimization changes modulation character;
- physical language exceeds psychoacoustic implementation.

## Decision recommendation

REVISE / HOLD IN DRAFT until compile, automated evidence, measurements, level-matched listening, host validation, and safe main integration are complete.

## Resolution verification

Pending local commands, exact commit, test logs, measurements, listening record, DAW matrix, validator results, and owner decision on defaults.
