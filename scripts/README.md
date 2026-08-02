# Local agent, governance, and validation scripts

GitHub Actions is intentionally not used for this project. These scripts provide deterministic local evidence and are called explicitly by Claude Code skills.

The GitHub contents API may not preserve executable bits, so invoke shell scripts through `bash`.

## 1. Agent preflight

```sh
bash scripts/agent-preflight.sh
```

Reports:

- repository root, branch, commit, and warning when editing `main`;
- working tree and staged/unstaged diff summaries;
- recent commits;
- presence of required governance and Claude Code foundation files;
- actual and expected Product Constitution hash where shell tools are available;
- local Git, Python, CMake, compiler, Claude Code, and GitHub CLI versions;
- current branch PR when `gh` is available and authenticated.

Preflight is orientation only. It is not governance, build, test, measurement, or listening evidence.

## 2. Quality governance audit

Development mode:

```sh
bash scripts/quality-gate.sh
```

Release-authorisation mode:

```sh
bash scripts/quality-gate.sh --release
```

The gate runs the Python auditor self-test, then checks:

- Product Constitution SHA-256 integrity;
- required governance artifacts;
- changed-path R0–R4 classification;
- per-path coverage by changed change records for every R2+ file;
- declared risk versus actual covered-path risk;
- required R3/R4 ADR and independent-review records;
- constitutional amendment lock/manifest updates;
- expired or unapproved active exceptions;
- release-mode product-owner approval where required.

Default report:

```text
build-local-release/governance-report.json
```

Override with:

```sh
DNA_ORBIT_GOVERNANCE_REPORT=build-governance/report.json \
bash scripts/quality-gate.sh
```

A passing result proves policy structure and integrity only. It does not prove sound quality, usability, host behavior, legal compliance, or release readiness.

## 3. Static real-time audit

```sh
bash scripts/static-realtime-audit.sh
```

Scans core DSP and processor files for likely:

- heap allocation/deallocation;
- locks, waits, and sleeps;
- filesystem, network, process, logging, or modal UI calls;
- unbounded/blocking constructs;
- portability and exception hazards;
- outstanding TODO/FIXME/HACK markers.

For exploratory warning-only use:

```sh
DNA_ORBIT_STATIC_AUDIT_WARN_ONLY=1 bash scripts/static-realtime-audit.sh
```

This is pattern-based triage. A clean result is not proof of real-time safety; the `realtime-safety-reviewer` must still inspect the actual callback call graph and bounds.

## 4. Complete local validation

```sh
bash scripts/validate-local.sh
```

The script performs:

1. agent preflight;
2. development quality-governance audit;
3. static real-time audit;
4. Release configure;
5. Release build;
6. complete CTest suite with log capture;
7. headless UI screenshot generation;
8. Debug ASan + UBSan build and CTest on macOS/Linux;
9. deterministic DSP benchmark.

Default evidence locations:

```text
build-local-release/governance-report.json
build-local-release/ctest-release.txt
build-local-release/render-shots.txt
build-local-release/screenshots/*.png
build-local-release/dsp-benchmark.txt
build-local-asan/ctest-sanitizers.txt
```

Optional environment variables:

```sh
DNA_ORBIT_JOBS=8 \
DNA_ORBIT_BENCHMARK_SECONDS=5 \
DNA_ORBIT_RELEASE_BUILD=build-release \
DNA_ORBIT_SANITIZER_BUILD=build-asan \
DNA_ORBIT_SCREENSHOT_DIR=build-release/screenshots \
DNA_ORBIT_GOVERNANCE_REPORT=build-release/governance-report.json \
bash scripts/validate-local.sh
```

The script stops on the first failure. Do not weaken governance rules, tests, sample-rate coverage, or warnings merely to finish the sequence. Establish whether the implementation, test, change contract, assumption, dependency, policy, or environment is wrong.

## Remaining manual and external gates

After local validation passes:

- obtain required Product Constitution/product-owner approvals;
- inspect active exceptions and quality debt;
- visually inspect generated screenshots at required sizes/DPI;
- run pluginval strictness 10;
- run Steinberg VST3 Validator;
- run `auval` on macOS;
- scan/load/automate/save/reopen/bypass/bounce in named DAWs;
- complete level-matched listening in `MANUAL_REQUIRED.md`;
- confirm licensing;
- sign, notarize, package, and test on clean machines;
- run `bash scripts/quality-gate.sh --release`;
- record the release decision in `docs/quality/RELEASE_DECISION_LOG.md`.

Use `/quality-governance` and `/release-gate` to report stages without conflating structural, automated, listening, host, owner, and distribution evidence.
