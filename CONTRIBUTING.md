# Contributing to DNA Orbit

DNA Orbit is an audio plug-in. A change can compile and still damage sound, old sessions, real-time safety, host behavior, or product truth. Follow the repository engineering foundation rather than treating this as a generic C++ project.

## Start here

Read:

1. `AGENTS.md`;
2. `CLAUDE.md` when using Claude Code;
3. `docs/claude-code/README.md`;
4. `docs/claude-code/CURRENT_ARCHITECTURE_MAP.md`;
5. relevant scoped rules, ADRs, tests, and `MANUAL_REQUIRED.md`.

Run:

```sh
bash scripts/agent-preflight.sh
```

Do not make production changes directly on `main`.

## Change brief

Before non-trivial work, define:

- user and musical outcome;
- current behavior and evidence of the problem;
- truth class: EXACT / NUMERICAL APPROXIMATION / PSYCHOACOUSTIC MODEL / ARTISTIC EXTENSION / UNKNOWN;
- equations/state machine, units, coordinates, timing, and assumptions;
- invariants and old-session compatibility;
- automated measurement, listening, host, and performance plan;
- rollback boundary.

Claude Code users should begin with:

```text
/start-dsp-task <outcome>
```

## Implementation expectations

- Keep parameter IDs and external product identifiers stable.
- Add schema migration when persistent semantics change.
- Preserve exact Dry, bypass, and documented compatibility paths.
- Keep audio callbacks allocation-free, lock-free, bounded, and free of I/O/logging/UI calls.
- Smooth audible discontinuities deliberately.
- Add regression/property tests for the intended invariant.
- Keep physical and psychoacoustic claims honest.
- Separate behavior changes from unrelated refactoring.
- Update ADRs, assumptions, documentation, and manual gates with the code.

## Evidence

Use the applicable project skills and specialist reviews. Run:

```sh
bash scripts/static-realtime-audit.sh
bash scripts/validate-local.sh
```

A contribution is not complete merely because code exists. Report separately:

- Designed;
- Implemented;
- Compiled;
- Tested;
- Measured;
- Listened;
- Host-validated;
- Release-ready.

Never claim a stage that was not actually completed.

## Pull requests

- Open as draft until applicable automated and manual gates pass.
- Explain product outcome, root cause, model/assumptions, compatibility, sound impact, real-time impact, tests, measurements, listening, host validation, CPU, and remaining risk.
- Include exact commands and environment.
- Preserve failed or inconclusive evidence rather than presenting only favorable results.
- Blockers and High findings cannot be averaged away by a quality score.

## Research and experiments

Use primary sources and record version/date. Store significant comparisons under `docs/experiments/` using the repository experiment protocol. Do not commit copyrighted audio or restricted datasets without permission; commit metadata, generation scripts, hashes, and derived results where appropriate.

## Release work

Merging code is not distribution approval. Follow `MANUAL_REQUIRED.md` for validators, DAWs, listening, licensing, signing, notarization, packaging, and clean-machine checks.
