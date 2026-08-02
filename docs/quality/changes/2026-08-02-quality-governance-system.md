# Change record: durable quality governance system

- Change ID: CHG-20260802-quality-governance
- Date: 2026-08-02
- Owner: repository product owner
- Branch / PR: agent/world-class-dsp-phase3 / PR #2
- Risk tier: R4 / CONSTITUTIONAL
- Covered paths: docs/governance/*; quality/*; scripts/quality-*; scripts/agent-preflight.sh; scripts/validate-local.sh; AGENTS.md; CLAUDE.md; CONTRIBUTING.md; .github/pull_request_template.md; docs/claude-code/README.md; docs/claude-code/QUALITY_SCORECARD.md; docs/commercial-upgrade/manifest.json; docs/commercial-upgrade/decisions/ADR-010-quality-governance-system.md; .claude/skills/quality-governance/SKILL.md; .claude/agents/quality-governor.md; docs/quality/*
- Decision: PROPOSED

## User and musical outcome

Future implementation must improve DNA Orbit without drifting from its musical identity, physical honesty, sound quality, usability, session reliability, or long-term coherence. High-risk decisions should require explicit evidence and independent challenge.

## Constitutional fit

- Affected pillars: P1–P10.
- Why this belongs in DNA Orbit: it protects the two-strand/centre concept and establishes a stable decision hierarchy for DSP, physics, UI, state, and release work.
- Whole-product benefit: local improvements cannot silently override higher product priorities.
- Constitutional conflict: none known; final wording review remains a release gate.

## Risk classification

- Highest credible failure consequence: poor governance could freeze useful work, create empty paperwork, or encode the wrong product intent.
- Protected paths affected: constitution, agent contracts, validation, PR process, policy manifest.
- Why this tier is sufficient: the change defines how future changes are accepted.
- Escalation triggers: changing identity, decision order, invariants, approval authority, or release blockers.

## Truth class and assumptions

- Truth class: product governance plus exact repository integrity checks.
- State model: risk tiers R0–R4 and gates G0–G6.
- Assumptions: Git and Python 3 are available locally; contributors follow the repository contract.
- Invalid regimes: automation cannot prove sound quality, usability, legal compliance, or product-owner intent.
- Primary references: existing engineering contract, scorecard, ADR system, tests, and the explicit request for durable governance.

## Protected invariants

- Blockers cannot be averaged away.
- Agents cannot self-authorise constitutional changes.
- Safety, compatibility, truth, and evidence remain above convenience.
- High-risk changes require rollback and independent review.
- Automated governance never claims to prove subjective quality.

## Compatibility and migration

- Parameter IDs/version hints: unchanged.
- State schema: unchanged.
- Old-session sound: unchanged.
- Presets/defaults: unchanged.
- Host automation: unchanged.
- Migration fixtures: not applicable.

## Evidence plan and rejection conditions

- Automated tests: auditor self-test plus file, hash, risk, path coverage, record, ADR, review, and exception checks.
- Numerical/audio measurements: not applicable.
- Level-matched listening: not applicable.
- UI/interaction evidence: not applicable.
- Host/validator evidence: full local validation must still run.
- Performance evidence: local execution time should remain negligible.
- Reject or roll back if the system blocks normal low-risk work, is easily bypassed, misstates intent, or rewards mechanical box-checking.

## Independent review

- Required reviewers: quality governor, validation architect, and product-owner wording review before release.
- Review records: `docs/quality/reviews/2026-08-02-quality-governance-review.md`.
- Blocker findings: none established without local execution.
- High findings: local script execution and first real agent use remain unverified.
- Disagreements and resolution: none recorded.

## Rollback boundary

Remove the governance audit from local validation first if it behaves incorrectly, retaining records for diagnosis. Full rollback is a revert of governance-only files and contract changes; no audio migration is required.

## Completion state and decision

- Designed: yes.
- Implemented: yes, in the draft branch.
- Compiled: not applicable to documents; scripts not locally executed here.
- Tested: not yet run on the development Mac.
- Measured: not yet.
- Governance-audited: not yet run locally.
- Screenshot-verified: not applicable.
- Keyboard-tested: not applicable.
- Listened: not applicable.
- Host-validated: not yet.
- Observed-user validated: not yet.
- Owner-approved where required: pending for constitution wording.
- Release-ready: no.
- Remaining debt/exceptions: no exception granted; wording approval and local execution remain open.
- Final decision and rationale: PROPOSED / IMPLEMENTED FOR DRAFT.
