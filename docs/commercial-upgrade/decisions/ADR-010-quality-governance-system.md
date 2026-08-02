# ADR-010: Constitution-backed quality governance

## Status

Accepted for draft implementation. Product-owner wording approval and local execution remain required before release-mode acceptance.

## Context

DNA Orbit now spans DSP, physical modelling, psychoacoustics, transport, saved state, UI/UX, validation, performance, and future agent-driven development. Existing engineering rules and scorecards describe good practice, but they do not alone prevent:

- gradual product-identity drift;
- score averaging over a severe blocker;
- silent weakening of established quality contracts;
- unbounded temporary exceptions;
- undocumented quality debt;
- an implementation agent approving its own assumptions;
- accidental modification of foundational product intent.

## Decision

Adopt a layered governance system:

1. a stable Product Constitution with a SHA-256 lock;
2. R0–R4 change risk classification;
3. G0–G6 lifecycle gates;
4. mandatory change records for substantial work;
5. ADR and independent-review requirements for R3/R4 changes;
6. explicit exception expiry and quality-debt ledgers;
7. deterministic local governance audit;
8. stricter release mode requiring owner approval and manual evidence;
9. a quality ratchet that prevents silent weakening of established invariants.

The constitution protects musical purpose, the two-strand/centre relationship, physical honesty, audio quality, safety, compatibility, simplicity, visual truth, evidence, and reversibility.

## Alternatives considered

### Keep only AGENTS.md and the scorecard

Rejected because guidance without change records, expiry, risk classification, or integrity checks can be ignored or applied inconsistently.

### Put every rule into one always-loaded instruction file

Rejected because excessive context reduces adherence and makes changes difficult to review. Stable intent, procedures, machine policy, and evidence are separated.

### Fully automate approval

Rejected because sound quality, product identity, usability, and owner intent cannot be proven by a repository script.

### Require the maximum process for every change

Rejected because bureaucracy would discourage small safe improvements and encourage bypass. Risk tiers scale the process.

### Allow permanent waivers

Rejected because permanent exceptions silently become a second constitution.

## Consequences

Positive:

- future agents receive stable product intent;
- high-risk work has explicit rejection and rollback criteria;
- exceptions and debt cannot remain invisible;
- critical changes need independent challenge;
- release decisions become traceable;
- concept drift becomes a recurring review question.

Costs/risks:

- more records for substantial changes;
- policy paths need maintenance as the repository evolves;
- mechanical compliance could replace thoughtful review;
- the first local execution may reveal script assumptions;
- constitution wording can itself encode the wrong intent if not reviewed.

## Safeguards

- R0/R1 changes remain lightweight;
- automation checks structure, not subjective approval;
- governance health is reviewed before releases;
- owner approval is required for constitutional amendments;
- the audit can be removed from validation independently if it produces false blockers while records are retained for diagnosis.

## Validation

Required:

- governance auditor self-test;
- development-mode audit on the current branch;
- intentional hash-mismatch failure test;
- missing-change-record failure test;
- expired-exception failure test;
- release-mode owner-approval failure test;
- complete `scripts/validate-local.sh` integration;
- first real Claude Code use with quality-governance skill and reviewer;
- product-owner review of constitution 1.0.0.

## Rollback

First remove the governance gate from `validate-local.sh` if local development is falsely blocked. Full rollback is a revert of governance-specific files and contract changes. No DSP, state, preset, or host migration is required.
