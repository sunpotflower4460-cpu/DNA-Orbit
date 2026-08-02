---
name: experiment-design
description: Turn a DNA Orbit DSP or sound-quality hypothesis into a reproducible experiment with controls, measurements, listening protocol, and acceptance criteria. Use when choosing between algorithms, tuning Character/presets, or evaluating a physical/psychoacoustic model.
argument-hint: "<hypothesis or alternatives>"
---

# Reproducible DSP experiment

Hypothesis or alternatives:

`$ARGUMENTS`

Read `docs/claude-code/EXPERIMENT_AND_VALIDATION_PROTOCOL.md`.

## Design requirements

1. State one primary hypothesis and any secondary hypotheses.
2. Define baseline/control and candidate implementations.
3. Freeze all variables not under test: input, level, sample rate, block size, start state, host position, seed, and monitoring gain.
4. Choose objective metrics that correspond to the hypothesis.
5. Define acceptance, rejection, and inconclusive criteria before running the test.
6. Include negative controls and a hidden reference where practical.
7. Separate exact/numerical validation from perceptual preference.
8. Randomize or counterbalance listening order and level-match within 0.1 dB.
9. Record environment, build commit, compiler, JUCE version, hardware, and tool versions.
10. Save raw results or machine-readable summaries where possible; never report only the preferred conclusion.

## Output artifact

Create an experiment record under `docs/experiments/YYYY-MM-DD-short-name.md` containing:

- question and product relevance;
- hypothesis;
- truth class;
- baseline and candidates;
- stimuli and test matrix;
- metrics and tools;
- listening protocol;
- predeclared thresholds;
- results with raw-data location;
- interpretation and confounds;
- decision: adopt / reject / iterate / inconclusive;
- follow-up and rollback.

If the experiment cannot be run, produce the complete protocol and mark results as pending. Do not invent measurements or listening results.
