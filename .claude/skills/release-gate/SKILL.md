---
name: release-gate
description: Verify whether DNA Orbit work is actually ready to leave draft status. Use before declaring a task complete, marking a PR ready, merging, tagging, packaging, or distributing binaries.
---

# DNA Orbit release gate

Read `MANUAL_REQUIRED.md`, `docs/claude-code/QUALITY_SCORECARD.md`, the active PR, and changed files.

## Automated gate

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
- UI resize, DPI, keyboard, accessibility names, warnings, and hidden-editor CPU.

## Musical gate

Require a recorded level-matched listening result for every user-facing sound change. Include material, monitoring, settings, comparisons, and unresolved preference decisions.

## Host and packaging gate

Require named DAW/format results, clean-machine installation, signing/notarization, license confirmation, and artifact hashes before distribution.

## Score and verdict

Complete the scorecard. Averages cannot hide a blocker.

Return exactly one:

- **NOT READY — implementation incomplete**
- **NOT READY — validation incomplete**
- **EXPERIMENTAL ONLY**
- **DRAFT RELEASE CANDIDATE**
- **READY FOR REVIEW**
- **RELEASE READY**

List evidence for every passed category and owner/action for every open item. Never infer manual gates from automated tests.
