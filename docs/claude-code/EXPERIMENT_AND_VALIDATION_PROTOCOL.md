# Experiment and Validation Protocol

## 1. Principle

Every important DSP or sound-quality decision should be framed so that the preferred conclusion could be disproved. Experiments are not demonstrations of what the author already believes.

## 2. Experiment record

Create one file per significant experiment under:

```text
docs/experiments/YYYY-MM-DD-short-name.md
```

Use the commit SHA of the tested code. Do not overwrite historical results when the implementation changes; create a new record or append a clearly dated rerun.

## 3. Pre-registration

Before running the experiment, write:

- product question;
- primary hypothesis;
- secondary hypotheses;
- baseline/control;
- candidates;
- truth class;
- independent variable;
- controlled variables;
- stimuli;
- metrics;
- listening attributes;
- acceptance/rejection/inconclusive criteria;
- known confounds;
- rollback plan.

Predeclaring thresholds prevents choosing a convenient success condition after seeing results.

## 4. Environment capture

Record:

- repository and commit;
- branch;
- dirty/clean state;
- compiler and version;
- build type and flags;
- JUCE version;
- OS and architecture;
- CPU and power mode;
- audio interface/driver where relevant;
- DAW and version;
- plug-in format;
- sample rate and block size;
- benchmark/measurement tool and version;
- listening hardware and gain-matching method.

## 5. Baseline integrity

A baseline must be:

- the current shipping/reference behavior, or clearly identified historical behavior;
- built with equivalent optimization and environment;
- level-matched where perception is compared;
- processed from the same input and state;
- free from accidental parameter/default differences.

For old-session compatibility, include an actual serialized old-state fixture rather than manually recreating values.

## 6. Objective validation classes

### Exact identity

Use for Dry, bypass, transparent compatibility paths, deterministic state transforms, and algebraic invariants.

Report maximum absolute sample difference, RMS residual, and first mismatch where useful. Use zero tolerance only when the operations permit exact floating-point identity.

### Analytical/numerical reference

Use for phase, panning, filters, crossover, interpolation, geometry, and transport mapping.

Compare to a closed-form, high-precision, oversampled, offline, or independently implemented reference. State why the reference is more trustworthy.

### Metamorphic/property testing

Examples:

- mirrored input and reversed direction produce mirrored output under defined conditions;
- identical PPQ and state produce identical Host Lock geometry;
- Symmetry 100% gives zero centroid;
- mono input with zero Side is invariant to Stereo Preserve;
- scaling input scales a linear path proportionally;
- changing block segmentation does not change deterministic output beyond error budget.

### Regression testing

Use a minimal input/state that reproduces an actual bug. Test root cause, not only a final checksum.

### Performance measurement

Performance results are comparative and environment-specific. Report median and dispersion over repeated runs where practical. Separate audio-only and editor-open measurements.

## 7. Stimulus design

A stimulus should expose the expected failure.

- impulse: latency, state, filter/delay transient;
- sine: gain/phase and modulation sidebands;
- sweep: transfer across frequency;
- multitone: intermodulation and sideband congestion;
- noise: statistical transfer and correlation;
- DC: high-pass/state behavior;
- silence: denormal/noise/tail;
- pure Side: stereo-preservation and mono risk;
- transient music: click, smearing and low-end focus;
- sustained bright music: interpolation/filter coloration and fatigue.

Use multiple phases for single tones when block/phase interaction is possible.

## 8. Tolerance design

Every tolerance needs a source:

- exact arithmetic identity;
- floating-point error propagation;
- analytical approximation bound;
- measured reference implementation difference;
- psychoacoustic threshold supported by evidence;
- product requirement;
- validator/format requirement.

Avoid tolerances derived only from “what currently passes.”

## 9. Listening experiment

### Preparation

- render or route baseline and candidates identically;
- match level within 0.1 dB where practical;
- remove identifying names;
- randomize order;
- include hidden reference/duplicate when the decision is important;
- choose short diagnostic loops and longer musical context.

### Questions

Separate:

- can a difference be detected?;
- which is preferred?;
- why?;
- on which source?;
- at what playback level?;
- what tradeoff appears elsewhere?;

### Rating dimensions

Use focused dimensions, not one overall “quality” score:

- motion continuity;
- depth impression;
- centre stability;
- low-end focus;
- width;
- tonal integrity;
- transient integrity;
- artifact audibility;
- mono compatibility;
- fatigue;
- musical usefulness.

### Interpretation

A preference result does not prove physical accuracy. A null result does not prove equality. Record uncertainty and listener count.

## 10. Host validation

For host-dependent behavior, test named hosts and versions. Include:

- scan and instantiate;
- editor open/close/reopen;
- audio start/stop;
- bypass/un-bypass;
- automation recording/playback;
- loop/seek/tempo/time-signature changes;
- save/close/reopen;
- duplicate and copy/paste instances;
- freeze/bounce/export realtime and offline;
- sample-rate/block-size change;
- removal/unload.

Do not generalize one DAW result to all hosts.

## 11. Data retention

Where practical save:

- command output;
- benchmark text/CSV/JSON;
- measurement plots/data;
- input fixture hashes;
- rendered output hashes;
- screenshot paths;
- validator logs;
- listening score sheet.

Do not commit large or copyrighted audio unless appropriate. Commit metadata, generation scripts, and hashes.

## 12. Result categories

- **Adopt** — evidence meets predeclared criteria and no higher-priority invariant regresses.
- **Reject** — candidate fails criteria or introduces unacceptable tradeoff.
- **Iterate** — direction is promising, but a defined defect remains.
- **Inconclusive** — experiment cannot distinguish candidates or has material confounds.
- **Experimental only** — musically interesting but evidence/compatibility is insufficient for default behavior.

## 13. Template

```markdown
# Experiment: <name>

- Date:
- Commit:
- Investigator:
- Product question:
- Truth class:

## Hypothesis

## Baseline and candidates

## Controlled variables

## Stimuli and matrix

## Metrics and tools

## Predeclared criteria

## Listening protocol

## Results

## Confounds and uncertainty

## Decision

## Follow-up / rollback
```

## 14. Anti-patterns

- tuning while seeing candidate names;
- comparing unmatched loudness;
- testing only the source that motivated the algorithm;
- changing implementation and thresholds simultaneously;
- reporting screenshots instead of raw measurements;
- selecting only favorable results;
- calling a failed or unavailable run “expected to pass”;
- replacing listening with metrics or metrics with listening when both are relevant.
