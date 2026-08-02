# DNA Orbit Development Operating System

## Purpose

This document defines how an agent turns an idea into trustworthy product work. It exists to prevent three recurring failures:

1. editing before understanding the current system;
2. increasing algorithmic complexity without proving physical or audible value;
3. claiming completion before compiling, measuring, listening, and host validation.

## 1. Convert the request into a product outcome

A request such as “make the orbit more physical” is not yet an engineering specification. Translate it into:

- intended listener perception;
- intended musical role;
- scope of physical claim;
- sources and hosts affected;
- compatibility constraints;
- objective and subjective evidence needed.

Example:

> Improve the perception that the strands pass behind the listener while preserving low-frequency centre stability, without adding latency, changing old projects, or causing audible pitch modulation.

That statement exposes conflicts that “improve depth” hides.

## 2. Reconstruct the current system

Trace actual data flow:

1. parameter declaration and default;
2. state restoration and migration;
3. processor snapshot and host data;
4. engine smoothing and state;
5. per-sample signal flow;
6. telemetry to UI;
7. presets and tests;
8. documentation and manual gates.

Do not rely on file names, comments, README, or previous conversation alone. The implementation is the current truth; tests reveal some contracts; ADRs reveal intended rationale.

Produce a short baseline map before editing.

## 3. Define truth class and reference behavior

For each affected behavior choose:

- EXACT;
- NUMERICAL APPROXIMATION;
- PSYCHOACOUSTIC MODEL;
- ARTISTIC EXTENSION;
- UNKNOWN / UNVALIDATED.

Then define the reference equation or state machine. Include:

- symbols and units;
- coordinate frame and signs;
- initial and reset state;
- parameter limits;
- discrete-time update;
- expected limiting cases;
- error budget.

No production implementation should be accepted solely because it “sounds plausible.”

## 4. Freeze invariants

Typical invariants:

- parameter IDs and old-session behavior;
- exact Dry and bypass endpoints;
- no audio-thread allocation or lock;
- finite bounded output;
- defined centre/antipodal geometry;
- deterministic host-locked timeline behavior;
- NULL CORE meaning;
- no unannounced latency;
- visualiser reflects DSP.

Write tests or fixtures for invariants before large changes.

## 5. Research before inventing

Research is mandatory when:

- host/JUCE behavior is uncertain;
- a physical law or psychoacoustic claim is material;
- a numerical method is unfamiliar;
- a format/validator/signing rule may have changed;
- a competitor feature is used as a product reference.

Use primary sources first. Record:

- source and version/date;
- exact claim supported;
- assumptions;
- conflicts with other sources;
- how the source maps to DNA Orbit.

Do not copy an algorithm because a blog calls it “industry standard.”

## 6. Choose the fidelity level deliberately

For a new model, compare at least:

1. current model;
2. simplest correct reference;
3. higher-fidelity physical/numerical model;
4. musical extension;
5. no-change baseline.

Evaluate:

- audible benefit;
- truthfulness;
- CPU and memory;
- latency;
- automation safety;
- determinism;
- state migration;
- implementation and test cost;
- future extensibility.

Prefer the simplest model that meets the product outcome and evidence target.

## 7. Implement in evidence-producing slices

Recommended sequence:

1. isolated math/helper with analytical tests;
2. reference DSP path;
3. state and migration;
4. automation smoothing;
5. stereo/mono and endpoint tests;
6. transport/bypass/reset tests;
7. objective measurement;
8. listening experiment;
9. CPU profiling and justified optimization;
10. UI and documentation.

Each slice should compile and provide new evidence.

## 8. Review in parallel, edit serially

Parallel work is useful for:

- standards research;
- physics audit;
- audio-quality audit;
- test design;
- call-graph/realtime review;
- host matrix research.

Avoid multiple agents editing the same DSP architecture concurrently. Let specialist agents return findings; one implementation thread integrates decisions and runs the final test sequence.

## 9. Failure handling

When a build or test fails:

1. reproduce with the smallest command;
2. capture exact error and environment;
3. identify whether implementation, test, assumption, or environment is wrong;
4. fix root cause;
5. add regression coverage if product behavior was wrong;
6. rerun narrow then full checks.

Forbidden responses to failure:

- widening tolerance without an error derivation;
- skipping the failing platform/sample rate;
- disabling a warning or sanitizer without root cause;
- adding output gain to hide a transfer-function change;
- deleting migration behavior to simplify new code.

## 10. Optimization protocol

Before optimization:

- use Release build;
- identify a representative production path;
- warm caches/state;
- measure same machine and power mode;
- capture ns/sample/instance and real-time percentage;
- identify hotspot with profiler where possible.

After optimization:

- run null/error comparison;
- run relevant frequency/phase/sideband tests;
- run level-matched listening if error is not provably inaudible;
- measure again with identical conditions.

No CPU claim without before/after numbers.

## 11. Completion protocol

Every final report must separate:

- designed;
- implemented;
- compiled;
- tested;
- measured;
- listened;
- host-validated;
- release-ready.

Include commit/branch, commands, tests, measurement files, listening record, open manual gates, and known risks.

## 12. Continuous improvement

After each significant task, ask:

- What did the agent misunderstand?
- What information was expensive to rediscover?
- What failure should become a deterministic test or script?
- What belongs in a scoped rule, skill, agent, ADR, or assumption entry?
- Is any existing instruction stale or contradictory?

Update the lowest-cost durable layer and remove duplication.
