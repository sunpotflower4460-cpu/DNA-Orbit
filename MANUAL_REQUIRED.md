# Manual verification required

This file lists checks still required before the commercial-upgrade branch can
be merged or shipped.

The current ChatGPT environment edited GitHub through the connector but could
not download and compile the complete branch. Therefore the newest centred
Stereo Preserve, Soft Bypass, correlation-aware Mix, Bass Anchor, Character,
Host Phase Lock, and user-centred editor changes are **not marked validated**
here. Source review and screenshot definitions do not prove compiled layout,
usability, accessibility, audio quality, or host behaviour.

## 1. Mandatory local build

The preferred complete command is:

```sh
git fetch origin
git checkout agent/world-class-dsp-phase3
bash scripts/validate-local.sh
```

It runs preflight, static real-time audit, Release configure/build, CTest,
screenshot generation, ASan/UBSan where supported, and the DSP benchmark.

Manual alternatives:

### macOS / Linux

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DDNA_ORBIT_BUILD_TESTS=ON \
  -DDNA_ORBIT_BUILD_TOOLS=ON \
  -DDNA_ORBIT_BUILD_BENCHMARKS=ON
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

### Windows

```powershell
git fetch origin
git checkout agent/world-class-dsp-phase3
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DDNA_ORBIT_BUILD_TESTS=ON `
  -DDNA_ORBIT_BUILD_TOOLS=ON `
  -DDNA_ORBIT_BUILD_BENCHMARKS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Keep the PR in draft if compilation or any test fails. Do not weaken test
thresholds merely to make the branch green; determine whether the code or test
assumption is wrong.

## 2. Sanitizers

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDNA_ORBIT_SANITIZERS="address,undefined"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Expected: no project memory error, undefined behaviour, or project leak.

## 3. Validators

- pluginval strictness 10
- Steinberg VST3 Validator
- macOS AU validation:

```sh
auval -v aufx DnaO FpSt
```

## 4. Bass Anchor measurement and listening

Measure Bass Anchor at Off/80/120/180/250 Hz using mono and stereo material.

Automated expectations to confirm after build:

- fourth-order Linkwitz–Riley low + high recombination remains within test
  tolerance at 60/120/1000/5000 Hz;
- `20 Hz / Off` is sample-transparent (`low = 0`, `high = input`);
- anti-phase 60 Hz energy is strongly removed from the high-band Side bed at a
  120 Hz anchor;
- 1 kHz Side remains substantially unchanged.

Audition kick, bass, drum loop, stereo piano, wide pad, full mix, mono vocal,
and acoustic guitar. Confirm low-end solidity, crossover transparency,
click-free automation, default suitability, and understood NULL CORE behaviour.

## 5. Character tuning

Compare Natural, Vivid, and Deep at equal loudness on vocal, guitar, synth,
pad, drums, and full mix.

Current engineering values:

- Natural: 4 dB rear attenuation, 5 kHz rear cutoff, 8 ms rear delay;
- Vivid: 6 dB, 8 kHz, 10 ms;
- Deep: 8 dB, 3.5 kHz, 14 ms.

Confirm distinct musical purpose, no loudness bias, no unacceptable harshness,
dullness, pitch wobble, click, or preset mismatch. Retune only from recorded
level-matched evidence.

## 6. Host Phase Lock and transport

Test Free, Retrigger, and Host Lock in Cubase or LUNA first, then other
available DAWs.

Time signatures: 3/4, 4/4, 5/4, 6/8, and 7/8.

Test multiple start positions, stop/restart, loops, forward/backward seeks,
tempo/time-signature changes, Division, Start Phase, Direction, repeated
offline renders, and realtime/offline comparison.

Confirm deterministic PPQ geometry, correct bar length, correct Retrigger and
Free behaviour, no hard seek click, acceptable 30 ms correction, safe fallback
when host data is absent, and truthful visual history.

## 7. Centred Stereo Preserve listening

Listen at 0/25/50/70/100% using mono vocal, L-only, R-only, unequal L/R,
wide pad, uncorrelated texture, anti-phase stress, guitar, piano, and full mix.

Confirm audible moving helix, preserved width without masking motion,
anti-phase survival above 0%, unchanged mono behaviour, and evidence-based
factory/preset values.

## 8. Soft Bypass listening

Automate and toggle Soft Bypass on vocal, sustained pad, drums, bass-heavy
material, and full mix.

Confirm no click or gain flare, exact Dry endpoint, no Output-trim leak at the
endpoint, continuing orbit state, and non-stale return.

## 9. Correlation-aware Mix and Auto Gain

Sweep Mix 0→100→0 with Auto Gain on/off using correlated, transient, wide,
anti-phase, NULL CORE, and full-mix material.

Record RMS/loudness, peak, 25/50/75% levels, transient behaviour, post-silence
response, and mono fold-down. Confirm the 250 ms correction is stable and does
not sound like a loudness rider.

## 10. Real DAW verification

Priority DAWs:

- LUNA
- Cubase
- Logic Pro
- Ableton Live
- REAPER
- Studio One
- FL Studio

For each available host:

1. scan and load;
2. resize editor repeatedly;
3. use every factory preset;
4. automate every parameter;
5. save, close, and reopen;
6. change tempo, time signature, and loop;
7. compare host and Soft Bypass;
8. bounce offline;
9. test mono-in/stereo-out and stereo-in/stereo-out;
10. open multiple instances and check CPU;
11. remove and reinsert;
12. confirm no state loss, focus trap, shortcut conflict, or crash.

## 11. UI screenshot matrix

Build and run the renderer directly or through `scripts/validate-local.sh`.

```sh
cmake -S . -B build-shots \
  -DCMAKE_BUILD_TYPE=Release \
  -DDNA_ORBIT_BUILD_TOOLS=ON
cmake --build build-shots --config Release -j --target DNAOrbitRenderShots
./build-shots/DNAOrbitRenderShots <output-directory>
```

The current renderer must generate:

- `ui_basic_min_free_820x650.png`
- `ui_basic_min_sync_820x650.png`
- `ui_detail_min_820x650.png`
- `ui_basic_standard_960x700.png`
- `ui_detail_standard_960x700.png`
- `ui_detail_wide_1440x900.png`
- `ui_detail_nullcore_960x700.png`
- `ui_basic_softbypass_960x700.png`

Inspect every image at 100% scale and as a small DAW-like preview.

Blockers:

- clipped or overlapping Japanese/English text;
- value field hidden by label or card edge;
- control too small to manipulate reliably;
- header preset or Bypass inaccessible;
- Sync disables Speed without visible Sync/Division explanation;
- warning obscures a required control;
- visualiser covers status/warning badges;
- Detail group has zero-width or visually detached controls;
- minimum-size visualiser loses all explanatory value;
- active, disabled, modified, warning, and bypass states cannot be distinguished.

Screenshots validate layout only. They do not prove keyboard access, host
scaling, usability, or visualiser performance.

## 12. UI interaction and accessibility

### Primary first-use journey

Give the plug-in to a target musician who has not seen the implementation.
Without coaching, ask them to:

1. identify what the plug-in does;
2. choose a suitable preset;
3. change Width, Depth, Mix, and Speed;
4. enable tempo Sync and choose Division;
5. compare processed and Dry with Bypass;
6. modify a preset and return to its original state;
7. find advanced motion, space, and output controls.

Record hesitation, wrong clicks, misunderstood labels, hidden dependencies,
and whether the result can be reached without opening Detail unnecessarily.
Do not explain the interface until the observation is complete.

### Keyboard and focus

Manually verify:

- every interactive control can receive focus;
- focus does not enter hidden Basic/Detail controls;
- tabs, preset, Bypass, knobs, toggles, and choices can be activated;
- focus remains visible enough to locate;
- DAW transport and shortcut keys are not captured unexpectedly;
- popup menus and tooltips do not trap focus;
- double-click reset returns each continuous control to the actual parameter default.

Keyboard-focus eligibility in source code is not proof that traversal order is
usable. Record the observed order and any host-specific difference.

### States and wording

Confirm:

- Soft Bypass is reachable from both pages;
- preset modified badge appears only after a selected preset changes;
- Revert restores complete preset state;
- Sync explains why Speed is disabled;
- Phase Mode and Start Position dependencies are understandable;
- NULL CORE disables Core and displays a text warning;
- detailed telemetry appears only on Detail;
- `Off` represents the literal Bass Anchor bypass;
- tooltips explain consequences rather than repeat labels.

### DPI, scaling, and resizing

Test minimum, standard, wide, and maximum sizes at available 100%, 125%, 150%,
175%, and 200% display scaling. Repeatedly drag through the 1120 px responsive
breakpoint.

Confirm no flicker, stale card bounds, cropped popup, incorrect font fallback,
unusable resize handle, tooltip outside the visible area, or sustained message-
thread spike.

### Colour and non-colour cues

Verify active, disabled, modified, drift, NULL CORE, and Bypass states in normal
colour, reduced-saturation display, and a grayscale screenshot. No critical
state may depend on colour alone.

## 13. Visualiser truth and performance

Confirm:

- geometry follows published DSP telemetry;
- centre state and warning overlays remain above the animation;
- Detail diagnostics agree with the DSP state;
- backward seeks or resets do not imply false continuous history;
- hidden editors stop or reduce animation;
- adaptive quality does not visibly pulse between tiers;
- large-window rendering does not compromise audio-thread performance;
- Basic remains visually calm enough for musical decisions.

Measure editor closed/open at 44.1/48/96/192 kHz, block sizes 32/64/128/512,
and 1/5/10/20 instances. Record average/peak CPU, callback time, editor frame
time, memory, load time, and automation/Host Lock spikes.

## 14. Signing and distribution

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

## 15. CI decision

Per the user's decision, no GitHub Actions workflow has been added. Checks are
currently local. The sanitizer CMake option can be connected to CI later
without changing DSP code.

## 16. Remaining product work

- compile/regression fixes from the first complete local build;
- render and inspect the complete eight-state UI matrix;
- correct issues found by keyboard, DPI, DAW, and first-use observation;
- measured CPU profiling and safe interpolation-based optimisation;
- final Character, Stereo Preserve, Bass Anchor, and preset tuning;
- transport-jump visual-history verification/improvement;
- English localisation beyond intentional technical labels;
- installer/signing workflow;
- complete real-DAW release matrix.

These are explicit release-planning items, not hidden completed work.
