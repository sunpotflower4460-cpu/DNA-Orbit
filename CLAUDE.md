@AGENTS.md

# Claude Code operating instructions

Keep this file concise. Procedures belong in `.claude/skills/`; file-specific rules belong in `.claude/rules/`; deep reference material belongs in `docs/claude-code/`.

## Session start

- Run `bash scripts/agent-preflight.sh` before substantial work.
- Read the active PR, current diff, recent commits, and relevant ADRs.
- Use plan mode for changes that touch DSP architecture, state schema, transport behavior, bypass, saved-session compatibility, or more than three production files.
- Do not begin a broad rewrite until the existing implementation and tests have been traced end-to-end.

## Required quality workflow

For DSP, acoustics, spatial geometry, modulation, filter, delay, gain, phase, or stereo changes:

1. Invoke or follow the `physics-audit` skill.
2. Invoke or follow the `audio-quality-gate` skill.
3. Delegate independent read-only reviews to `physics-auditor`, `audio-quality-auditor`, and `realtime-safety-reviewer` when the change is non-trivial.
4. Add tests that encode the intended invariant before declaring success.
5. Run `bash scripts/static-realtime-audit.sh` and the relevant build/tests.

Before presenting work as complete, invoke or follow `adversarial-review` and `release-gate`.

## Context discipline

- Do not preload every design document. Read only the references needed for the current task.
- Prefer focused subagents for large searches, logs, standards research, and hostile review so the main context retains implementation decisions.
- After `/compact`, re-read this file and the active task’s skill if behavior becomes inconsistent.
- Durable discoveries belong in source comments, tests, ADRs, or `docs/claude-code/ASSUMPTIONS_REGISTER.md`; do not rely on machine-local auto memory alone.

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
