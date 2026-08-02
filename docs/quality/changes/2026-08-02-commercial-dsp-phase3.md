# Change record: commercial DSP and deterministic transport phase 3

- Change ID: CHG-20260802-commercial-dsp-phase3
- Date: 2026-08-02
- Owner: repository product owner
- Branch / PR: agent/world-class-dsp-phase3 / PR #2
- Risk tier: R4
- Covered paths: CMakeLists.txt; Source/Parameters.h; Source/PluginProcessor.*; Source/Presets.h; Source/dsp/*; Tests/*; Tools/BenchmarkDSP.cpp
- Decision: PROPOSED

## User and musical outcome

Provide a stable, centred, repeatable, musically useful spatial-motion processor with protected low end, intentional depth characters, reliable bypass/mix behavior, old-session compatibility, and measurable performance.

## Constitutional fit

- Affected pillars: P1–P6, P9, P10.
- Why this belongs in DNA Orbit: every change supports the two-strand/centre spatial-motion concept rather than adding unrelated processing.
- Why it does not turn the product into a generic effect: Bass Anchor, Character, Stereo Preserve, Mix, and Host Lock remain subordinate to the orbit/centre model.
- Whole-product benefit: sound, transport repeatability, session safety, and release evidence improve together.
- Constitutional conflict: none intended; new defaults and model labels require listening/owner confirmation.

## Risk classification

- Highest credible failure consequence: loud artifact, callback instability, changed old-session sound, nondeterministic render, broken parameter/state contract, or misleading physical claim.
- Protected paths affected: identifiers/schema, processor host integration, DSP engine, presets, tests, build configuration.
- Why this tier is sufficient: Parameter and processor contracts are R4 even though most signal algorithms are R3.
- Escalation triggers: identifier change, destructive migration, release claim, or irreversible default adoption.

## Truth class and assumptions

- Truth class: EXACT digital contracts, NUMERICAL APPROXIMATION for filters/interpolation, PSYCHOACOUSTIC MODEL for depth cues, ARTISTIC EXTENSION for Character.
- Equations/state machine: recorded in ADR-004 through ADR-008 and source/tests.
- Units/coordinates/timing: samples, seconds, Hz, dB, PPQ, radians, normalized percentages.
- Assumptions: stereo output, valid JUCE host lifecycle, available PPQ/BPM for Host Lock, programme-dependent listening still required.
- Invalid regimes: current depth model is not a full physical propagation/HRTF/room solver.
- Primary references: JUCE contracts, DSP equations, repository ADRs and standards.

## Protected invariants

Exact Dry and Soft Bypass, transparent Bass Anchor Off, finite bounded output, callback safety, parameter IDs, legacy schema sound, antipodal full Symmetry, deterministic Host Lock, mono/Side behavior, and truthful model labels.

## Compatibility and migration

- Parameter IDs/version hints: existing IDs preserved; schema-3 parameters added intentionally.
- State schema: schema 3 with schema-1/2 compatibility defaults.
- Old-session sound: Bass Anchor Off, Natural, Free phase and legacy Stereo Preserve migration paths protect prior behavior.
- Presets/defaults: complete-state presets; new defaults require listening confirmation.
- Host automation: smoothed or bounded transitions; host playhead queried in process callbacks.
- Migration fixtures: state and preset tests included.

## Evidence plan and rejection conditions

- Automated tests: baseline, state, presets, stereo, centred Side bed, Mix, bypass, crossover/Character, transport phase.
- Numerical/audio measurements: crossover sum, exact endpoints, correlation behavior, determinism, benchmark.
- Level-matched listening: required on named vocal/instrument/full-mix corpus.
- UI/interaction evidence: parameter dependencies handled by separate UI change record.
- Host/validator evidence: LUNA/Cubase first, broader DAW matrix, pluginval/VST3 Validator/auval.
- Performance evidence: benchmark at 44.1–192 kHz, block sizes, multiple instances.
- Reject or roll back if old sessions change without policy, Host Lock is not reproducible, low end hollows, artifacts appear, or complexity lacks audible benefit.

## Independent review

- Required reviewers: quality governor, physics/audio, realtime safety, validation architecture.
- Review records: `docs/quality/reviews/2026-08-02-commercial-dsp-phase3-review.md`.
- Blocker findings: local compile/test/validator evidence not yet available.
- High findings: complete real-host and listening validation remains open.
- Disagreements and resolution: final Character/default tuning awaits evidence.

## Rollback boundary

Revert individual coherent features and migrations by ADR boundary while preserving parameter IDs and safe legacy defaults. Do not remove adopted IDs after release.

## Completion state and decision

- Designed: yes.
- Implemented: yes, draft branch.
- Compiled: not in this environment.
- Tested: tests written, not run here.
- Measured: benchmark written, not run here.
- Governance-audited: not yet locally.
- Screenshot-verified: covered by UI record.
- Keyboard-tested: covered by UI record.
- Listened: not yet.
- Host-validated: not yet.
- Observed-user validated: not applicable to DSP alone.
- Owner-approved where required: pending final defaults/product release.
- Release-ready: no.
- Remaining debt/exceptions: no exception granted; validation gaps are explicit gates, not accepted debt.
- Final decision and rationale: PROPOSED / IMPLEMENTED FOR DRAFT.
