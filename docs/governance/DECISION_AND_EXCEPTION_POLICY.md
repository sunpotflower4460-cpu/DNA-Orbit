# DNA Orbit Decision, Exception, and Quality-Debt Policy

## Decision records

Every governed change ends with one explicit decision:

- `ACCEPT`
- `ACCEPT WITH DEBT`
- `EXPERIMENT ONLY`
- `REVISE`
- `REJECT`
- `ROLL BACK`

Silence, a merged commit, or an enthusiastic summary is not a decision record.

## What cannot receive an exception

No waiver may permit:

- a known hearing-risk event;
- crash, hang, corruption, or unbounded output in intended use;
- audio-thread allocation, locking, blocking I/O, or UI/message-thread calls;
- knowingly false physical, product, validation, or completion claims;
- evidence reported as run when it was not;
- silent parameter-ID or state-contract breakage;
- release with an unresolved constitutional conflict;
- distribution without required legal/signing authority.

These must be fixed, scoped out, or the change rejected.

## Temporary exceptions

An exception is allowed only for a non-constitutional, non-safety requirement when immediate removal would create a larger documented risk or when a staged experiment needs bounded flexibility.

Every exception record under `docs/quality/exceptions/` must contain:

- unique ID;
- status: `proposed`, `active`, `resolved`, or `revoked`;
- owner;
- creation date;
- expiry date;
- affected rule and files;
- precise scope;
- reason the normal rule cannot yet be met;
- user/product risk;
- compensating controls;
- verification method;
- removal plan;
- rollback trigger;
- approval authority.

No `active` exception may omit an expiry. “Until later,” “temporary,” and “after release” are not expiry dates.

## Exception behavior

- Expired active exceptions are automated governance failures.
- An exception applies only to its written scope.
- New work may not copy an exception as precedent without a new decision.
- An exception cannot lower the truthfulness of status reporting.
- Release mode blocks when an applicable exception lacks required owner approval.
- Resolved exceptions remain as history; do not delete the evidence trail.

## Quality debt

Quality debt is a known non-blocking deficiency accepted with a concrete reason. It belongs in `docs/quality/QUALITY_DEBT_LEDGER.md`.

Each entry needs:

- ID;
- severity;
- originating change/ADR;
- affected constitutional pillar or quality domain;
- user-visible consequence;
- owner;
- target removal milestone/date;
- objective exit condition;
- current status;
- evidence or issue link.

Debt without an owner or exit condition is an unbounded product promise and must not be accepted.

## Debt limits

- Blocker debt is forbidden.
- High debt may exist only on an experiment branch or explicitly narrowed non-release scope.
- Medium debt must have a near-term plan and cannot accumulate repeatedly in the same protected area.
- Low debt is reviewed at each release.
- Repeated debt in one domain escalates the next related change’s risk tier.

## Product-owner decisions

Product-owner approval is required for:

- constitutional amendments;
- irreversible behavior or compatibility changes;
- subjective product-direction conflicts that evidence cannot resolve;
- accepted regression against an established quality ratchet;
- final release authorisation;
- legal, licensing, signing, or distribution exceptions.

Approval must name the tradeoff. A generic “looks good” does not approve an unspecified regression.

## Independent-review disagreements

When reviewers disagree:

1. identify the exact disputed premise;
2. design the smallest test or listening experiment that can distinguish the claims;
3. preserve both positions in the review record;
4. default to the safer compatibility interpretation while unresolved;
5. ask the product owner only for genuine product values, not technical facts that can be measured.

## Rollback policy

Rollback is a normal governance outcome, not failure.

Rollback immediately when:

- a non-negotiable invariant regresses;
- evidence contradicts the core benefit;
- a host/session compatibility issue cannot be safely migrated;
- complexity exceeds demonstrated value;
- a feature causes repeated exceptions or debt;
- the implementation cannot be validated within its intended truth class.

A rolled-back experiment may remain documented and may inform a different design later.
