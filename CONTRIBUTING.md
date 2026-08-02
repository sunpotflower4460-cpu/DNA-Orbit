# Contributing to DNA Orbit

DNA Orbit is an audio plug-in. A change can compile and still damage sound, product identity, old sessions, real-time safety, host behavior, usability, or physical truth. Follow the repository governance and engineering foundation rather than treating this as a generic C++ project.

## Start here

Read:

1. `docs/governance/PRODUCT_CONSTITUTION.md`;
2. `AGENTS.md`;
3. `CLAUDE.md` when using Claude Code;
4. `docs/governance/QUALITY_GOVERNANCE_SYSTEM.md`;
5. `docs/governance/CHANGE_RISK_MODEL.md`;
6. `docs/claude-code/CURRENT_ARCHITECTURE_MAP.md`;
7. relevant scoped rules, ADRs, tests, assumptions, exceptions/debt, and `MANUAL_REQUIRED.md`.

Run:

```sh
bash scripts/agent-preflight.sh
bash scripts/quality-gate.sh
```

Do not make production changes directly on `main`.

## Risk and change contract

Classify work by worst credible consequence:

- R0 — editorial;
- R1 — internal/no product behavior;
- R2 — bounded user-facing change;
- R3 — core DSP/host/state behavior;
- R4 — constitutional, irreversible, identifier, migration-strategy, legal, or release-critical.

R2–R4 work requires a change record under `docs/quality/changes/`. R3/R4 work also requires a relevant ADR and independent review. R4 needs explicit product-owner approval before release.

Before non-trivial work, define:

- user and musical outcome;
- constitutional pillars and why the change belongs in DNA Orbit;
- current behavior and evidence of the problem;
- truth class: EXACT / NUMERICAL APPROXIMATION / PSYCHOACOUSTIC MODEL / ARTISTIC EXTENSION / UNKNOWN;
- equations/state machine, units, coordinates, timing, and assumptions;
- protected invariants and old-session compatibility;
- automated measurement, listening, UI, host, and performance plan;
- a rejection condition;
- rollback or feature-disable boundary.

Claude Code users should begin with:

```text
/quality-governance <outcome>
/start-dsp-task <outcome>
```

## Constitutional safety

Do not edit `PRODUCT_CONSTITUTION.md` as a side effect or update `quality/constitution.sha256` merely to make the audit pass.

A constitutional amendment requires a dedicated R4 record, ADR, independent review, explicit previous promise being changed, migration/rollback, owner approval, version/hash update, and release-mode audit.

## Implementation expectations

- Keep parameter IDs and external product identifiers stable.
- Add schema migration when persistent semantics change.
- Preserve exact Dry, bypass, and documented compatibility paths.
- Keep audio callbacks allocation-free, lock-free, bounded, and free of I/O/logging/UI calls.
- Smooth audible discontinuities deliberately.
- Add regression/property tests for the intended invariant.
- Keep physical and psychoacoustic claims honest.
- Separate behavior changes from unrelated refactoring.
- Update ADRs, assumptions, change records, documentation, and manual gates with the code.
- Stop when scope expands and update the risk/contract before continuing.

## Evidence

Use the applicable project skills and specialist reviews. Run:

```sh
bash scripts/quality-gate.sh
bash scripts/static-realtime-audit.sh
bash scripts/validate-local.sh
```

A contribution is not complete merely because code exists. Report separately:

- Designed;
- Implemented;
- Compiled;
- Tested;
- Measured;
- Governance-audited;
- Screenshot-verified;
- Keyboard-tested;
- Listened;
- Host-validated;
- Observed-user validated;
- Owner-approved where required;
- Release-ready.

Never claim a stage that was not actually completed.

## Exceptions and quality debt

Do not silently skip a gate.

- Temporary exceptions use `docs/quality/exceptions/TEMPLATE.md` and require owner, exact scope, expiry, compensating controls, removal plan, and rollback trigger.
- Expired active exceptions fail governance audit.
- Accepted non-blocking debt belongs in `docs/quality/QUALITY_DEBT_LEDGER.md` with owner and objective exit condition.
- Blocker debt is forbidden; High debt blocks release and must remain outside shipping scope.

## Pull requests

- Open as draft until applicable governance, automated, and manual gates pass.
- Link the change record, risk tier, affected constitutional pillars, ADRs, independent reviews, experiments, exceptions, and debt.
- Explain product outcome, rejection condition, root cause, model/assumptions, compatibility, sound impact, real-time impact, UI impact, tests, measurements, listening, host validation, CPU, and remaining risk.
- Include exact commands and environment.
- Preserve failed or inconclusive evidence rather than presenting only favorable results.
- Blockers and High findings cannot be averaged away by a quality score.
- A passing governance script is structural evidence, not release approval.

## Research and experiments

Use primary sources and record version/date. Store significant comparisons under `docs/experiments/` using the repository experiment protocol. Do not commit copyrighted audio or restricted datasets without permission; commit metadata, generation scripts, hashes, and derived results where appropriate.

## Release work

Merging code is not distribution approval. Before authorisation run:

```sh
bash scripts/quality-gate.sh --release
```

Then follow `MANUAL_REQUIRED.md` for validators, DAWs, listening, UI/accessibility, licensing, signing, notarization, packaging, clean-machine checks, and record the decision in `docs/quality/RELEASE_DECISION_LOG.md`.
