# DNA Orbit Quality Records

This directory is the durable evidence and accountability layer of the quality governance system.

## Directories

- `changes/` — one current contract per substantial coherent change. Every R2+ path must be covered by at least one changed record.
- `reviews/` — independent reviews with severity-ranked findings and actual evidence examined.
- `exceptions/` — temporary policy exceptions with owner, expiry, controls, removal, and rollback.

## Ledgers

- `QUALITY_DEBT_LEDGER.md` — accepted non-blocking debt only.
- `RELEASE_DECISION_LOG.md` — explicit owner release decisions for exact commits/artifacts.

## Rules

- Records describe actual scope; do not copy an old record merely to pass automation.
- Failed, negative, and inconclusive evidence remains visible when it affects the decision.
- Blockers cannot be converted into debt.
- High debt stays outside shipping scope.
- Resolved exceptions and debt remain as history.
- Automated PASS is structural evidence, not sound, host, usability, legal, owner, or release approval.
