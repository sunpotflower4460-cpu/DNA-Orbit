---
name: start-dsp-task
description: Start any non-trivial DNA Orbit DSP, transport, stereo, filter, delay, phase, bypass, or audio-quality task with a rigorous baseline, invariants, research plan, and validation plan. Use before editing production DSP code.
argument-hint: "<desired musical or technical outcome>"
---

# Start a DNA Orbit DSP task

Goal from the user:

`$ARGUMENTS`

Do not edit production code until the following kickoff is complete.

## 1. Establish repository state

Run and summarize:

```sh
bash scripts/agent-preflight.sh
git diff -- Source Tests CMakeLists.txt
git log -8 --oneline
```

Read:

- `AGENTS.md`
- `CLAUDE.md`
- relevant source and tests end-to-end
- relevant ADRs under `docs/commercial-upgrade/decisions/`
- `docs/claude-code/PHYSICS_FIDELITY_STANDARD.md`
- `docs/claude-code/AUDIO_QUALITY_STANDARD.md`
- `MANUAL_REQUIRED.md`

## 2. Produce a task brief

Before implementation, write a compact brief containing:

- **User outcome:** the musical/product result, not merely the requested code change.
- **Current behavior:** traced from actual code and tests.
- **Problem evidence:** bug, limitation, measurement, or clearly stated hypothesis.
- **Truth class:** EXACT / NUMERICAL APPROXIMATION / PSYCHOACOUSTIC MODEL / ARTISTIC EXTENSION / UNKNOWN.
- **Reference model:** equations or state machine with symbol definitions and units.
- **Invariants:** behavior that must not change.
- **Compatibility:** parameter IDs, schema, presets, latency, host behavior, and old sessions.
- **Risks:** sound, stability, CPU, UI truth, and release risks.
- **Validation:** automated tests, measurements, listening, and host checks.
- **Rollback:** smallest reversible boundary if the approach fails.

## 3. Research uncertain claims

Use primary sources for:

- JUCE and host API behavior;
- VST3/AU/CLAP format contracts;
- physical acoustics and psychoacoustics;
- loudness/measurement standards;
- numerical methods and interpolation behavior.

Record source, date/version, claim supported, and remaining uncertainty in the relevant ADR or assumptions register.

## 4. Plan the implementation

Prefer this order:

1. encode the invariant in a failing or missing test;
2. implement the smallest correct reference path;
3. compare against analytical/high-precision behavior;
4. add smoothing and automation safety;
5. measure audio quality;
6. measure CPU;
7. optimize only if the profile justifies it;
8. update UI/documentation/ADR;
9. run hostile independent reviews.

Do not mix speculative redesign, formatting, and unrelated refactors into the same change.

## 5. Completion

Use `physics-audit`, `audio-quality-gate`, `adversarial-review`, and `release-gate` before reporting the task as complete.
