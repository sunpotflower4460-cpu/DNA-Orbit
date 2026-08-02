# DNA Orbit Change Risk Model

Risk is assigned by the highest credible failure consequence, not by lines changed or implementation difficulty.

## R0 — Editorial

Examples:

- spelling or formatting;
- comments that do not alter claims;
- non-normative documentation cleanup.

Required:

- review the diff;
- ensure no product or physical claim changed accidentally.

## R1 — Internal / no product behavior

Examples:

- test infrastructure;
- development scripts;
- refactoring proven behaviorally identical;
- build diagnostics that do not change shipping artifacts.

Required:

- change record when substantial;
- relevant tests;
- rollback boundary;
- no unsupported “no behavior change” claim.

## R2 — User-facing bounded change

Examples:

- UI layout and wording;
- accessibility;
- visual polish;
- presets that do not change state contracts;
- tooltips and workflow changes;
- non-critical tooling shipped with the project.

Required:

- change record;
- constitutional fit;
- screenshot/interaction evidence as applicable;
- state and automation review;
- independent UI/validation review for substantial work.

## R3 — Core behavior

Examples:

- DSP algorithms;
- filter, delay, modulation, gain, stereo, phase, and spatial behavior;
- transport and host playhead behavior;
- bypass behavior;
- default sound;
- persistent state semantics;
- performance changes in the audio path;
- visual telemetry semantics.

Required:

- complete change record;
- truth class, equations/state machine, units, assumptions, and invalid regimes;
- relevant ADR;
- regression/property tests;
- numerical/audio evidence;
- compatibility and rollback plan;
- independent specialist review;
- level-matched listening and host validation when audible/host-facing;
- draft status until applicable gates pass.

## R4 — Constitutional / irreversible / release-critical

Examples:

- Product Constitution;
- product identity or non-negotiable invariants;
- parameter IDs, plugin identifiers, state versioning strategy;
- removal of compatibility guarantees;
- release/security/signing policy;
- licensing decisions;
- destructive migration;
- any decision that is difficult or impossible to reverse after users adopt it.

Required:

- dedicated `R4 / CONSTITUTIONAL` change record;
- dedicated ADR;
- explicit previous promise being changed;
- independent reviews across all materially affected domains;
- migration and rollback plan;
- product-owner approval recorded;
- constitution version/hash update when applicable;
- governance audit in release mode;
- no unresolved Blocker or High finding.

## Escalation rules

Escalate to the higher tier when any of these are true:

- the change affects a protected invariant;
- a user session can sound different after restore;
- a failure can create a loud transient, crash, hang, or corrupt state;
- a parameter meaning/default/automation range changes;
- a product or physical claim becomes stronger;
- the visualiser changes what it implies about DSP state;
- the change crosses audio/UI/host/state boundaries;
- rollback requires migration rather than a simple revert;
- evidence cannot cleanly isolate the change.

## De-escalation is evidence, not opinion

A change may be treated as lower risk only when the reason is documented and falsifiable. Small diffs can be R4. Large test-only additions can be R1.

## Minimum review matrix

| Domain affected | Minimum independent review |
|---|---|
| DSP/physics | physics or audio-quality auditor |
| audio callback/host | real-time safety reviewer |
| state/compatibility | validation architect plus migration evidence |
| UI/UX | UI/UX auditor |
| release/security/licensing | product owner plus relevant human specialist |
| constitution | quality governor plus all materially affected specialists and product owner |

## Decision severity

Findings use:

- **Blocker** — cannot merge/release for intended stage;
- **High** — likely serious regression or unsupported core claim; resolve before leaving draft unless owner explicitly narrows scope;
- **Medium** — meaningful weakness with an owner and plan;
- **Low** — local improvement or documentation gap;
- **Question** — unresolved premise; not counted as evidence.

A count or average never neutralises severity.
