# Manual verification required

This file lists checks that are still required before the commercial-upgrade
branch can be merged or shipped.

The current ChatGPT environment could edit the GitHub repository through the
GitHub connector, but it could not clone the repository or execute CMake,
JUCE, CTest, plugin validators or a DAW. Therefore the newest centred Stereo
Preserve and Soft Bypass changes have **not been compiled or executed in this
environment**. Older commits contain reported Linux test results, but those do
not validate the new branch changes.

Do not mark any item below complete without recording the machine, OS, command
and result.

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

The draft PR must remain draft if either build fails or any CTest assertion
fails.

## 2. Sanitizers

On GCC or Clang:

```sh
cmake -S . -B build-asan \
  -DCMAKE_BUILD_TYPE=Debug \
  -DDNA_ORBIT_SANITIZERS="address,undefined"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Expected result: no project memory error, undefined behaviour or leak.

## 3. Validators

- pluginval strictness 10 against the built VST3 and AU where applicable.
- Steinberg VST3 Validator on macOS and Windows.
- macOS AU validation:

```sh
auval -v aufx DnaO FpSt
```

## 4. Centred Stereo Preserve listening

The current design is documented in
`docs/commercial-upgrade/decisions/ADR-004-centered-stereo-preserve.md`.

The moving DNA strands and Core receive Mid. Stereo Preserve restores a
stationary Side bed after geometry Auto Gain. The 70% fresh-instance default
is a candidate, not a final perceptual truth.

Listen at 0/25/50/70/100% using:

- mono vocal;
- female and male lead vocal;
- L-only and R-only material;
- stereo input with approximately 6 dB L/R imbalance;
- wide pad;
- uncorrelated stereo noise or texture;
- anti-phase stress signal;
- acoustic guitar;
- piano;
- full mix.

Confirm:

- the moving helix remains perceptible;
- the stable Side bed does not mask the DNA motion;
- L/R-unbalanced material does not make the moving orbit lean unnaturally;
- anti-phase material remains audible above 0%;
- mono input does not change when Preserve moves;
- mono fold-down remains musically useful in HELIX mode;
- the most musical factory value is recorded and presets are retuned if 70%
  is not universally appropriate.

## 5. Soft Bypass listening

Soft Bypass is now implemented and documented in
`docs/commercial-upgrade/decisions/ADR-005-soft-bypass.md`.

Automate or toggle it on:

- vocal;
- sustained pad;
- transient drum loop;
- bass-heavy material;
- full mix.

Confirm:

- no click;
- no temporary gain flare;
- endpoint is exact Dry;
- Output trim does not affect fully bypassed audio;
- the orbit continues while bypassed;
- returning from bypass resumes the current orbit, not a stale one.

Compare Soft Bypass with the DAW's host bypass. Host bypass may be immediate;
Soft Bypass is the preferred musical A/B control.

## 6. Main Mix and Auto Gain

The main Dry/Wet law is still equal-power. Correlated Dry and Wet may produce
a mid-Mix gain bump. Test Mix 0→100→0 with Auto Gain on and off using mono,
stereo, transient and anti-phase material.

Record:

- integrated or long-window RMS/loudness;
- peak level;
- audible pumping;
- level at 25/50/75%;
- mono fold-down.

Correlation-aware Mix normalisation remains a planned commercial-quality
improvement and must be completed or explicitly accepted before shipping.

## 7. Real DAW verification

Required DAWs where available:

- Cubase;
- Logic Pro;
- Ableton Live;
- REAPER;
- Studio One;
- FL Studio.

For each:

1. scan and load;
2. open and resize editor;
3. use every factory preset;
4. automate every parameter, including Stereo Preserve and Soft Bypass;
5. save, close and reopen project;
6. change tempo and loop;
7. test host bypass and Soft Bypass;
8. render or bounce offline;
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
new Soft Bypass row fits at minimum and maximum editor sizes.

## 9. Performance

Measure with editor closed and open at 44.1/48/96/192 kHz, several block
sizes and 1/5/10 instances.

Pay particular attention to the existing per-sample trigonometry and
exponential coefficient calculations. Control-rate optimisation and Bass
Anchor are not yet implemented on this branch.

## 10. Signing and distribution

### macOS

- Developer ID signing;
- Hardened Runtime;
- notarisation with `notarytool`;
- staple;
- `codesign --verify --deep --strict`;
- `spctl --assess`.

### Windows

- code-signing certificate;
- timestamped installer signing;
- SmartScreen check;
- standard VST3 install path;
- uninstall and upgrade-install test.

## 11. CI decision

Per the user's explicit decision, no GitHub Actions workflow has been added.
All checks must currently be run locally. The existing CMake sanitizer option
can be connected to CI later without changing the DSP.

## 12. Remaining product work

Not implemented yet:

- Bass Anchor crossover;
- PPQ-position Host Phase Lock and non-4/4 bar handling;
- correlation-aware main Mix normalisation;
- Character modes;
- English localisation and accessibility pass;
- full installer/signing workflow.

These are not hidden limitations; they are release-planning items.
