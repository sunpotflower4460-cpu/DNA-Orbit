---
name: quality-governance
description: Apply DNA Orbit's constitution, risk model, change contract, evidence gates, independent review, exception policy, and release decision rules. Use before substantial work, whenever scope expands, before leaving draft, and before release.
---

# Quality governance workflow

## 1. Read the governing intent

Read:

- `docs/governance/PRODUCT_CONSTITUTION.md`;
- `docs/governance/QUALITY_GOVERNANCE_SYSTEM.md`;
- `docs/governance/CHANGE_RISK_MODEL.md`;
- relevant ADRs and assumptions;
- current change record, if one exists.

Do not edit the constitution as part of ordinary feature work.

## 2. Run constitutional fit before implementation

Answer:

1. What musician/user outcome improves?
2. Why is this DNA Orbit?
3. Which pillars are strengthened or threatened?
4. What truth class applies?
5. What result would reject the proposal?
6. What old-session/default/preset behavior can change?
7. What complexity is added and removed?
8. What is the rollback boundary?

If the proposal cannot answer these, narrow it to an experiment or stop.

## 3. Assign risk

Use the highest credible consequence and `CHANGE_RISK_MODEL.md`.

- R0: editorial;
- R1: internal/no product behavior;
- R2: bounded user-facing change;
- R3: core DSP/host/state behavior;
- R4: constitutional, irreversible, identifier, migration-strategy, legal, or release-critical.

Do not de-escalate because the diff is small.

## 4. Create or update the change record

For R2–R4, create a file from `docs/quality/changes/TEMPLATE.md`.

For R3/R4 also require:

- relevant ADR;
- independent review record;
- explicit compatibility and rollback plan;
- falsifiable evidence plan.

For R4, record the previous promise being changed and leave owner approval pending until explicitly granted.

## 5. Protect the quality ratchet

List established tests, exact endpoints, migrations, validators, user workflows, and baselines that this change can weaken.

A regression needs an explicit higher-order benefit, evidence, ADR, and owner approval when it changes user or constitutional contracts.

## 6. Govern scope changes

When implementation expands beyond the original change record:

- stop adding code;
- update risk and affected pillars;
- add reviewers/evidence;
- split unrelated work;
- revise rollback and rejection criteria.

Scope expansion without governance update is concept drift.

## 7. Handle exceptions and debt

Never silently bypass a gate.

- Use `docs/quality/exceptions/TEMPLATE.md` for a temporary, bounded exception.
- Give it owner, expiry, compensating controls, and removal plan.
- Record accepted non-blocking debt in `QUALITY_DEBT_LEDGER.md`.
- Never waive hearing safety, realtime safety, truthful claims, compatibility contracts, or evidence honesty.

## 8. Independent challenge

Delegate to `quality-governor` and the domain specialists needed to falsify the highest-risk assumption.

Ask reviewers to identify:

- local optimisation that harms the whole;
- false physical or quality claims;
- hidden compatibility change;
- complexity without user value;
- evidence gaps;
- rollback weakness;
- constitution drift.

## 9. Run the gate

During development:

```sh
bash scripts/quality-gate.sh
```

Before release authorisation:

```sh
bash scripts/quality-gate.sh --release
```

Do not update the constitution hash merely to silence a mismatch. Follow the amendment process.

## 10. Decide honestly

Use only:

- ACCEPT;
- ACCEPT WITH DEBT;
- EXPERIMENT ONLY;
- REVISE;
- REJECT;
- ROLL BACK.

Report Designed/Implemented/Compiled/Tested/Measured/Listened/Host-validated/Release-ready separately. A passing governance script proves policy structure, not sound or usability.
