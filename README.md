# DNA Orbit

DNA Orbit is a DNA-shaped stereo motion effect. Two copies of the **Mid**
content orbit a shared centre axis on opposite sides, while the source's
original **Side** content can remain as a stable stereo bed around them.
Amplitude, spectral and micro-delay cues add front/back motion on top of
ordinary stereo, so the result is more than a simple auto-panner.

- **C++20 / JUCE 8.0.15 / CMake**
- Formats: **VST3**, **Audio Unit** (macOS), **Standalone**
- Product: **DNA Orbit**
- Manufacturer: **Flower Pot Studio**

> Current commercial-upgrade work is developed on a draft branch and must be
> rebuilt and validated locally before merge. See the validation section.

## 1. Product idea

Each strand's geometric position is:

```text
xA = radius * sin(thetaA)
zA = cos(thetaA)
xB = radius * sin(thetaB)
zB = cos(thetaB)
```

At full Symmetry:

```text
thetaB = thetaA + pi
```

The two moving points are antipodal, so their geometric centroid stays at the
origin. The moving strands also receive the same Mid programme material,
which avoids making one moving strand dominate merely because the source's L
and R channels contain different energy.

The source's original width is preserved separately:

```text
M = 0.5 * (L + R)
S = 0.5 * (L - R)

moving strand A source = M
moving strand B source = M
centred Core source     = M

sideBed = StereoPreserve * S
wetL += sideBed
wetR -= sideBed
```

This gives DNA Orbit two complementary layers:

1. a centred, moving Mid helix;
2. a stable Side atmosphere that retains the source's stereo identity.

`Stereo Preserve = 0%` reproduces the original Mid-only signal path.
`Stereo Preserve = 100%` restores the input Side component at unity.

## 2. HELIX and NULL CORE

| | HELIX | NULL CORE |
|---|---|---|
| Default | Active | Off |
| Purpose | Musical, centred DNA motion | Experimental Side-only Wet |
| Polarity | Normal | Complete Wet Mid is removed |
| Mono behaviour | Designed to remain useful | Wet may almost disappear |
| Dry signal | Unchanged | Unchanged |

NULL CORE is deliberately dangerous in mono. The UI shows a persistent red
warning whenever the parameter is on, including host automation and preset
loads. Its transition is smoothed over approximately 120 ms.

Core has no audible effect in fully engaged NULL CORE because Core is Mid and
NULL CORE removes Wet Mid by definition.

## 3. Interface

The editor has a Basic and a Detail page, with the live DNA visualiser always
visible.

### Basic

| Label | Parameter | Meaning |
|---|---|---|
| 速さ | Rate | Time for one revolution |
| 広がり | Radius | Left/right orbit width |
| 立体感 | Depth | Front/back cue strength |
| 効果量 | Mix | Dry/Wet blend |

### Detail

- Tempo Sync and Division
- Symmetry
- Twist
- Core
- Output
- Stereo Preserve
- Auto Gain
- Soft Bypass
- NULL CORE

### 3D visualiser

The double helix is generated from the true time-history of the same orbit
math used by the DSP. It displays:

- the two strand trajectories;
- the ideal centre reference;
- centroid drift when Symmetry is reduced;
- instantaneous top-down orbit;
- output RMS and L/R correlation;
- NULL CORE warning state.

It uses JUCE's normal graphics API rather than an OpenGL context. Rendering
quality adapts to measured frame cost, and animation stops when the editor is
not visible.

## 4. Parameters

| Parameter | ID | Range | Default | Notes |
|---|---|---:|---:|---|
| Rate | `rate` | 0.02–4 Hz | 0.12 Hz | Displayed as seconds per revolution |
| Sync | `sync` | Off/On | Off | Derives cycle speed from host BPM |
| Division | `division` | 4 bars–1/8 | 1 bar | Currently assumes 4/4 for bar choices |
| Radius | `radius` | 0–100% | 80% | Left/right spread |
| Depth | `depth` | 0–100% | 55% | Front/back gain, filter and delay cues |
| Symmetry | `symmetry` | 0–100% | 100% | 100% locks the geometric centroid |
| Twist | `twist` | 0–20 ms | 5 ms | Extra delay on strand B |
| Core | `core` | 0–100% | 0% | Centred Mid copy in Wet |
| NULL CORE | `nullCore` | Off/On | Off | Converts the complete Wet output to Side-only |
| Mix | `mix` | 0–100% | 35% | Equal-power main Dry/Wet blend |
| Output | `output` | -12–+6 dB | 0 dB | Processed output trim |
| Auto Gain | `autoGain` | Off/On | On | Geometry-based Mid-orbit compensation |
| Stereo Preserve | `stereoPreserve` | 0–100% | 70%* | Restores the original stationary Side bed |
| Soft Bypass | `softBypass` | Off/On | Off | 60 ms transition to exact Dry |

\* Projects saved before Stereo Preserve existed load at 0% to preserve their
legacy signal path. The 70% fresh-instance value is a listening candidate and
must be confirmed on real programme material before release.

## 5. Stereo Preserve

The old Mid-only Wet path made pure Side or strongly anti-phase stereo input
collapse toward silence. Stereo Preserve fixes that without feeding unrelated
L/R content into the two moving strands.

The Side bed is added **after** geometry Auto Gain. Therefore the source's
original width is not multiplied by a make-up estimate designed for the
moving Mid copies.

Properties of the design:

- mono input is unchanged at every Preserve value;
- pure Side input has equal L/R energy and zero Mid;
- anti-phase material remains audible above 0%;
- the moving DNA remains content-centred;
- 0% keeps the legacy Mid-only path;
- NULL CORE remains literally Side-only for the complete Wet output.

See:

- `docs/commercial-upgrade/decisions/ADR-004-centered-stereo-preserve.md`
- `Tests/StereoPreserveTests.cpp`
- `Tests/StereoCenterTests.cpp`

## 6. Auto Gain and Mix

Auto Gain predicts the power of the moving Mid-orbit from geometry, pan gains,
front/back attenuation, Core and delay coherence. It is deterministic rather
than an RMS follower, so it does not chase the incoming programme material.

The preserved Side bed is outside this make-up stage. NULL CORE is not given
an unbounded mono-loss compensation.

The main Mix remains an equal-power Dry/Wet law. Correlation-aware Mix
normalisation and broader real-programme measurements remain a commercial
upgrade task; do not assume the present implementation guarantees identical
perceived loudness at every Mix value and every source correlation.

## 7. Soft Bypass

Soft Bypass is the plug-in-owned control for musical A/B and automation.

- The DSP engine, delays, filters, orbit and visual state continue running.
- The audible output crossfades linearly to exact input Dry over 60 ms.
- At the fully bypassed endpoint, Output trim is bypassed too.
- Linear interpolation avoids the correlated-signal gain bump that an
  equal-power bypass law can create.

Host bypass remains a separate path: it outputs exact Dry immediately while
processing a preallocated scratch copy so internal state does not freeze.

See `docs/commercial-upgrade/decisions/ADR-005-soft-bypass.md`.

## 8. Factory presets

Factory presets live in `Source/Presets.h`, independently of the GUI. Each
preset defines the complete audio parameter state, including Sync, Division,
Output, Auto Gain, Stereo Preserve and Soft Bypass. Selecting a preset therefore
does not inherit hidden values from the previous state.

Current presets:

- ボーカルを広げる
- パッドを回す
- ギターに揺らぎ
- シンセを速く回す
- 実験:中心を消す

After a parameter is changed, the Basic page offers 元に戻す to reapply the
selected preset exactly.

## 9. Signal flow

```text
Input L/R
  |
  +-- Dry copy ---------------------------------------------------------+
  |
  +-- M/S split                                                        |
       |                                                               |
       +-- Mid -> Strand A -> depth cues -> delay -> pan --+           |
       +-- Mid -> Strand B -> depth cues -> delay -> pan --+           |
       +-- Mid -> Core -------------------------------------+           |
                                                          Mid-orbit Wet |
                                                               |       |
                                                   geometry Auto Gain  |
                                                               |       |
       +-- Side * Stereo Preserve ----------------------------+       |
                                                               |       |
                                                   optional NULL CORE  |
                                                               |       |
                                                   main Dry/Wet Mix    |
                                                               |       |
                                                   processed Output    |
                                                               |       |
                                             optional Soft Bypass -----+
                                                               |
                                                             Output
```

## 10. Build

Requirements:

- CMake 3.22 or newer
- C++20 compiler
- Xcode command-line tools on macOS
- Visual Studio 2022 Desktop C++ workload on Windows

### macOS / Linux

```sh
git clone <repository-url> DNA-Orbit
cd DNA-Orbit
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

### Windows

```powershell
git clone <repository-url> DNA-Orbit
cd DNA-Orbit
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

JUCE is fetched through CMake `FetchContent` and pinned to `8.0.15`. For an
offline build, provide a local checkout with `FETCHCONTENT_SOURCE_DIR_JUCE`.

Build output is placed under `build/DNAOrbit_artefacts/Release/`.

## 11. Test coverage

The CTest executable covers:

- orbit math and antipodal centroid invariants;
- finite output across sample rates, block sizes and extreme parameters;
- parameter smoothing and abrupt automation;
- NULL CORE mono behaviour;
- state round-trip, malformed state and schema migration;
- host-bypass Dry passthrough and internal state continuity;
- deterministic factory presets and Revert state;
- legacy Stereo Preserve 0% behaviour;
- pure-Side balance, zero Mid and Auto Gain independence;
- Soft Bypass exact-Dry endpoint, transition bound and phase continuity;
- level-match measurements;
- visual geometry and projection bounds.

### Required local validation for this draft branch

This environment could edit the repository but could not clone and compile it.
Run all of the following before making the PR ready for review:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

Then run the sanitizer build configuration already defined in `CMakeLists.txt`
and render headless screenshots:

```sh
cmake -S . -B build-shots -DCMAKE_BUILD_TYPE=Release -DDNA_ORBIT_BUILD_TOOLS=ON
cmake --build build-shots --config Release -j --target DNAOrbitRenderShots
./build-shots/DNAOrbitRenderShots <output-directory>
```

Also run pluginval, VST3 Validator and `auval` where available.

## 12. Known limitations before commercial release

- No PPQ-position Host Phase Lock yet; Sync currently controls rate only.
- Bar divisions currently assume 4/4.
- Bass Anchor crossover is not implemented yet.
- Correlation-aware main Mix normalisation is not implemented yet.
- The front/back model is perceptual, not HRTF or physical binaural rendering.
- Factory Stereo Preserve values require real listening and retuning.
- macOS AU, Windows VST3, pluginval and real-DAW validation remain mandatory.
- The editor is Japanese-only in the current implementation.
- Factory presets are not exposed as DAW program slots.

## 13. Licensing and distribution

See `LICENSE_NOTES.md`. JUCE licensing, the DNA Orbit source licence,
code-signing, notarisation and installer behaviour must be confirmed before
distribution. The current build leaves JUCE's splash-screen behaviour at its
default.

## 14. Manual checks before shipping

- [ ] Build Release on macOS and Windows.
- [ ] Run CTest and sanitizers with zero project errors.
- [ ] Run pluginval strictness 10, VST3 Validator and `auval`.
- [ ] Confirm VST3/AU scanning, loading, resizing and project state restore.
- [ ] Test mono-in/stereo-out and stereo-in/stereo-out.
- [ ] Listen to Stereo Preserve at 0/25/50/70/100% on mono vocal, L-only,
      R-only, wide pad, uncorrelated stereo, anti-phase material and a full mix.
- [ ] Confirm the moving helix remains audible without the stationary Side bed
      masking it.
- [ ] Automate Soft Bypass on vocals, pads and transients; confirm no click,
      gain flare or stale orbit on return.
- [ ] Sweep Mix 0–100% with Auto Gain on/off and note any correlated gain bump.
- [ ] Sum HELIX to mono and confirm useful signal remains.
- [ ] Sum NULL CORE to mono and confirm the Wet collapse is intentional and
      clearly warned.
- [ ] Change tempo and divisions while playing.
- [ ] Save, close and reopen projects; verify every parameter exactly.
- [ ] Compare editor-open and editor-closed CPU with multiple instances.
- [ ] Listen on headphones, speakers and a small mono playback device.
