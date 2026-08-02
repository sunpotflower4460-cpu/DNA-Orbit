# Claude Code Foundation for DNA Orbit

This directory is the durable technical foundation for future Claude Code work. It is designed to maximize results without loading every long procedure into every session.

> **Validation state:** the files and scripts in this foundation are implemented in the current draft branch. They have not yet been exercised by an installed Claude Code session or by the repository’s full local build. The governance auditor and expanded UI screenshot matrix are specified but have not yet been run and inspected locally. Run the first-session check and `bash scripts/validate-local.sh` before treating the foundation as validated.

## Governing intent comes first

The stable product source of truth is:

- `docs/governance/PRODUCT_CONSTITUTION.md`;
- `docs/governance/QUALITY_GOVERNANCE_SYSTEM.md`;
- `docs/governance/CHANGE_RISK_MODEL.md`;
- `docs/governance/DECISION_AND_EXCEPTION_POLICY.md`.

The Product Constitution protects musical purpose, the two-strand/centre relationship, physical honesty, audio quality, safety, compatibility, simplicity, visual truth, evidence, and reversibility. It is SHA-256 locked and cannot be edited as an incidental feature change.

## How the layers work

| Layer | Location | Loaded when | Purpose |
|---|---|---|---|
| Product Constitution | `docs/governance/PRODUCT_CONSTITUTION.md` | Before substantial work | Stable identity, pillars, invariants, decision order |
| Machine governance | `quality/governance.json` + governance scripts | Local gate | Risk, required records, integrity, exception expiry |
| Core contract | `AGENTS.md` + `CLAUDE.md` | Every session | Non-negotiable priorities, honesty, workflow, safety |
| Change records | `docs/quality/` | Every substantial change | Scope, risk, evidence, reviews, exceptions, debt, release decisions |
| Scoped rules | `.claude/rules/*.md` | Matching files are read | DSP, audio quality, UI/UX, state, tests, documentation rules |
| Skills | `.claude/skills/*/SKILL.md` | Invoked or recognized as relevant | Repeatable multi-step procedures |
| Subagents | `.claude/agents/*.md` | Delegated by Claude or user | Independent read-only specialist review |
| Deep standards | `docs/claude-code/*.md` | Read on demand | Detailed engineering and product-quality references |
| Deterministic scripts | `scripts/*` | Explicitly run | Governance, repository state, static audit, build/test/render/benchmark |
| ADRs | `docs/commercial-upgrade/decisions/` | Relevant decision work | Historical rationale and compatibility decisions |
| Experiment records | `docs/experiments/` | One record per decision | Reproducible measurements/listening and adopted/rejected results |
| Manual gates | `MANUAL_REQUIRED.md` | Before review/release | Listening, UI inspection, DAW, signing, installer and legal work |

## First session check

From the repository root:

```sh
bash scripts/agent-preflight.sh
bash scripts/quality-gate.sh
claude
```

Inside Claude Code, verify:

```text
/context
/skills
/agents
```

`CLAUDE.md` should appear under memory/context. Project skills and agents should be listed.

For any substantial change:

```text
/quality-governance <intended user or musical outcome>
```

For substantial DSP work:

```text
/start-dsp-task <intended audio result>
```

For substantial editor or visual work:

```text
/ui-ux-audit <intended user result>
```

## Recommended governance workflow

1. `/quality-governance` — constitutional fit, R0–R4 risk, rejection condition, rollback, required records/review.
2. Create/update the change record under `docs/quality/changes/`.
3. Use the domain workflow below.
4. When scope expands, stop and update governance before adding more code.
5. Delegate `quality-governor` plus relevant specialists.
6. Run `bash scripts/quality-gate.sh` during development.
7. Use only ACCEPT / ACCEPT WITH DEBT / EXPERIMENT ONLY / REVISE / REJECT / ROLL BACK.
8. Before release authorisation run `bash scripts/quality-gate.sh --release` and record the owner decision.

## Recommended DSP workflow

1. `/start-dsp-task` — establish baseline, truth class, equations, invariants, compatibility, and evidence plan.
2. `/research-primary-sources` — use when APIs, physical claims, standards, validators, or numerical methods are uncertain/version-sensitive.
3. Implement a small coherent change with tests.
4. `/physics-audit` — verify physical and numerical validity.
5. `/audio-quality-gate` — verify exact contracts, measurements, listening, and CPU tradeoffs.
6. `/adversarial-review` — attack the implementation from independent lenses.
7. `/quality-governance` — verify scope, ratchet, debt, rollback, and whole-product direction.
8. `/release-gate` — determine the actual completion state.

For competing algorithms or subjective tuning:

```text
/experiment-design <hypothesis or alternatives>
```

For measured performance work:

```text
/optimize-dsp-safely <hotspot or target>
```

## Recommended UI / UX workflow

1. `/quality-governance` — confirm the user outcome and product fit.
2. `/ui-ux-audit` — define target musician, first useful result, information architecture, dependencies, responsive sizes, and evidence plan.
3. Trace parameters, UI state, automation, presets, warnings, and visualiser before changing layout.
4. Implement the smallest coherent user-flow improvement.
5. Generate the required screenshot matrix at 820x650, 960x700, and 1440x900.
6. Delegate read-only reviews to `quality-governor` and `ui-ux-auditor`.
7. Test keyboard traversal, resizing, host scaling, tooltips, warnings, preset revert, and DAW shortcut coexistence.
8. Distinguish implemented, governance-audited, screenshot-verified, keyboard-tested, host-validated, and observed-user validation.

## Project skills

- `quality-governance` — constitution, risk, change contract, evidence gates, exceptions/debt, and release decision.
- `start-dsp-task` — rigorous DSP task kickoff and baseline.
- `research-primary-sources` — current primary-source research and assumption capture.
- `physics-audit` — model, units, assumptions, numerical and physical validity.
- `audio-quality-gate` — exact contracts, measurement, level-matched listening and CPU tradeoffs.
- `experiment-design` — preregister and record reproducible algorithm/listening experiments.
- `optimize-dsp-safely` — profiler-led optimization with sound-preservation evidence.
- `ui-ux-audit` — musician workflow, hierarchy, responsive layout, accessibility, visual truth, and screenshot evidence.
- `adversarial-review` — severity-ranked hostile review.
- `release-gate` — honest completion and commercial release status.

## Specialist subagents

- `quality-governor` — constitution, whole-product direction, risk, ratchet, exceptions/debt, and release integrity.
- `physics-auditor` — equations, units, acoustics, numerical model and claims.
- `audio-quality-auditor` — audible defects, measurement, listening and CPU-quality tradeoffs.
- `realtime-safety-reviewer` — callback safety, bounded execution and host-state risks.
- `validation-architect` — falsifiable automated/manual evidence design.
- `ui-ux-auditor` — first-use workflow, hierarchy, responsive layout, accessibility, visual truth, and missing user evidence.

The implementation agent should not treat these reviewers as approval machines. Compare findings, resolve disagreements from source code and evidence, and document the decision. No agent may mark product-owner approval.

## Deep standards

- `CURRENT_ARCHITECTURE_MAP.md` — current code responsibilities, signal flow and high-risk zones.
- `DEVELOPMENT_OPERATING_SYSTEM.md` — complete lifecycle and decision process.
- `PHYSICS_FIDELITY_STANDARD.md` — model hierarchy, equations, units and physical honesty.
- `PHYSICAL_AUDIO_ROADMAP.md` — staged route from current psychoacoustic product to defensible physical/hybrid modes.
- `AUDIO_QUALITY_STANDARD.md` — signal quality, stereo, automation, measurement and listening.
- `UI_UX_STANDARD.md` — first-use workflow, persistent/global actions, control language, responsive layout, accessibility and visual evidence.
- `EXPERIMENT_AND_VALIDATION_PROTOCOL.md` — reproducible experiments and error budgets.
- `QUALITY_SCORECARD.md` — go/no-go scoring without averaging away blockers.
- `ASSUMPTIONS_REGISTER.md` — explicit assumptions, evidence and invalid regimes.
- `PROMPT_LIBRARY.md` — high-value Japanese prompts for future sessions.
- `REFERENCE_SOURCES.md` — primary documentation and standards entry points.
- `OPTIONAL_AUTOMATION.md` — optional hooks and stronger local enforcement.

## Maintenance rule

When Claude makes the same class of mistake twice, add the correction at the lowest-cost correct layer:

- product identity or decision hierarchy → propose constitutional/governance change, never edit casually;
- universal engineering invariant → `CLAUDE.md` / `AGENTS.md`;
- applies only to files or directory → scoped rule;
- repeatable procedure → skill;
- repeatable specialist review → subagent;
- deep explanation → standards/governance docs;
- deterministic enforcement → script or hook;
- product decision → ADR;
- measurement/listening/usability result → experiment record;
- uncertain premise → assumptions register;
- temporary bypass → expiring exception;
- accepted non-blocking deficiency → quality debt ledger.

Keep `CLAUDE.md` short. Long always-loaded instructions reduce adherence and consume the context needed for actual engineering.
