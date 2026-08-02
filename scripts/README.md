# Local agent and validation scripts

GitHub Actions is intentionally not used for this project. These scripts provide deterministic local evidence and are called explicitly by Claude Code skills.

The GitHub contents API may not preserve executable bits, so invoke scripts through `bash`.

## 1. Agent preflight

```sh
bash scripts/agent-preflight.sh
```

Reports:

- repository root, branch, commit, and warning when editing `main`;
- working tree and staged/unstaged diff summaries;
- recent commits;
- presence of required Claude Code foundation files;
- local Git, CMake, compiler, Claude Code, and GitHub CLI versions;
- current branch PR when `gh` is available and authenticated.

Preflight is orientation only. It is not build, test, measurement, or listening evidence.

## 2. Static real-time audit

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

## 3. Complete local validation

```sh
bash scripts/validate-local.sh
```

The script performs:

1. agent preflight;
2. static real-time audit;
3. Release configure;
4. Release build;
5. complete CTest suite with log capture;
6. headless UI screenshot generation;
7. Debug ASan + UBSan build and CTest on macOS/Linux;
8. deterministic DSP benchmark.

Default evidence locations:

```text
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
bash scripts/validate-local.sh
```

The script stops on the first failure. Do not weaken tests, skip a sample rate, or hide a build warning merely to finish the sequence. Establish whether the implementation, test, assumption, dependency, or environment is wrong.

## Remaining manual and external gates

After local validation passes:

- visually inspect generated screenshots at required sizes/DPI;
- run pluginval strictness 10;
- run Steinberg VST3 Validator;
- run `auval` on macOS;
- scan/load/automate/save/reopen/bypass/bounce in named DAWs;
- complete level-matched listening in `MANUAL_REQUIRED.md`;
- confirm licensing;
- sign, notarize, package, and test on clean machines.

Use `/release-gate` to report these stages without conflating automated, listening, host, and distribution evidence.
