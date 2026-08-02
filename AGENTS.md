# DNA Orbit — Agent Engineering Contract

This file is the vendor-neutral contract for every coding agent working in this repository.
The product goal is not merely “working code.” The goal is a world-class musical spatial effect whose behavior is physically honest, numerically stable, sonically excellent, reproducible, humane to use, and safe in real DAWs.

## Governing authority

Read `docs/governance/PRODUCT_CONSTITUTION.md` before substantial work. It is the stable source of product identity and decision order.

Detailed governance lives in:

- `docs/governance/QUALITY_GOVERNANCE_SYSTEM.md`;
- `docs/governance/CHANGE_RISK_MODEL.md`;
- `docs/governance/DECISION_AND_EXCEPTION_POLICY.md`;
- `quality/governance.json`;
- per-change records under `docs/quality/`.

No agent may edit the Product Constitution as a side effect of feature work or update its hash merely to silence an audit. Constitutional change is R4, needs a dedicated ADR/review, explicit product-owner approval, migration/rollback, version/hash update, and release-mode governance audit.

## Priority order

When goals conflict, use this order:

1. Protect hearing, files, sessions, and user data.
2. Preserve audio-thread safety and host stability.
3. Preserve saved-session and parameter-ID compatibility.
4. Preserve mathematical and physical truth.
5. Preserve or improve audible quality.
6. Preserve deterministic behavior and testability.
7. Improve immediate usability, accessibility, and product coherence.
8. Improve CPU and memory efficiency using measurements.
9. Improve visual polish and implementation elegance.

Never trade a higher item for a lower item without an explicit ADR, change record, evidence, and required approval.

## Quality governance entry gate

Before substantial work:

1. run `bash scripts/agent-preflight.sh`;
2. run or follow the `quality-governance` skill;
3. assign R0–R4 risk from worst credible consequence, not diff size;
4. create/update a change record for R2–R4 work;
5. define constitutional fit, rejection conditions, rollback, and evidence;
6. require ADR and independent review for R3/R4 work.

When scope expands, stop adding code until the change record, risk tier, reviewers, and evidence plan are updated.

## Truth labels

Every model, comment, UI claim, README statement, and design proposal must fit one label:

- **EXACT** — follows directly from mathematics or a defined digital signal operation.
- **NUMERICAL APPROXIMATION** — approximates a known continuous/discrete model with quantified error.
- **PSYCHOACOUSTIC MODEL** — designed to evoke perception; not claimed as literal physical acoustics.
- **ARTISTIC EXTENSION** — intentionally departs from physical reality for musical value.
- **UNKNOWN / UNVALIDATED** — plausible but not yet demonstrated.

DNA Orbit’s current front/back cues are a psychoacoustic musical model, not an HRTF, room solver, or complete physical propagation renderer. Never describe them as physically exact.

## Mandatory workflow

Before editing:

1. Read this file, `CLAUDE.md` when applicable, the Product Constitution, relevant ADRs, `MANUAL_REQUIRED.md`, and current tests.
2. Inspect the current branch, diff, recent commits, PR state, active exceptions, and quality debt.
3. State desired user outcome, current baseline, invariants, assumptions, risk tier, rejection condition, and validation plan.
4. Identify whether the change is EXACT, approximate, psychoacoustic, artistic, or unknown.
5. Search primary sources when facts, APIs, standards, or acoustic models are uncertain or current.

During implementation:

1. Prefer the smallest coherent change that proves one concept.
2. Add or update tests before claiming an invariant.
3. Keep parameter IDs stable. Add schema migration for new saved state.
4. Keep the audio thread allocation-free, lock-free, exception-free, and free of I/O/logging/UI calls.
5. Smooth every audible discontinuity introduced by automation, transport, mode, filter, delay, gain, or bypass changes.
6. Do not “fix” a failing test by widening tolerance until the physical/numerical reason is understood.
7. Do not optimize from intuition. Establish a baseline and compare quality and CPU on the same machine.
8. Update documentation, ADRs, assumptions, change records, and manual gates in the same change as behavior.
9. Record temporary policy bypasses only as scoped, owned, expiring exceptions.
10. Record accepted non-blocking deficiencies in `docs/quality/QUALITY_DEBT_LEDGER.md`.

Before finishing:

1. Run the narrowest relevant tests, then `bash scripts/quality-gate.sh` and the full local validation sequence.
2. Inspect endpoint behavior, mono/stereo behavior, silence, non-finite input, extreme parameters, sample-rate changes, variable block sizes, automation, bypass, restore, and offline render.
3. Run independent/adversarial review using the repository skills/subagents.
4. Report exactly what passed, failed, could not run, and still requires listening, UI, owner, or DAW verification.
5. Keep the PR draft until all applicable release gates are satisfied.
6. Before release authorisation run `bash scripts/quality-gate.sh --release` and record the decision in `docs/quality/RELEASE_DECISION_LOG.md`.

## Non-negotiable DSP invariants

- No NaN or infinity may escape the processor.
- Silence remains bounded and free from denormal CPU spikes.
- Mix 0% is exact Dry unless a documented latency contract requires otherwise.
- Fully engaged Soft Bypass reaches exact Dry and does not stop internal state advancement.
- Bass Anchor at 20 Hz / Off remains the documented transparent compatibility path.
- Host Lock must be derived from host timeline state and remain reproducible at identical state and position.
- Symmetry 100% must preserve the defined antipodal geometry and centred spatial centroid.
- NULL CORE is intentionally Side-only and must remain clearly marked mono-unsafe.
- UI geometry must reflect actual DSP state, not a decorative approximation presented as truth.

## Real-time code rules

Inside or reachable from `processBlock`, `processBlockBypassed`, or `HelixEngine::process`:

- no heap allocation or deallocation;
- no mutexes, waits, sleeps, condition variables, or blocking atomics;
- no filesystem, networking, console output, logging, modal UI, or message-thread calls;
- no unbounded loops or data-dependent recursion;
- no exceptions crossing the callback;
- no coefficient redesign unless bounded and measured;
- preallocate scratch memory in `prepareToPlay` / `prepare`;
- sanitize external and parameter values at a defined boundary;
- use lock-free atomics only for small UI telemetry and document memory ordering.

## Audio-quality evidence

A change is not “higher quality” because it is more complex. Quality claims require relevant evidence such as:

- null residual or endpoint identity;
- impulse/sweep/multitone response;
- phase, correlation, mono fold-down, and channel-energy behavior;
- alias, THD+N, IMD, noise, or DC measurements when applicable;
- click/zipper and modulation-sideband checks;
- level-matched listening or ABX-style comparison;
- deterministic duplicate renders;
- CPU and memory measurements at 44.1, 48, 96, and 192 kHz.

If a metric is not applicable, say why. Do not optimize a metric that harms the musical purpose.

## Exceptions and debt

No exception may waive hearing safety, crash/corruption protection, real-time safety, truthful claims, evidence honesty, parameter/state contracts, or unresolved constitutional conflict.

Every allowed exception requires a record under `docs/quality/exceptions/` with owner, exact scope, expiry, compensating controls, removal plan, and rollback trigger. Expired active exceptions fail governance audit.

No Blocker debt is accepted. High debt blocks release and must remain outside shipping scope. Other accepted debt needs an owner and objective exit condition in the debt ledger.

## Git and change safety

- Work on a feature branch; never push directly to `main`.
- Never force-push, hard-reset, clean untracked work, or discard user changes without explicit approval.
- Keep commits scoped and descriptive.
- Do not mix unrelated refactors with a DSP behavior change.
- Preserve a readable decision trail in change records, ADRs, reviews, experiments, and the PR body.

## Communication

The user communicates primarily in Japanese. Progress reports and final summaries should be in Japanese, while code identifiers and precise technical terminology may remain English.

Be candid. “Implemented but not compiled,” “compiled but not listened,” “governance-audited but not owner-approved,” and “listened but not validated across hosts” are different states and must never be conflated.
