# Claude Code Foundation for DNA Orbit

This directory is the durable technical foundation for future Claude Code work.
It is designed to maximize results without loading every long procedure into every session.

> **Validation state:** the files and scripts in this foundation are implemented in the current draft branch. They have not yet been exercised by an installed Claude Code session or by the repository’s full local build. Run the first-session check and `bash scripts/validate-local.sh` before treating the foundation as validated.

## How the layers work

| Layer | Location | Loaded when | Purpose |
|---|---|---|---|
| Core contract | `AGENTS.md` + `CLAUDE.md` | Every session | Non-negotiable priorities, honesty, workflow, safety |
| Scoped rules | `.claude/rules/*.md` | Matching files are read | DSP, audio quality, state, tests, UI, documentation rules |
| Skills | `.claude/skills/*/SKILL.md` | Invoked or recognized as relevant | Repeatable multi-step procedures |
| Subagents | `.claude/agents/*.md` | Delegated by Claude or user | Independent read-only specialist review |
| Deep standards | `docs/claude-code/*.md` | Read on demand | Detailed reasoning and reference material |
| Deterministic scripts | `scripts/*.sh` | Explicitly run | Repository state, static audit, build/test/benchmark |
| ADRs | `docs/commercial-upgrade/decisions/` | Relevant decision work | Historical rationale and compatibility decisions |
| Experiment records | `docs/experiments/` | One record per decision | Reproducible measurements/listening and adopted/rejected results |
| Manual gates | `MANUAL_REQUIRED.md` | Before review/release | Listening, DAW, signing, installer and legal work |

## First session check

From the repository root:

```sh
bash scripts/agent-preflight.sh
claude
```

Inside Claude Code, verify:

```text
/context
/skills
/agents
```

`CLAUDE.md` should appear under memory/context. Project skills and agents should be listed. Run:

```text
/start-dsp-task <your intended result>
```

for substantial DSP work.

## Recommended workflow

1. `/start-dsp-task` — establish baseline, truth class, equations, invariants, compatibility, and evidence plan.
2. `/research-primary-sources` — use when APIs, physical claims, standards, validators, or numerical methods are uncertain/version-sensitive.
3. Implement a small coherent change with tests.
4. `/physics-audit` — verify physical and numerical validity.
5. `/audio-quality-gate` — verify exact contracts, measurements, listening, and CPU tradeoffs.
6. `/adversarial-review` — attack the implementation from independent lenses.
7. `/release-gate` — determine the actual completion state.

For competing algorithms or subjective tuning:

```text
/experiment-design <hypothesis or alternatives>
```

For measured performance work:

```text
/optimize-dsp-safely <hotspot or target>
```

## Project skills

- `start-dsp-task` — rigorous task kickoff and baseline.
- `research-primary-sources` — current primary-source research and assumption capture.
- `physics-audit` — model, units, assumptions, numerical and physical validity.
- `audio-quality-gate` — exact contracts, measurement, level-matched listening and CPU tradeoffs.
- `experiment-design` — preregister and record reproducible algorithm/listening experiments.
- `optimize-dsp-safely` — profiler-led optimization with sound-preservation evidence.
- `adversarial-review` — severity-ranked hostile review.
- `release-gate` — honest completion and commercial release status.

## Specialist subagents

- `physics-auditor` — equations, units, acoustics, numerical model and claims.
- `audio-quality-auditor` — audible defects, measurement, listening and CPU-quality tradeoffs.
- `realtime-safety-reviewer` — callback safety, bounded execution and host-state risks.
- `validation-architect` — falsifiable automated/manual evidence design.

The implementation agent should not treat these reviewers as approval machines. Compare findings, resolve disagreements from source code and evidence, and document the decision.

## Deep standards

- `CURRENT_ARCHITECTURE_MAP.md` — current code responsibilities, signal flow and high-risk zones.
- `DEVELOPMENT_OPERATING_SYSTEM.md` — complete lifecycle and decision process.
- `PHYSICS_FIDELITY_STANDARD.md` — model hierarchy, equations, units and physical honesty.
- `PHYSICAL_AUDIO_ROADMAP.md` — staged route from current psychoacoustic product to defensible physical/hybrid modes.
- `AUDIO_QUALITY_STANDARD.md` — signal quality, stereo, automation, measurement and listening.
- `EXPERIMENT_AND_VALIDATION_PROTOCOL.md` — reproducible experiments and error budgets.
- `QUALITY_SCORECARD.md` — go/no-go scoring without averaging away blockers.
- `ASSUMPTIONS_REGISTER.md` — explicit assumptions, evidence and invalid regimes.
- `PROMPT_LIBRARY.md` — high-value Japanese prompts for future sessions.
- `REFERENCE_SOURCES.md` — primary documentation and standards entry points.
- `OPTIONAL_AUTOMATION.md` — optional hooks and stronger local enforcement.

## Maintenance rule

When Claude makes the same class of mistake twice, add the correction at the lowest-cost correct layer:

- universal fact or invariant → `CLAUDE.md` / `AGENTS.md`;
- applies only to files or directory → scoped rule;
- repeatable procedure → skill;
- repeatable specialist review → subagent;
- deep explanation → this directory;
- deterministic enforcement → script or hook;
- product decision → ADR;
- measurement/listening result → experiment record;
- uncertain premise → assumptions register.

Keep `CLAUDE.md` short. Long always-loaded instructions reduce adherence and consume the context needed for actual engineering.
