# DNA Orbit Governance

Start here when deciding whether and how a change should exist.

## Stable intent

- `PRODUCT_CONSTITUTION.md` — product identity, pillars, invariants, decision order, and amendment authority.
- `quality/constitution.sha256` — integrity lock. A mismatch is not fixed casually.

Product Constitution 1.0.0 is implemented as a draft normative foundation. Its exact wording still requires explicit product-owner approval before release-mode acceptance. A governance script, agent, reviewer, merge, or tag cannot grant that approval.

## Operating policy

- `QUALITY_GOVERNANCE_SYSTEM.md` — lifecycle gates, quality ratchet, decision rights, concept-drift control.
- `CHANGE_RISK_MODEL.md` — R0–R4 classification and required evidence/review.
- `DECISION_AND_EXCEPTION_POLICY.md` — decisions, temporary exceptions, quality debt, and rollback.
- `quality/governance.json` — machine-readable policy.

## Per-change records

- `docs/quality/changes/` — change contracts. Every R2+ path must be covered, and declared risk must not be lower than path risk.
- `docs/quality/reviews/` — independent review evidence.
- `docs/quality/exceptions/` — temporary bounded exceptions.
- `docs/quality/QUALITY_DEBT_LEDGER.md` — accepted non-blocking debt.
- `docs/quality/RELEASE_DECISION_LOG.md` — explicit release authorisation.

## Commands

Development governance audit:

```sh
bash scripts/quality-gate.sh
```

Release-authorisation audit:

```sh
bash scripts/quality-gate.sh --release
```

A passing command means the governance structure is internally consistent. It does not prove audio quality, usability, host behavior, legal compliance, owner intent, or release readiness.

## Human and agent responsibility

Automation detects structural violations such as hash mismatch, missing artifacts, under-declared path risk, uncovered R2+ files, missing R3/R4 records, and expired exceptions. It cannot determine whether a matching record is semantically honest, whether the sound is better, or whether the whole product is truly moving in the best direction. Those remain explicit independent-review and product-owner responsibilities.
