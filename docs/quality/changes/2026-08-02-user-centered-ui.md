# Change record: user-centred editor architecture

- Change ID: CHG-20260802-user-centered-ui
- Date: 2026-08-02
- Owner: repository product owner
- Branch / PR: agent/world-class-dsp-phase3 / PR #2
- Risk tier: R3
- Covered paths: Source/PluginEditor.*; Source/ui/*; Tools/RenderShots.cpp; docs/claude-code/UI_UX_STANDARD.md; .claude/skills/ui-ux-audit/SKILL.md; .claude/agents/ui-ux-auditor.md; docs/commercial-upgrade/decisions/ADR-009-user-centered-editor-architecture.md
- Decision: PROPOSED

## User and musical outcome

A first-time musician should understand DNA Orbit, reach a useful sound from Basic, see why a control is disabled, compare Dry/processed immediately, and reach advanced Motion/Space/Output controls without visual or conceptual overload.

## Constitutional fit

- Affected pillars: P1, P2, P7, P8, P9.
- Why this belongs in DNA Orbit: the UI exposes the orbit/centre model and makes the existing musical system easier to use.
- Why it does not turn the product into a generic effect: groups follow DNA Orbit’s Motion, Space, and Output concepts rather than generic module accumulation.
- Whole-product benefit: faster first result, clearer dependencies, better warning visibility, stronger visual truth.
- Constitutional conflict: none intended; real-user observation remains required.

## Risk classification

- Highest credible failure consequence: inaccessible or misleading controls, hidden bypass/dependencies, unreadable minimum layout, false visual state, or DAW keyboard conflict.
- Protected paths affected: editor architecture, visualiser presentation, LookAndFeel, screenshot tooling.
- Why this tier is sufficient: editor/visual telemetry behavior is product-facing and can affect automation understanding, though no parameter IDs or audio state are intentionally changed.
- Escalation triggers: persistent UI state/schema changes, visual claims stronger than DSP telemetry, or control removal affecting sessions.

## Truth class and assumptions

- Truth class: EXACT UI state wiring and DSP telemetry; ARTISTIC visual styling; user-flow hypotheses remain UNKNOWN until observed.
- Equations/state machine: Basic/Detail visibility, responsive breakpoint, parameter dependency state.
- Units/coordinates/timing: pixels, DPI/host scaling, timer rates, editor dimensions.
- Assumptions: target users understand common audio controls but should not need implementation knowledge.
- Invalid regimes: screenshots alone cannot prove usability/accessibility.
- Primary references: current editor source, UI/UX standard, ADR-009, user request.

## Protected invariants

Visualiser truth, parameter/state attachment behavior, automation-driven warnings, accessible names, Japanese glyph support, hidden-editor efficiency, bypass reachability, no hidden cause for disabled Basic controls.

## Compatibility and migration

- Parameter IDs/version hints: unchanged.
- State schema: editor page/size state preserved.
- Old-session sound: unchanged.
- Presets/defaults: unchanged by layout.
- Host automation: UI reflects APVTS and engine telemetry.
- Migration fixtures: not applicable to audio state.

## Evidence plan and rejection conditions

- Automated tests: screenshot renderer dimensions and existing state tests.
- Numerical/audio measurements: not applicable except visualiser performance/CPU.
- Level-matched listening: not applicable to layout; Bypass workflow supports later listening.
- UI/interaction evidence: eight-state screenshots, keyboard, resizing, DPI, tooltips, warnings, modified preset/revert, observed first-use tasks.
- Host/validator evidence: LUNA/Cubase plus another host, DAW shortcut coexistence.
- Performance evidence: editor open/closed and adaptive rendering.
- Reject or roll back if Basic becomes slower, Detail remains cramped, warnings are hidden, visuals misstate DSP, or DAW interaction regresses.

## Independent review

- Required reviewers: quality governor, UI/UX auditor, validation architect.
- Review records: `docs/quality/reviews/2026-08-02-user-centered-ui-review.md`.
- Blocker findings: screenshots and real interaction have not yet been run.
- High findings: compile/API correctness, keyboard order, DPI, and observed-user workflow remain open.
- Disagreements and resolution: visual taste decisions must follow actual renders and user outcome, not code intent.

## Rollback boundary

Revert editor/LookAndFeel/renderer files to the previous layout while preserving DSP and APVTS parameters. No audio-state migration is required.

## Completion state and decision

- Designed: yes.
- Implemented: yes, draft branch.
- Compiled: not in this environment.
- Tested: screenshot matrix coded, not run here.
- Measured: not yet.
- Governance-audited: not yet locally.
- Screenshot-verified: no.
- Keyboard-tested: no.
- Listened: not applicable to layout itself.
- Host-validated: no.
- Observed-user validated: no.
- Owner-approved where required: pending visual/product review after renders.
- Release-ready: no.
- Remaining debt/exceptions: no exception granted; validation gaps are gates.
- Final decision and rationale: PROPOSED / IMPLEMENTED FOR DRAFT.
