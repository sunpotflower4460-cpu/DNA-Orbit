@AGENTS.md

# Claude Code operating instructions

Keep this file concise. Procedures belong in `.claude/skills/`; file-specific rules belong in `.claude/rules/`; deep reference material belongs in `docs/claude-code/`.

## Session start

- Run `bash scripts/agent-preflight.sh` before substantial work.
- Read the active PR, current diff, recent commits, `docs/claude-code/CURRENT_ARCHITECTURE_MAP.md`, and relevant ADRs.
- Use `start-dsp-task` for non-trivial DSP work.
- Use plan mode for changes that touch DSP architecture, state schema, transport behavior, bypass, saved-session compatibility, or more than three production files.
- Do not begin a broad rewrite until the existing implementation and tests have been traced end-to-end.

## Required quality workflow

For DSP, acoustics, spatial geometry, modulation, filter, delay, gain, phase, or stereo changes:

1. Invoke or follow the `physics-audit` skill.
2. Invoke or follow the `audio-quality-gate` skill.
3. Use `research-primary-sources` when an API, standard, physical claim, numerical method, validator, or toolchain fact is uncertain or version-sensitive.
4. Use `experiment-design` when choosing between algorithms, physical models, Character values, presets, or subjective alternatives.
5. Use `optimize-dsp-safely` for performance work; no sound-changing approximation without an error budget and evidence.
6. Delegate independent read-only reviews to `physics-auditor`, `audio-quality-auditor`, `realtime-safety-reviewer`, and `validation-architect` when the change is non-trivial.
7. Add tests that encode the intended invariant before declaring success.
8. Run `bash scripts/static-realtime-audit.sh` and the relevant build/tests.

Before presenting work as complete, invoke or follow `adversarial-review` and `release-gate`.

## Context discipline

- Do not preload every design document. Read only the references needed for the current task.
- Prefer focused subagents for large searches, logs, standards research, and hostile review so the main context retains implementation decisions.
- After `/compact`, re-read this file and the active task’s skill if behavior becomes inconsistent.
- Durable discoveries belong in source comments, tests, ADRs, experiment records, or `docs/claude-code/ASSUMPTIONS_REGISTER.md`; do not rely on machine-local auto memory alone.

## Autonomy

Work autonomously through all safe, reversible, locally verifiable steps. Do not stop merely to ask whether to run tests, update documentation, or fix issues directly caused by the current change.

Stop and request human input only for genuinely subjective listening choices, credentials/signing, destructive Git operations, irreversible release actions, legal/licensing decisions, or ambiguity that could change the product’s intended sound.

## Completion language

Use these states precisely:

- **Designed** — specification exists.
- **Implemented** — code exists.
- **Compiled** — relevant target built successfully.
- **Tested** — named automated tests passed.
- **Measured** — named metrics and environment recorded.
- **Listened** — level-matched listening completed on named material.
- **Host-validated** — named DAWs/formats passed.
- **Release-ready** — every applicable gate in `MANUAL_REQUIRED.md` passed.

Never collapse these into a generic “done.”
