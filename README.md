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

> This commercial-upgrade branch is still a draft. Its newest changes must be
> compiled, tested and auditioned locally before merge.

## Product signal model

```text
Input L/R
  ├─ Dry path
  └─ 4th-order Linkwitz–Riley Bass Anchor
       ├─ Low Mid ───────────────────────────────┐
       └─ High band M/S                         │
            ├─ Mid → Strand A orbit ──┐          │
            ├─ Mid → Strand B orbit ──┼─ Auto Gain
            ├─ Mid → Core ────────────┘          │
            └─ Side × Stereo Preserve ───────────┤
                                                 ├─ optional NULL CORE
                                                 ├─ correlation-aware Dry/Wet
                                                 ├─ Output
                                                 └─ optional Soft Bypass → Output
```

At full Symmetry, strand B stays `pi` radians opposite strand A. Both moving
strands carry the same Mid programme content, so the orbit's centre is not
biased by unrelated left/right energy. Stereo width is restored separately as
a balanced high-band Side layer.

## Main controls

### Basic

- 速さ — free-running revolution time
- 広がり — left/right orbit width
- 立体感 — front/back cue strength
- 効果量 — correlation-aware Dry/Wet amount
- action-named factory presets

### Detail

- Tempo Sync and musical Division
- Phase Mode: Free / Retrigger / Host Lock
- Start Phase and Direction
- Symmetry and Twist
- Core and Stereo Preserve
- Bass Anchor
- Character: Natural / Vivid / Deep
- Output and Auto Gain
- Soft Bypass
- NULL CORE

## Parameters

| Parameter | ID | Range | Fresh default |
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
| Bass Anchor | `bassAnchorHz` | Off / 20–500 Hz | 120 Hz* |
| Character | `character` | Natural/Vivid/Deep | Natural |
| Phase Mode | `phaseMode` | Free/Retrigger/Host Lock | Host Lock* |
| Start Phase | `startPhase` | 0–360° | 0° |
| Direction | `direction` | CW/CCW | CW |

\* Compatibility migration:

- schema-1 projects load Stereo Preserve at 0%;
- schema-1/2 projects load Bass Anchor at 20 Hz, which is a bit-transparent
  bypass rather than a 20 Hz filter;
- schema-1/2 projects load Phase Mode as Free, reproducing the previous
  speed-synchronised but non-position-locked behaviour.

## Bass Anchor

Bass Anchor uses a fourth-order Linkwitz–Riley crossover. Below the selected
frequency, L/R are converted to a stable mono Low Mid and reintroduced after
the moving high-band orbit. Above it, the Mid DNA and preserved Side bed work
normally.

Goals:

- keep kick, bass and full mixes spatially stable;
- prevent sub/low-frequency Side energy from orbiting unpredictably;
- retain the source's upper-frequency stereo atmosphere;
- recombine low/high crossover paths with flat magnitude.

`20 Hz / Off` is a literal bypass: `low = 0`, `high = input`. This is required
for old-project signal-path compatibility.

## Character

Character controls the front/back perceptual model rather than adding generic
saturation or oversampling.

- **Natural** — original 4 dB attenuation, 5 kHz rear cutoff, 8 ms rear delay
- **Vivid** — stronger 6 dB movement, brighter 8 kHz rear cutoff, 10 ms delay
- **Deep** — 8 dB attenuation, darker 3.5 kHz rear cutoff, 14 ms delay

The values are current engineering defaults, not final artistic tuning. Final
preset values require real vocal, guitar, synth, drum and full-mix auditioning.

## Tempo and phase

Tempo Sync now separates **speed** from **phase behaviour**.

### Free

The orbit runs continuously. Host BPM changes the rate when Sync is enabled,
but the current position is not forced to the song timeline.

### Retrigger

The orbit returns to Start Phase whenever transport changes from stopped to
playing, then runs continuously.

### Host Lock

The orbit is calculated from host PPQ song position:

```text
phase = startPhase + direction * 2*pi * (PPQ / beatsPerCycle)
```

`beatsPerCycle` honours the host time signature for bar divisions. A one-bar
cycle is therefore 4 quarter notes in 4/4, 3 in 3/4 and 3 in 6/8. Half-,
quarter- and eighth-note divisions remain absolute musical note values.

The same PPQ position, cycle, direction and Start Phase produce the same orbit
position, supporting repeatable playback and offline rendering. Loop or song
position jumps use a bounded 30 ms correction instead of a hard audio jump.
When BPM or PPQ is unavailable, Host Lock falls back safely to Free.

## Stereo Preserve

Stereo Preserve restores the high-band Side component after the geometry-based
Mid-orbit Auto Gain:

```text
M = 0.5 * (L + R)
S = 0.5 * (L - R)
sideBed = p * S
wetL += sideBed
wetR -= sideBed
```

Consequences:

- the moving DNA always carries common Mid content;
- pure Side material has equal L/R energy and zero Mid;
- anti-phase material remains audible above 0%;
- Side width is not multiplied by a same-source geometry estimate;
- NULL CORE remains literally Side-only.

## Auto Gain and correlation-aware Mix

Auto Gain has two bounded stages.

1. A deterministic geometry estimate compensates the moving Mid strands and
   Core using pan, attenuation and delay coherence.
2. Dry/Wet correlation is measured per block, smoothed over 250 ms and used to
   remove the persistent +3.01 dB 50%-Mix bump that equal-power mixing produces
   when Dry and Wet are similar.

```text
P = gD² + gW² + 2*rho*gD*gW
normalizer = 1 / sqrt(P)
```

Correction is limited to approximately ±3.01 dB. Mix 0% and 100% remain
unchanged. Auto Gain Off exposes the raw geometry and mix law.

## Bypass modes

### Soft Bypass

The plug-in-owned A/B control crossfades linearly to exact Dry over 60 ms while
keeping delays, filters and orbit position running. Fully bypassed output
ignores Output trim.

### Host bypass

Host bypass returns immediate exact Dry while the engine advances on a
preallocated scratch buffer. This avoids stale phase and delay state after
un-bypass without allocating on the audio thread.

## NULL CORE

NULL CORE removes the Mid component of the complete Wet output. It is an
experimental, intentionally mono-unsafe Side-only mode. The transition is
smoothed over approximately 120 ms, the UI shows a warning, and Core is visibly
disabled while the mode is active.

## Performance work in this branch

- symmetric mode reuses `sin(theta + pi) = -sin(theta)` and
  `cos(theta + pi) = -cos(theta)` for Strand B;
- Bass Anchor coefficient rebuilds occur only when the cutoff changes;
- no new audio-thread allocations, locks, file I/O or UI calls were added;
- the exact legacy Bass Anchor-off path is maintained;
- more aggressive control-rate interpolation was deliberately postponed until
  it can be measured against a larger audio regression corpus.

## Factory presets

Factory presets are defined independently of the GUI in `Source/Presets.h`.
Every preset sets the complete parameter state, including Bass Anchor,
Character, phase mode, Start Phase and Direction, so presets never inherit
hidden values from prior automation.

Current presets:

- ボーカルを広げる
- パッドを回す
- ギターに揺らぎ
- シンセを速く回す
- 実験:中心を消す

## Visualiser

The 3D double helix is generated from the real orbit history used by the DSP.
It displays strand paths, the ideal centre reference, centroid drift, output
RMS, L/R correlation, Bass Anchor state and Host Lock status. Rendering quality
adapts to measured frame cost and animation stops while the editor is hidden.

## Build

### macOS / Linux

```sh
git clone <repository-url> DNA-Orbit
cd DNA-Orbit
git checkout agent/world-class-dsp-phase3
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

### Windows

```powershell
git clone <repository-url> DNA-Orbit
cd DNA-Orbit
git checkout agent/world-class-dsp-phase3
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

JUCE is pinned to 8.0.15 and fetched through CMake `FetchContent`.

## Test coverage

The CTest executable now includes coverage for:

- orbit geometry and finite-output stress cases;
- state migration and complete parameter round-trip;
- exact host-bypass and Soft Bypass behaviour;
- Stereo Preserve and centred Side-bed invariants;
- correlation-aware mix endpoints and 50% level behaviour;
- Linkwitz–Riley crossover recombination;
- Bass Anchor low-frequency Side removal and high-frequency preservation;
- distinct finite Character responses;
- 3/4, 4/4 and 6/8 musical bar lengths;
- deterministic Host Lock at identical PPQ positions;
- Start Phase and direction mapping;
- complete factory-preset determinism.

## Validation status

The code and tests are present in the draft PR, but this ChatGPT environment
could not download and compile the complete GitHub branch. None of the new
build/test claims are marked passed yet.

Before changing the PR from draft:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

Then run the sanitizer configuration in `CMakeLists.txt`, headless screenshot
rendering, pluginval, VST3 Validator and `auval` where available.

## Remaining commercial work

- compile-error and regression fixes discovered by the first local build;
- measured CPU profiling at 44.1/48/96/192 kHz and multiple block sizes;
- final Character and factory-preset listening/tuning;
- visual timeline reset verification on host loop/jump;
- English localisation and full accessibility pass;
- macOS/Windows signing, notarisation and installers;
- real-DAW scan, automation, project restore and offline-bounce verification.

The front/back model is a musical perceptual approximation, not HRTF or a
physical binaural renderer.

## Licensing and manual release checks

See `LICENSE_NOTES.md`, `MANUAL_REQUIRED.md` and the documents under
`docs/commercial-upgrade/`. Confirm the JUCE licence, signing, notarisation,
installer and clean-machine requirements before distribution.
