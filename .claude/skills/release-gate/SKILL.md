---
name: release-gate
description: Verify whether DNA Orbit work is actually ready to leave draft status. Use before declaring a task complete, marking a PR ready, merging, tagging, packaging, or distributing binaries.
---

# DNA Orbit release gate

Read:

- `docs/governance/PRODUCT_CONSTITUTION.md`;
- `docs/governance/QUALITY_GOVERNANCE_SYSTEM.md`;
- active change records, ADRs, independent reviews, exceptions, and quality debt;
- `MANUAL_REQUIRED.md`;
- `docs/claude-code/QUALITY_SCORECARD.md`;
- active PR and changed files.

## Governance gate

Run development governance first:

```sh
bash scripts/quality-gate.sh
```

Confirm:

- constitution hash verified;
- every R2+ path is covered by a current change record;
- declared risk is not below covered path risk;
- required ADRs and independent reviews exist;
- no expired active exception;
- no Blocker debt or High debt in shipping scope;
- change records reflect actual current scope;
- unresolved constitutional/product-owner decisions are explicit.

Before any release authorisation run:

```sh
bash scripts/quality-gate.sh --release
```

A passing governance command is structural evidence only. It does not prove sound, UI, host behavior, legal compliance, or owner approval unless those records explicitly exist.

## Automated engineering gate

Run, or explain precisely why unavailable:

```sh
bash scripts/agent-preflight.sh
bash scripts/static-realtime-audit.sh
bash scripts/validate-local.sh
```

Confirm:

- clean Release compile;
- full CTest pass;
- sanitizer pass where supported;
- benchmark output captured;
- headless UI screenshots generated and inspected;
- pluginval strictness 10;
- Steinberg VST3 Validator;
- `auval` for AU on macOS;
- no unexplained warnings or skipped tests.

## Behavior gate

Check applicable items:

- exact Dry, Soft Bypass, Bass Anchor Off, state round-trip, and preset completeness;
- no NaN/Inf, denormal spike, DC surprise, click, zipper, or unstable output;
- mono, stereo, pure Side, anti-phase, hard-pan, silence, and non-finite inputs;
- supported sample rates and variable block sizes;
- automation for every changed parameter in both directions;
- transport stop/start/loop/seek/tempo/time-signature behavior;
- duplicate realtime and offline renders;
- old schema fixtures and current project restore;
- UI resize, DPI, keyboard, accessibility names, warnings, observed first-use workflow, and hidden-editor CPU.

## Musical gate

Require a recorded level-matched listening result for every user-facing sound change. Include material, monitoring, settings, comparisons, uncertainty, and unresolved preference decisions.

## Host and packaging gate

Require named DAW/format results, clean-machine installation, signing/notarization, license confirmation, and artifact hashes before distribution.

## Independent governance challenge

Delegate `quality-governor` plus relevant domain specialists. Resolve or explicitly retain every Blocker/High finding. The implementation agent cannot mark owner approval.

## Score and verdict

Complete the scorecard. Averages cannot hide a constitutional conflict, Blocker, expired exception, or release-relevant High finding.

Return exactly one:

- **NOT READY — governance incomplete**
- **NOT READY — implementation incomplete**
- **NOT READY — validation incomplete**
- **EXPERIMENTAL ONLY**
- **DRAFT RELEASE CANDIDATE**
- **READY FOR REVIEW**
- **ELIGIBLE FOR PRODUCT-OWNER RELEASE DECISION**
- **RELEASE READY**

`RELEASE READY` additionally requires a product-owner decision recorded in `docs/quality/RELEASE_DECISION_LOG.md` for the exact commit/artifacts.

List evidence for every passed category and owner/action for every open item. Never infer manual gates, owner approval, or release from automated tests, merge, tag, or build success.
