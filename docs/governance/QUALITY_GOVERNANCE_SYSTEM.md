# DNA Orbit Quality Governance System

## Purpose

This system exists so that increasing implementation volume does not cause concept drift, quality dilution, hidden debt, false certainty, or local optimisations that damage the whole product.

The Product Constitution defines what must remain true. This document defines how changes are proposed, challenged, evidenced, approved, monitored, and, when necessary, rejected or rolled back.

## Governance principles

1. **Hard constraints come before scores.** A Blocker cannot be averaged away.
2. **Whole-product value outranks local elegance.** A locally clever change may still be rejected.
3. **Evidence is cumulative.** New regression protection becomes part of the permanent quality floor.
4. **Unknown is an acceptable status.** Pretending certainty is not.
5. **Independent review is required for high-risk change.** The implementation agent cannot approve its own assumptions.
6. **Exceptions are temporary liabilities.** They need an owner, scope, expiry, and removal plan.
7. **Constitutional change is rare and explicit.** Normal feature work cannot rewrite the product’s purpose.
8. **Release is a separate decision.** Merged code is not automatically shippable code.

## Governance layers

### Layer 0 — Product Constitution

`docs/governance/PRODUCT_CONSTITUTION.md`

Defines identity, protected pillars, invariants, decision order, and amendment authority. Its SHA-256 is locked in `quality/constitution.sha256`.

### Layer 1 — Enforceable policy

- `quality/governance.json`
- this document;
- `CHANGE_RISK_MODEL.md`;
- `DECISION_AND_EXCEPTION_POLICY.md`;
- `AGENTS.md` and `CLAUDE.md`.

Defines risk tiers, protected paths, required artifacts, mandatory reviews, and release behavior.

### Layer 2 — Change contracts

Every substantial change receives a record under `docs/quality/changes/`.

The record connects a concrete implementation to:

- user and musical outcome;
- constitutional pillars;
- truth class;
- protected invariants;
- risk tier;
- compatibility and rollback;
- evidence plan;
- independent review;
- actual completion state.

### Layer 3 — Evidence and review

- tests and fixtures;
- `docs/experiments/`;
- `docs/quality/reviews/`;
- validator logs;
- screenshot matrix;
- benchmark results;
- listening and DAW records.

Evidence must preserve negative, failed, and inconclusive results when they affect the decision.

### Layer 4 — Exceptions and debt

- `docs/quality/exceptions/`;
- `docs/quality/QUALITY_DEBT_LEDGER.md`.

No exception is permanent. No quality debt is hidden in prose, chat, or memory.

### Layer 5 — Automated governance audit

`scripts/quality-governance-audit.py` and `scripts/quality-gate.sh` verify:

- constitution integrity;
- required governance artifacts;
- changed-path risk classification;
- presence and completeness of change records;
- required ADR/review artifacts for critical change;
- expired exceptions;
- quality-debt and release-decision structure.

The automated audit cannot prove sound quality or usability. It proves that the required decision and evidence system is present and internally consistent.

## Change lifecycle

### Gate G0 — Constitutional fit

Before design:

- state the user outcome;
- identify affected constitutional pillars;
- explain why the change belongs in DNA Orbit;
- identify a rejection condition;
- classify the truth model;
- assign a risk tier.

A change that cannot pass G0 should not be implemented merely to “see what happens.” Use an isolated experiment instead.

### Gate G1 — Design contract

Before production code:

- define the current baseline;
- define equations/state machine and assumptions where applicable;
- list protected invariants;
- define old-session and preset behavior;
- define evidence, listening, host, UI, and CPU plans;
- define rollback or feature-disable boundary.

### Gate G2 — Implementation integrity

During implementation:

- make the smallest coherent change;
- preserve unrelated behavior;
- add tests for the intended invariant;
- keep docs, ADRs, state migration, and UI language aligned;
- do not hide debt behind temporary code without a ledger entry.

### Gate G3 — Evidence

Evidence is evaluated in layers:

1. structural and static checks;
2. compilation and automated tests;
3. numerical/audio measurements;
4. level-matched listening;
5. UI screenshot and interaction checks;
6. real-host validation;
7. distribution and clean-machine checks.

Not every change requires every layer, but every omitted layer must have an explicit reason.

### Gate G4 — Independent challenge

R3 and R4 changes require at least one independent review record. Use the specialist that can falsify the highest-risk assumption, not the reviewer most likely to agree.

Relevant reviewers include:

- quality governor;
- physics/numerical auditor;
- audio-quality auditor;
- real-time safety reviewer;
- validation architect;
- UI/UX auditor.

### Gate G5 — Decision

Allowed decisions:

- **ACCEPT** — evidence supports the outcome and no unresolved release blocker remains for the intended stage;
- **ACCEPT WITH DEBT** — only non-blocking debt, explicitly owned and time-bounded;
- **EXPERIMENT ONLY** — useful learning, not product behavior;
- **REVISE** — concept fits, evidence or implementation is insufficient;
- **REJECT** — violates constitution, user contract, safety, or product coherence;
- **ROLL BACK** — evidence shows regression or risk exceeds benefit.

### Gate G6 — Release authorisation

Release mode is stricter than development mode. It blocks on:

- pending constitutional approval;
- expired exceptions;
- unresolved Blocker or High findings required by policy;
- incomplete required evidence;
- false completion claims;
- missing signing, validator, host, listening, or distribution gates.

## Quality ratchet

The repository has a one-way quality floor:

- a fixed bug gains a regression test where feasible;
- an established exact endpoint remains exact;
- a validated migration remains covered by fixtures;
- a discovered host failure becomes a permanent release check;
- a user-critical workflow becomes a UI/DAW test requirement;
- a measured baseline remains comparable across future work.

Weakening a ratchet item is an explicit regression decision, not maintenance.

## Concept-drift control

Concept drift is checked at four moments:

1. every substantial change record;
2. every R3/R4 independent review;
3. before a PR leaves draft;
4. before each release.

The drift review asks:

- Is the product still primarily a musical spatial-orbit effect?
- Is the centre/relationship concept still meaningful?
- Did controls grow faster than user understanding?
- Did visual complexity outrun audible value?
- Did “physical” language outrun the model?
- Did optimisation harm sound or interaction?
- Did compatibility debt accumulate?
- Is the default path still simple?
- Can the newest feature be removed without breaking product identity?
- Are we improving the whole or only adding surface area?

## Decision rights

- **Product owner:** final authority over constitutional intent, subjective product direction, irreversible tradeoffs, and release.
- **Implementation agent/developer:** may design and implement within the constitution; may not self-authorise constitutional amendments or unresolved exceptions.
- **Independent reviewer:** may issue Blocker/High findings and require evidence; cannot rewrite product intent.
- **Automated gate:** may block on structural policy violations; cannot approve subjective sound or usability.

When authorities disagree, the strictest safety/compatibility interpretation applies while the product owner resolves product intent.

## Required records

For a substantial change:

- one change record;
- one ADR when architecture, behavior, compatibility, defaults, or constitutional interpretation changes;
- one independent review for R3/R4;
- experiment/evidence records where competing models or subjective tuning are involved;
- exception record for any temporary policy bypass;
- quality debt ledger entry for any accepted non-blocking debt.

## Governance health review

Before a release, review the governance system itself:

- Are policies still understandable and enforceable?
- Are agents bypassing records mechanically?
- Are exceptions accumulating or expiring?
- Are scorecards producing false comfort?
- Are tests protecting meaningful outcomes or only implementation details?
- Are manual gates realistically executed?
- Has owner intent changed?
- Do the constitution and actual product still match?

Governance that creates paperwork without improving decisions must be simplified. Governance that can be silently bypassed must be strengthened.
