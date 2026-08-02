# DNA Orbit

DNA Orbit is a DNA-shaped stereo motion effect. Two copies of the **Mid**
content orbit a shared centre axis on opposite sides, while the source's
original **Side** content can remain as a stable stereo bed around them.
Amplitude, spectral and micro-delay cues create front/back movement in
ordinary stereo without requiring a binaural or Atmos renderer.

- C++20 / JUCE 8.0.15 / CMake
- VST3 / Audio Unit on macOS / Standalone
- Product: DNA Orbit
- Manufacturer: Flower Pot Studio

> This commercial-upgrade branch is a draft. Its newest changes must be built,
> tested and listened to locally before merge.

## Signal concept

```text
M = 0.5 * (L + R)
S = 0.5 * (L - R)

moving strand A = processed M
moving strand B = processed M
Core             = centred M
Side bed          = StereoPreserve * S
```

At full Symmetry, strand B stays `pi` radians opposite strand A. The two
moving strands therefore share both an antipodal geometry and the same
programme content. Stereo width is retained separately as a balanced Side
layer instead of feeding unrelated L/R energy into the two moving strands.

This distinction matters: a geometric midpoint of zero alone does not ensure
a perceptually centred orbit when the two moving objects contain different
energy. The Mid-orbit + Side-bed design keeps the moving DNA centred while
preserving the source's stereo atmosphere.

## HELIX and NULL CORE

HELIX is the normal musical mode. Both strands retain normal polarity and the
Wet signal is designed to remain useful in mono.

NULL CORE removes the Mid component of the **complete Wet output**, including
the preserved Side bed. It is intentionally Side-only and can nearly disappear
when summed to mono. The UI displays a persistent warning and the transition is
smoothed over approximately 120 ms.

## Interface

### Basic

- 速さ — revolution time
- 広がり — left/right orbit width
- 立体感 — front/back cue strength
- 効果量 — Dry/Wet amount
- action-named factory presets

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

The 3D double helix is generated from the real orbit history used by the DSP.
It displays the strand paths, ideal centre reference, centroid drift, output
RMS and L/R correlation. Rendering quality adapts to measured frame cost and
animation stops when the editor is not visible.

## Parameters

| Parameter | ID | Range | Default |
|---|---|---:|---:|
| Rate | `rate` | 0.02–4 Hz | 0.12 Hz |
| Sync | `sync` | Off/On | Off |
| Division | `division` | 4 bars–1/8 | 1 bar |
| Radius | `radius` | 0–100% | 80% |
| Depth | `depth` | 0–100% | 55% |
| Symmetry | `symmetry` | 0–100% | 100% |
| Twist | `twist` | 0–20 ms | 5 ms |
| Core | `core` | 0–100% | 0% |
| NULL CORE | `nullCore` | Off/On | Off |
| Mix | `mix` | 0–100% | 35% |
| Output | `output` | -12–+6 dB | 0 dB |
| Auto Gain | `autoGain` | Off/On | On |
| Stereo Preserve | `stereoPreserve` | 0–100% | 70%* |
| Soft Bypass | `softBypass` | Off/On | Off |

\* Legacy projects saved before Stereo Preserve existed load at 0%. The 70%
fresh-instance value is a listening candidate and must be confirmed before
release.

## Stereo Preserve

`Stereo Preserve = 0%` is the legacy Mid-only Wet path.

Above 0%, the original Side component is restored after geometry Auto Gain:

```text
sideBed = p * S
wetL += sideBed
wetR -= sideBed
```

Consequences:

- mono input is unchanged at every value;
- pure Side material has equal L/R energy and zero Mid;
- anti-phase material remains audible above 0%;
- original width is not multiplied by the Mid-orbit make-up gain;
- the moving DNA remains content-centred;
- NULL CORE remains literally Side-only.

See `ADR-004-centered-stereo-preserve.md` and the Stereo Preserve/Center tests.

## Auto Gain and correlation-aware Mix

Auto Gain has two bounded stages.

### 1. Geometry Wet compensation

A deterministic estimate compensates the moving Mid strands and Core using
pan gains, front/back attenuation, delay coherence and geometry. It is not an
RMS follower and does not chase individual transients. The stationary Side bed
is added after this stage.

### 2. Dry/Wet correlation normalization

Equal-power mixing can create a +3.01 dB bump at 50% when Dry and Wet are
identical. DNA Orbit estimates stereo Dry/Wet correlation once per block,
smooths it over 250 ms, and predicts the main-mix power:

```text
P = gD^2 + gW^2 + 2*rho*gD*gW
normalizer = 1 / sqrt(P)
```

The predicted power is clamped so correction cannot exceed approximately
±3.01 dB. Mix 0% and 100% are unchanged. Auto Gain OFF disables both the
geometry makeup and correlation normalization, exposing the raw path.

See `ADR-006-correlation-aware-mix.md` and `CorrelationMixTests.cpp`.

## Soft Bypass

Soft Bypass is the plug-in-owned musical A/B control.

- the engine, delays, filters and orbit continue running;
- processed output crossfades linearly to exact input Dry over 60 ms;
- fully bypassed audio ignores Output trim;
- returning resumes the current orbit instead of a stale position.

Linear interpolation is intentional because an equal-power bypass law can
create another gain bump when Dry and processed audio are correlated.

Host bypass is separate: it returns immediate exact Dry while advancing the
engine on a preallocated scratch copy.

## Factory presets

Factory presets are defined independently of the GUI in `Source/Presets.h`.
Every preset sets the complete audio state, including Sync, Division, Output,
Auto Gain, Stereo Preserve and Soft Bypass. Applying a preset therefore never
inherits hidden values from the previous state.

Current presets:

- ボーカルを広げる
- パッドを回す
- ギターに揺らぎ
- シンセを速く回す
- 実験:中心を消す

The Basic page shows 元に戻す after the selected preset is modified.

## Simplified signal flow

```text
Input L/R
  +-- Dry ----------------------------------------------------------+
  +-- M/S split                                                    |
       +-- M -> Strand A -> depth/filter/delay/pan --+             |
       +-- M -> Strand B -> depth/filter/delay/pan --+             |
       +-- M -> Core --------------------------------+             |
                                      geometry Auto Gain           |
       +-- S * Stereo Preserve -----------------------+             |
                                      optional NULL CORE           |
                                      Dry/Wet Mix                   |
                                      correlation normalization    |
                                      Output trim                  |
                                      optional Soft Bypass --------+
                                                                  Output
```

## Build

Requirements: CMake 3.22+, a C++20 compiler, Xcode command-line tools on
macOS, or Visual Studio 2022 Desktop C++ on Windows.

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

JUCE is pinned to 8.0.15 and fetched through CMake `FetchContent`. Build
artifacts are placed under `build/DNAOrbit_artefacts/Release/`.

## Test coverage

The CTest executable contains tests for:

- orbit math and antipodal centroid invariants;
- finite output across sample rates, block sizes and parameter extremes;
- parameter smoothing and abrupt automation;
- NULL CORE mono behaviour;
- malformed state, round-trip and schema migration;
- exact host-bypass passthrough and state continuity;
- deterministic factory presets;
- legacy Stereo Preserve behaviour;
- pure-Side balance, zero Mid and Auto Gain independence;
- Soft Bypass exact-Dry endpoint, transition and phase continuity;
- correlation-aware 50% Mix level matching and endpoint invariance;
- visual history, projection and resize bounds.

### Mandatory validation for this draft

The current ChatGPT environment edited GitHub but could not compile this
branch. Before changing the PR from draft:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

Then run the sanitizer configuration in `CMakeLists.txt`, render the headless
screenshots, and run pluginval, VST3 Validator and `auval` where available.

## Remaining work before commercial release

- Bass Anchor crossover
- PPQ-position Host Phase Lock and non-4/4 bar handling
- control-rate/CPU optimisation of per-sample coefficient calculations
- Character modes and final factory-preset tuning
- English localisation and accessibility pass
- macOS/Windows signing, notarisation and installers
- real DAW and real-programme listening verification

The front/back model is a musical perceptual approximation, not HRTF or
physical binaural rendering.

## Licensing

See `LICENSE_NOTES.md`. Confirm the JUCE licence, DNA Orbit source licence,
code signing, notarisation and installer requirements before distribution.

## Manual release checks

See `MANUAL_REQUIRED.md` and the documents under
`docs/commercial-upgrade/`. At minimum, verify Stereo Preserve at
0/25/50/70/100%, automate Soft Bypass, sweep Mix with Auto Gain on/off, test
mono fold-down, save/reopen projects, render offline, and measure multiple
instances with the editor open and closed.
