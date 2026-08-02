# Manual verification required

This file lists checks still required before the commercial-upgrade branch can
be merged or shipped.

The current ChatGPT environment edited GitHub through the connector but could
not download and compile the complete branch. Therefore the newest centred
Stereo Preserve, Soft Bypass, correlation-aware Mix, Bass Anchor, Character
and Host Phase Lock changes are **not marked validated** here. Older Linux test
results do not validate the new branch.

## 1. Mandatory local build

### macOS / Linux

```sh
git fetch origin
git checkout agent/world-class-dsp-phase3
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

### Windows

```powershell
git fetch origin
git checkout agent/world-class-dsp-phase3
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Keep the PR in draft if compilation or any test fails. Do not weaken test
thresholds merely to make the branch green; determine whether the code or the
test assumption is wrong.

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

## 4. Bass Anchor measurement and listening

Measure Bass Anchor at Off/80/120/180/250 Hz using mono and stereo material.

Automated expectations to confirm after build:

- fourth-order Linkwitz–Riley low + high recombination remains within the test
  tolerance at 60/120/1000/5000 Hz;
- `20 Hz / Off` is bit-transparent (`low = 0`, `high = input`);
- anti-phase 60 Hz energy is strongly removed from the high-band Side bed at a
  120 Hz anchor;
- 1 kHz Side remains substantially unchanged.

Audition:

- kick and bass;
- bass guitar with stereo upper harmonics;
- drum loop;
- stereo piano;
- wide pad;
- full mix;
- mono vocal and acoustic guitar.

Confirm:

- low end remains centred and solid;
- crossover motion does not create a hollow band or transient smear;
- automation does not click;
- 120 Hz is a suitable fresh default;
- NULL CORE behaviour is clearly understood: because it removes complete Wet
  Mid, the mono Bass Anchor portion is intentionally removed in that mode.

## 5. Character tuning

Compare Natural, Vivid and Deep at equal loudness on vocal, guitar, synth,
pad, drums and full mix.

Current engineering values:

- Natural: 4 dB rear attenuation, 5 kHz rear cutoff, 8 ms rear delay;
- Vivid: 6 dB, 8 kHz, 10 ms;
- Deep: 8 dB, 3.5 kHz, 14 ms.

Confirm:

- the three modes are meaningfully distinct;
- Vivid does not become harsh or merely louder;
- Deep sounds farther away rather than only duller;
- fast Motion does not create unacceptable pitch wobble;
- Character automation remains click-free;
- preset Character choices fit their named use.

Retune only from level-matched listening evidence.

## 6. Host Phase Lock and transport

Test Free, Retrigger and Host Lock in Cubase first, then other available DAWs.

Time signatures:

- 3/4
- 4/4
- 5/4
- 6/8
- 7/8

Actions:

1. start from multiple song positions;
2. stop and restart;
3. loop one or more bars;
4. jump forward and backward;
5. change tempo while playing;
6. change time signature;
7. change Division;
8. change Start Phase and Direction;
9. render the same range twice offline;
10. compare realtime and offline renders.

Confirm:

- the same PPQ position produces the same orbit position in Host Lock;
- bar divisions follow the host time signature;
- Retrigger resets only at stopped→playing;
- Free remains continuous;
- loops and seeks do not produce a hard click;
- the 30 ms correction is not perceptibly late or smeared;
- when BPM/PPQ is unavailable, Host Lock falls back safely to Free;
- the visual history does not draw a misleading long bridge across a transport
  jump. This visual reset still needs explicit host verification.

## 7. Centred Stereo Preserve listening

Listen at 0/25/50/70/100% using:

- mono male and female vocal;
- L-only and R-only material;
- input with approximately 6 dB L/R imbalance;
- wide pad;
- uncorrelated stereo texture;
- anti-phase stress signal;
- guitar, piano and full mix.

Confirm the moving helix remains audible, the Side bed does not mask motion,
anti-phase material remains audible above 0%, mono input is unchanged and the
best factory value is recorded. Retune presets if 70% is not suitable.

## 8. Soft Bypass listening

Automate and toggle Soft Bypass on vocal, sustained pads, drums, bass-heavy
material and full mixes.

Confirm no click or temporary gain flare, fully bypassed output is exact Dry,
Output trim does not affect the endpoint, orbit state continues and returning
does not resume a stale position.

## 9. Correlation-aware Mix and Auto Gain

Sweep Mix 0→100→0 with Auto Gain on/off using correlated, transient, wide,
anti-phase, NULL CORE and full-mix material.

Record input/output RMS or loudness, peak level, levels at 25/50/75%, transient
behaviour, delayed gain changes after silence and mono fold-down. Confirm the
250 ms correction sounds stable rather than like a loudness rider.

## 10. Real DAW verification

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
4. automate every parameter;
5. save, close and reopen;
6. change tempo, time signature and loop;
7. compare host and Soft Bypass;
8. bounce offline;
9. test mono-in/stereo-out and stereo-in/stereo-out;
10. open multiple instances and check CPU;
11. remove and reinsert;
12. confirm no state loss or crash.

## 11. Headless screenshots

```sh
cmake -S . -B build-shots \
  -DCMAKE_BUILD_TYPE=Release \
  -DDNA_ORBIT_BUILD_TOOLS=ON
cmake --build build-shots --config Release -j --target DNAOrbitRenderShots
./build-shots/DNAOrbitRenderShots <output-directory>
```

Inspect Basic, Detail, NULL CORE, Host Lock, Bass Anchor Off/On, each Character
mode, drift and modified-preset states. Confirm the expanded Detail panel fits
at minimum and maximum editor sizes and at target DPI scaling.

## 12. Performance

Measure editor closed/open at 44.1/48/96/192 kHz, block sizes 32/64/128/512,
and 1/5/10/20 instances.

This branch reuses Strand A trigonometry for antipodal Strand B and avoids
crossover coefficient rebuilds when cutoff is unchanged. More aggressive
control-rate processing is deliberately not claimed complete: it must be
implemented only with interpolation and audio-regression evidence.

Record average/peak CPU, callback time, editor frame time, memory, load time and
whether automation or Host Lock creates spikes.

## 13. Signing and distribution

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

## 14. CI decision

Per the user's decision, no GitHub Actions workflow has been added. Checks must
currently be run locally. The sanitizer CMake option can be connected to CI
later without changing DSP code.

## 15. Remaining product work

- compile/regression fixes from the first complete local build;
- measured CPU profiling and safe interpolation-based optimisation;
- final Character, Stereo Preserve, Bass Anchor and preset tuning;
- transport-jump visual-history reset verification/improvement;
- English localisation and accessibility;
- installer/signing workflow;
- full real-DAW release matrix.

These are explicit release-planning items, not hidden completed work.
