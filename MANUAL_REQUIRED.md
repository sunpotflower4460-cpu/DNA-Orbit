# Manual verification required

This file lists checks still required before the commercial-upgrade branch can
be merged or shipped.

The current ChatGPT environment could edit GitHub through the connector but
could not clone or compile the repository. Therefore the newest centred Stereo
Preserve, Soft Bypass and correlation-aware Mix changes have **not been
executed in this environment**. Older commits contain reported Linux results;
those results do not validate the new branch.

## 1. Mandatory local build

### macOS / Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

### Windows

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Keep the PR in draft if compilation or any test fails.

## 2. Sanitizers

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDNA_ORBIT_SANITIZERS="address,undefined"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Expected: no project memory error, undefined behaviour or leak.

## 3. Validators

- pluginval strictness 10
- Steinberg VST3 Validator
- macOS AU validation:

```sh
auval -v aufx DnaO FpSt
```

## 4. Centred Stereo Preserve listening

See `ADR-004-centered-stereo-preserve.md`.

Listen at 0/25/50/70/100% using:

- mono male and female vocal;
- L-only and R-only material;
- input with approximately 6 dB L/R imbalance;
- wide pad;
- uncorrelated stereo texture;
- anti-phase stress signal;
- guitar, piano and full mix.

Confirm:

- the moving helix remains audible;
- the stable Side bed does not mask motion;
- unbalanced stereo does not make the moving orbit lean unnaturally;
- anti-phase material remains audible above 0%;
- mono input is unchanged;
- HELIX mono fold-down remains musical;
- the best factory value is recorded. Retune presets if 70% is not suitable.

## 5. Soft Bypass listening

See `ADR-005-soft-bypass.md`.

Automate and toggle Soft Bypass on vocal, sustained pads, drums, bass-heavy
material and full mixes.

Confirm:

- no click or temporary gain flare;
- fully bypassed output is exact Dry;
- Output trim does not affect the endpoint;
- orbit state continues;
- returning does not resume a stale position.

Compare with host bypass. Host bypass may be immediate; Soft Bypass is the
preferred musical A/B control.

## 6. Correlation-aware Mix and Auto Gain

See `ADR-006-correlation-aware-mix.md`.

The implementation uses the previous block's Dry/Wet correlation, smoothed
over 250 ms, with correction bounded to approximately ±3 dB.

Sweep Mix 0→100→0 with Auto Gain on and off using:

- identical/near-identical Dry and Wet conditions;
- mono vocal;
- sustained pad;
- transient drums;
- wide stereo;
- anti-phase material;
- NULL CORE;
- full mix.

Record:

- input and output RMS or loudness;
- peak level;
- level at 25/50/75%;
- transient behaviour;
- any delayed gain change after silence or source changes;
- mono fold-down.

Confirm that the correction sounds stable rather than like a loudness rider.
If the 250 ms time constant is audible, change it only with measurement and
listening evidence.

## 7. Real DAW verification

Priority DAWs:

- Cubase
- Logic Pro
- Ableton Live
- REAPER
- Studio One
- FL Studio

For each:

1. scan and load;
2. resize editor;
3. use every factory preset;
4. automate all parameters, including Preserve and Soft Bypass;
5. save, close and reopen;
6. change tempo and loop;
7. compare host and Soft Bypass;
8. bounce offline;
9. test mono-in/stereo-out and stereo-in/stereo-out;
10. open multiple instances and check CPU;
11. remove and reinsert;
12. confirm no state loss or crash.

## 8. Headless screenshots

```sh
cmake -S . -B build-shots \
  -DCMAKE_BUILD_TYPE=Release \
  -DDNA_ORBIT_BUILD_TOOLS=ON
cmake --build build-shots --config Release -j --target DNAOrbitRenderShots
./build-shots/DNAOrbitRenderShots <output-directory>
```

Inspect Basic, Detail, NULL CORE, drift and modified-preset states. Confirm the
Soft Bypass row fits at minimum and maximum editor sizes.

## 9. Performance

Measure editor closed/open at 44.1/48/96/192 kHz, several block sizes and
1/5/10 instances.

The correlation estimate adds only block-rate work, but the existing DSP still
performs substantial per-sample trigonometry and exponential coefficient
calculation. Control-rate optimisation remains required.

## 10. Signing and distribution

### macOS

- Developer ID signing
- Hardened Runtime
- notarisation and staple
- `codesign --verify --deep --strict`
- `spctl --assess`

### Windows

- timestamped code and installer signing
- SmartScreen check
- standard VST3 path
- uninstall and upgrade-install tests

## 11. CI decision

Per the user's decision, no GitHub Actions workflow has been added. Checks must
currently be run locally. The sanitizer CMake option can be connected to CI
later without changing DSP code.

## 12. Remaining product work

- Bass Anchor crossover
- PPQ-position Host Phase Lock and non-4/4 bar handling
- control-rate CPU optimisation
- Character modes and final preset tuning
- English localisation and accessibility
- installer/signing workflow

These are explicit release-planning items, not hidden completed work.
