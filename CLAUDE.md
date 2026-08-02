@AGENTS.md

# Claude Code operating instructions

Keep this file concise. Stable product intent belongs in `docs/governance/PRODUCT_CONSTITUTION.md`; procedures belong in `.claude/skills/`; file-specific rules belong in `.claude/rules/`; deep reference material belongs in `docs/claude-code/` and `docs/governance/`.

## Session start

- Run `bash scripts/agent-preflight.sh` before substantial work.
- Read the Product Constitution, active PR, current diff, recent commits, active change record, exceptions/debt, `docs/claude-code/CURRENT_ARCHITECTURE_MAP.md`, and relevant ADRs.
- Use `quality-governance` before substantial work, whenever scope expands, before leaving draft, and before release.
- Use `start-dsp-task` for non-trivial DSP work.
- Use `ui-ux-audit` for non-trivial editor, LookAndFeel, control grouping, wording, screenshot, accessibility, or visualiser presentation work.
- Use plan mode for R3/R4 changes, DSP architecture, state schema, transport, bypass, saved-session compatibility, UI information architecture, constitution/governance, or more than three production files.
- Do not begin a broad rewrite until the existing implementation, tests, constitutional fit, risk, and rollback have been traced end-to-end.

## Governance rules

1. Assign R0–R4 risk from worst credible consequence, not diff size.
2. Create/update a change record for R2–R4 work.
3. Require an ADR and independent review for R3/R4 work.
4. Stop implementation when scope expands until governance artifacts are updated.
5. Never modify the Product Constitution as a side effect or update its hash to silence an audit.
6. Never self-authorise owner approval, constitutional amendment, irreversible regression, exception, or release.
7. Record temporary exceptions with owner, scope, expiry, compensating controls, and removal plan.
8. Record accepted non-blocking debt in the quality debt ledger.
9. Run `bash scripts/quality-gate.sh` during development and `bash scripts/quality-gate.sh --release` before release authorisation.
10. A passing governance audit proves policy consistency only; it does not prove sound, UI, host, or release quality.

## Required quality workflow

For DSP, acoustics, spatial geometry, modulation, filter, delay, gain, phase, or stereo changes:

1. Follow `quality-governance` and establish constitutional fit, risk, rejection condition, and rollback.
2. Invoke or follow `physics-audit`.
3. Invoke or follow `audio-quality-gate`.
4. Use `research-primary-sources` when an API, standard, physical claim, numerical method, validator, or toolchain fact is uncertain or version-sensitive.
5. Use `experiment-design` when choosing between algorithms, physical models, Character values, presets, or subjective alternatives.
6. Use `optimize-dsp-safely` for performance work; no sound-changing approximation without an error budget and evidence.
7. Delegate independent read-only reviews to `quality-governor`, `physics-auditor`, `audio-quality-auditor`, `realtime-safety-reviewer`, and `validation-architect` as applicable.
8. Add tests that encode the intended invariant before declaring success.
9. Run `bash scripts/static-realtime-audit.sh` and the relevant build/tests.

For UI/UX changes:

1. Follow `quality-governance`, `ui-ux-audit`, and `docs/claude-code/UI_UX_STANDARD.md`.
2. Preserve parameter/state contracts and DSP visual truth.
3. Delegate independent reviews to `quality-governor` and `ui-ux-auditor` for non-trivial changes.
4. Generate and inspect the required minimum, standard, wide, warning, Sync, and Bypass screenshots.
5. Distinguish screenshot-verified, keyboard-tested, host-validated, and observed-user evidence.

Before presenting work as complete, invoke or follow `adversarial-review`, `quality-governance`, and `release-gate`.

## Context discipline

- Do not preload every design document. Read only the references needed for the current task.
- Prefer focused subagents for large searches, logs, standards research, governance challenge, and hostile review so the main context retains implementation decisions.
- After `/compact`, re-read this file, the Product Constitution, and the active task’s skill/change record if behavior becomes inconsistent.
- Durable discoveries belong in source comments, tests, ADRs, change/review/experiment records, or `docs/claude-code/ASSUMPTIONS_REGISTER.md`; do not rely on machine-local auto memory alone.
- If an existing change record no longer describes the current work, update it or create a new one; do not reuse it mechanically to satisfy the audit.

## Autonomy

Work autonomously through all safe, reversible, locally verifiable steps. Do not stop merely to ask whether to run tests, update documentation, fix issues caused by the current change, or complete governance records.

Stop and request human input only for genuinely subjective listening/product choices, constitution wording approval, credentials/signing, destructive Git operations, irreversible release actions, legal/licensing decisions, or ambiguity that could change the intended product identity or sound.

## Completion language

Use these states precisely:

- **Designed** — specification exists.
- **Implemented** — code exists.
- **Compiled** — relevant target built successfully.
- **Tested** — named automated tests passed.
- **Measured** — named metrics and environment recorded.
- **Governance-audited** — development or release governance command passed at a named commit; not owner approval.
- **Screenshot-verified** — required renders were generated and visually inspected.
- **Keyboard-tested** — focus traversal and activation were manually checked.
- **Listened** — level-matched listening completed on named material.
- **Host-validated** — named DAWs/formats passed.
- **Observed-user validated** — a target user completed the intended workflow without coaching or hidden assistance.
- **Owner-approved** — the product owner explicitly approved the named constitutional/product tradeoff.
- **Release-ready** — every applicable governance, technical, manual, legal, and distribution gate passed.

Never collapse these into a generic “done.”
