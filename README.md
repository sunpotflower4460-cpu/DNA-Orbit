# DNA Orbit

A DNA-shaped spatial effect plugin. Two copies of the input signal ("strands")
orbit a shared centre axis on opposite sides, like the two strands of a DNA
helix rotating together. Left/right panning is combined with amplitude,
high-frequency, and micro-delay cues to fake a front/back circular motion on
top of ordinary stereo — it is not a simple auto-panner.

- **C++20 / JUCE 8.0.15 / CMake**
- Formats: **VST3**, **Audio Unit** (macOS only), **Standalone**
- Plugin name: **DNA Orbit** — internal code `DnaO` — manufacturer **Flower Pot Studio**

## 1. What it does

Each strand's position is:

```
xA = radius * sin(thetaA)      // left/right
zA = cos(thetaA)                // front/back
xB = radius * sin(thetaB)
zB = cos(thetaB)
```

In the default, fully symmetric state, `thetaB = thetaA + pi`, so the two
strands sit on exactly opposite sides of the shared axis and rotate together
at the same speed. Their **spatial centroid** — `((xA+xB)/2, (zA+zB)/2)` —
stays at the origin.

Important: **"centre = 0" describes the spatial centre of gravity of the two
strands, not silence.** DNA Orbit never cancels the signal in its normal
mode. Both strands keep the source's normal polarity at all times in HELIX
mode; polarity inversion is never used to create the "empty centre" effect.

## 2. HELIX vs. NULL CORE

| | HELIX (default) | NULL CORE (experimental) |
|---|---|---|
| Default state | **ON** (always active) | **OFF** |
| What it does | Places the two strands symmetrically; keeps normal polarity | Zeroes the **Mid** component of the **Wet** signal only (`Mid = 0`, output = Side only) |
| Mono safety | Safe — does not collapse to silence when summed to mono | **Not safe** — the processed (Wet) signal can nearly disappear in mono |
| Applies to | Dry and Wet | **Wet only.** Dry is always left untouched |
| Intended use | Everyday music production | Deliberate, occasional sound-design effect |

When NULL CORE is switched on, the UI shows a persistent red warning —
**「NULL CORE — モノラルで消える可能性があります」** — and the toggle itself is
red. The warning tracks the *parameter*, so it stays correct under host
automation and preset loads, not just UI clicks. Switching NULL CORE on/off
crossfades over ~120 ms to avoid clicks. The **Core** knob (adds a centred copy of the source into the Wet
signal) has no audible effect while NULL CORE is on, since NULL CORE removes
exactly the Mid content Core would add — this is expected, not a bug.

## 3. The interface

The window is split into a 基本 (Basic) tab and a 詳細 (Detail) tab, with the
3D visualiser always visible between them. UI text is Japanese.

**基本 (Basic)** — four knobs only, plus action-named presets:

| Label | Parameter | Meaning |
|---|---|---|
| 速さ | `rate` | How long one revolution takes |
| 広がり | `radius` | Left/right spread |
| 立体感 | `depth` | Front/back depth |
| 効果量 | `mix` | Blend against the dry signal |

Presets are named after what they do rather than what they are — 
ボーカルを広げる / パッドを回す / ギターに揺らぎ / シンセを速く回す /
実験:中心を消す — so the plugin is usable before touching a knob.

**詳細 (Detail)** — テンポ同期 + 分割, 対称性, ねじれ, 中心の芯, 出力,
ステレオ保持, 音量自動補正, and NULL CORE (marked in red). Every control has a
Japanese tooltip.

### The 3D visualiser

The double helix is **not decoration: it is the true time-history of the two
strands' (x, z) positions.** The vertical axis is time, x is pan, z is
front/back depth. Two antipodal points rotating over time trace a double helix
as a matter of geometry, so the DNA shape falls out of the actual physics.
It reuses the same `OrbitMath` functions the audio path uses.

This makes the "+/- rotating, centre = 0" behaviour directly visible:

- The **centre line** is the running midpoint of the two strands, drawn over a
  dead-straight reference axis. At Symmetry 100% it sits exactly on that
  reference; below 100% it visibly waves off it and turns orange.
- The **readout** shows the physics literally — e.g. `A +0.069 / B -0.069 /
  中心 0.0000` when locked, versus `A -0.247 / B -0.797 / 中心 -0.5223` with
  103° of phase error when drifting.
- The **top-down inset** shows the true instantaneous orbit (an ellipse, since
  only x is scaled by Radius) with A and B antipodal, plus a trail of the
  centroid's wander.

Rendering is real 3D geometry — perspective projection, per-segment depth
sorting, shaded sphere sprites with specular and rim lighting — rasterised
through JUCE's normal 2D API. That means Direct2D on Windows and CoreGraphics
on macOS, i.e. GPU-composited, **without ever creating an OpenGL context** and
so without the context-loss and DAW-conflict failures that come with one.
Frame cost is measured at runtime and quality steps down automatically if it
exceeds budget.

## 4. Parameters

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Rate | `rate` | 0.02 – 4.0 Hz | 0.12 Hz | Orbit speed (log-skewed). Displayed as seconds-per-revolution (`8.33秒/周`), which is far more intuitive for a slow orbit than Hz |
| Sync | `sync` | on/off | off | When on, Rate is derived from host tempo + Division |
| Sync Division | `division` | 4 bars / 2 bars / 1 bar / 1/2 / 1/4 / 1/8 | 1 bar | Cycle length in beats when Sync is on |
| Radius | `radius` | 0 – 100% | 80% | Left/right spread of both strands |
| Depth | `depth` | 0 – 100% | 55% | Strength of the front/back cues (gain, filtering, delay) |
| Symmetry | `symmetry` | 0 – 100% | 100% | 100% = centroid locked exactly at the origin. Below 100%, Strand B's rate differs slightly and the centroid drifts |
| Twist | `twist` | 0 – 20 ms | 5 ms | Extra delay on Strand B only, for decorrelation between the two strands |
| Core | `core` | 0 – 100% | 0% | Adds a centred copy of the source into the Wet signal |
| Null Core | `nullCore` | on/off | **off** | See section 2 above |
| Mix | `mix` | 0 – 100% | 35% | Equal-power Dry/Wet |
| Output | `output` | -12 – +6 dB | 0 dB | Final output trim |
| Auto Gain | `autoGain` | on/off | **on** | Level-matches Wet to Dry so moving Mix does not change perceived loudness |
| Stereo Preserve | `stereoPreserve` | 0 – 100% | **70%**, candidate pending listening (0% for projects saved before this parameter existed) | How much of the input's Side content is added back as a separate width "bed" alongside the strands. See below |

When host tempo is unavailable while Sync is on, the plugin falls back
safely to the free-running Rate knob rather than guessing a tempo.

### Stereo Preserve

Before this parameter existed, both strands (and Core) were fed from a
single Mid downmix (`0.5 * (L + R)`), which meant a strongly anti-phase
stereo source (`L ≈ -R`) collapsed the Wet signal toward silence, and a wide
stereo source lost its left/right identity on the way in.

An earlier version of this parameter fixed that by feeding Strand A and
Strand B directly from `M + p*S` and `M - p*S`. That kept the two strands
exactly *geometrically* antipodal, but for an asymmetric input (e.g.
L-only, or anything hard-panned) it could make one strand's *energy* far
exceed the other's — so even though their positions stayed exactly
opposite, the perceptually-weighted centre drifted toward whichever strand
carried more signal. Measured at the shipped 70% default, an L-only input's
energy-weighted centre bias reached 94% of the geometric radius — in
practice, "one dancer, not two." See
`docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md` for the
full derivation and measurements.

The current design fixes this structurally instead of by degree: both
strands and Core are **always** fed from Mid only, at any `stereoPreserve`
value, so their energies are exactly equal by construction (not just on
average) — the two strands' energy-weighted centre stays exactly on the
geometric centre regardless of input, `stereoPreserve`, or Symmetry. Stereo
width is restored by a separate, non-orbiting "bed" term instead:

```
M = 0.5 * (L + R)
S = 0.5 * (L - R)
bedL =  p * S
bedR = -p * S
wetL = (strand A + strand B + Core, all Mid-fed) + bedL
wetR = (strand A + strand B + Core, all Mid-fed) + bedR
```

At `p = 0` the bed is silent, which is exactly the old shared-Mid behaviour
(including the anti-phase-collapses-to-silence case, unchanged). Mono input
(`L == R`) is unaffected at any `p`, since Side is always 0. Because
`bedL + bedR == 0` identically, the bed alone can never worsen mono
fold-down safety — measured across seven input conditions (dual mono,
L-only, R-only, a 6 dB L/R difference, uncorrelated stereo, anti-phase, a
wide pad), the mono-summed level stays within ~0.1% of its `p = 0` value at
every `stereoPreserve` setting (see ADR-004; reproducible via
`Tools/StereoPreserveAnalysis.cpp`, built with `-DDNA_ORBIT_BUILD_TOOLS=ON`).

**Compatibility**: a project saved before this parameter existed (state
schema 1) has no `stereoPreserve` node in its saved XML at all. Loading it
forces `stereoPreserve` to 0% rather than the new default, so the project
reproduces its original sound exactly — see `Tests/BaselineRegressionTests.cpp`
for the pinned numeric fingerprints (including a literal per-sample
reference-buffer comparison) and ADR-004 for the full design rationale,
including why the spec's proposed extra loudness-matching normalizer for
this knob was deliberately left out, and why 70% is a candidate default
pending real-material listening rather than a confirmed final value.

### Presets

Selectable from the プリセット menu in the 基本 tab. Defined in
`Source/Presets.h` (GUI-independent, unit-tested) as a point in the full
13-parameter space — Sync, Division, Output, Auto Gain and Stereo Preserve
are set explicitly by every preset (Sync off, Division 1 bar, Output 0 dB,
Auto Gain on, Stereo Preserve 70%, unless noted below), so choosing a preset
is deterministic: the result never depends on what was set before. The
values are then saved with the project like any other setting.

Once you nudge anything after picking a preset, a 元に戻す (Revert) button
appears next to the menu to snap back to the preset's exact values.

| Preset | Rate | Radius | Depth | Symmetry | Twist | Core | Mix | Null Core |
|---|---|---|---|---|---|---|---|---|
| ボーカルを広げる | 0.10 Hz | 75% | 45% | 100% | 4 ms | 10% | 30% | off |
| パッドを回す | 0.18 Hz | 100% | 65% | 100% | 7 ms | 10% | 45% | off |
| ギターに揺らぎ | 0.08 Hz | 80% | 60% | 88% | 6 ms | 15% | 40% | off |
| シンセを速く回す | 0.60 Hz | 90% | 70% | 100% | 8 ms | 0% | 40% | off |
| 実験:中心を消す | 0.04 Hz | 100% | 50% | 100% | 8 ms | 0% | 30% | **on** |

### Level matching (Auto Gain)

Measured, the raw wet path sits about **7 dB below dry** at defaults (about
10 dB for wide stereo input) and swings to roughly **+4 dB** at Core 100%, so
raising Mix used to lower the perceived level and made A/B comparison
misleading.

Auto Gain (on by default) cancels this with a gain derived purely from the
current geometry — it is **deterministic, not an RMS follower, so it cannot
pump**. Each channel carries three copies of the source at different delays
(strand A, strand B, and the undelayed Core), so the estimate sums their
powers plus the pairwise cross terms, weighted by a coherence factor from each
pair's delay difference.

Measured result across an 81-point radius/depth/twist/core sweep: worst
deviation **-1.20 dB**; a tonal source lands at **-0.18 dB**. Two deliberate
exceptions, both documented rather than papered over:

- **NULL CORE is not compensated.** That mode is meant to be able to almost
  vanish in mono; for near-mono material the required boost would be unbounded.
- **Wide/uncorrelated stereo input stays ~3.6 dB down**, because the wet path
  is built from the mono downmix and the compensation is signal-independent by
  design.

Switch Auto Gain off in the 詳細 tab to get the raw, uncompensated wet level.

## 5. Signal flow

```
Input
  +- Dry path (unchanged) --------------------------------------+
  |                                                              |
  +- Mid/Mono extraction                                        |
       +- Strand A: orbit pos -> depth gain -> depth LPF ->      |
       |            fractional delay -> equal-power pan   --+   |
       +- Strand B: (+ twist delay) orbit pos -> depth gain ->   |
                    depth LPF -> fractional delay -> pan   --+   |
                                                              |   |
                                                        Wet sum   |
                                                          + Core  |
                                                     [optional]   |
                                                    NULL CORE     |
                                                    (Wet only)    |
                                                          |       |
                                            Equal-power Dry/Wet mix
                                                          |
                                                  Output gain -> Output
```

Stereo input uses `Mid = 0.5*(L+R)` as the source signal for both strands
and the Core signal; Dry keeps the original L/R. Mono input uses the mono
signal directly for both Dry and Wet.

## 6. Building on macOS

Requirements: Xcode command line tools, CMake ≥ 3.22.

```sh
git clone <this-repo> DNA-Orbit
cd DNA-Orbit
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

Or generate an Xcode project:

```sh
cmake -S . -B build-xcode -G Xcode
cmake --build build-xcode --config Release
ctest --test-dir build-xcode -C Release --output-on-failure
```

JUCE is fetched automatically via CMake `FetchContent` from
`https://github.com/juce-framework/JUCE.git`, pinned to tag `8.0.15`. If you
need to build offline, clone JUCE yourself and point CMake at it:

```sh
git clone --branch 8.0.15 --depth 1 https://github.com/juce-framework/JUCE.git
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=$(pwd)/JUCE
```

Formats built: **Standalone**, **VST3**, **AU** (AU only builds on macOS).

## 7. Building on Windows

Requirements: Visual Studio 2022 (Desktop C++ workload), CMake ≥ 3.22.

```powershell
git clone <this-repo> DNA-Orbit
cd DNA-Orbit
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Formats built: **Standalone**, **VST3** (AU is not available on Windows).

## 8. Building on Linux (used to validate this project during development)

Requires ALSA, X11, FreeType/Fontconfig, and GTK3 development packages
(`libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev libfreetype6-dev
libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev
libxrandr-dev libxrender-dev libgtk-3-dev` on Debian/Ubuntu), plus
`fonts-noto-cjk` for the Japanese UI text. Same commands as macOS above. Formats built: **Standalone**, **VST3** (no AU on Linux).

## 9. Testing

```sh
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

The suite (`Tests/DNAOrbitTests`, run via CTest) covers, using JUCE's
built-in `UnitTest` framework:

- **OrbitMath** — perfect-symmetry centroid lock (2000+ angles, < 1e-5
  error), same-position centroid, 90° analytical check, radius-mismatch
  centroid bias, rate-difference beat-cycle behaviour, equal-power pan
  unity-power check, front/back mapping.
- **HelixEngine** — silence-in sanity at parameter extremes, impulse-response
  finiteness, all 5 required sample rates × all 9 required block sizes,
  all-parameters-at-extremes stress test, long-run Symmetry-100% centroid
  stability (angles stay wrapped, centroid stays locked), parameter-jump
  click bound, Mix 0% == Dry, NULL CORE + Mix 100% mono cancellation, NULL
  CORE off does *not* cancel in mono, mono-in/stereo-out path.
- **State** — full parameter round-trip through `getStateInformation` /
  `setStateInformation`, crash-safety against null/garbage/empty state data,
  bus-layout support/rejection, bypass pass-through (including the oversized-
  block fallback), bypass keeping the engine's orbit phase advancing instead
  of freezing it, state schema versioning, migrating a pre-Phase-1 project's
  flat editor-state properties into their own `uiState` node, a fresh
  instance defaulting Stereo Preserve to 70%, and a schema-1 project loading
  with it forced to 0% instead.
- **Presets** — every factory preset sets all 13 parameters the same way
  regardless of prior state (determinism), the Modified indicator matching
  right after applying a preset and going false once nudged, and Revert
  (re-apply) restoring the matched state.
- **StereoPreserve** — 0% still collapses anti-phase input to silence
  (legacy path unaffected), above 0% keeps it audible, Wet level scales
  linearly with the parameter for anti-phase input, mono input is
  unaffected at any value, 100% ties the output to its originating channel,
  parameter automation is smoothed rather than stepped, and — the Phase 2.5
  re-verification (ADR-004) — L-only and R-only input produce an *identical*
  Strand+Core contribution at every Stereo Preserve value (differing only by
  the exact, predicted bed contribution), proving the energy-weighted centre
  stays on the geometric centre rather than drifting toward whichever
  strand an asymmetric input happens to favour.
- **LevelMatch** — the 81-point Auto Gain sweep described above, Auto Gain
  off restoring the raw level, the correlation meter against known
  mono/inverted signals, and the RMS meter returning to zero on silence.
  Driven with pink noise rather than white: the Depth low-pass has unity DC
  gain but removes real energy from a broadband signal, and white noise
  exaggerates that far beyond any real programme material. Also: Auto Gain's
  measured deviation across Stereo Preserve values (the bed is deliberately
  uncompensated - see ADR-004), and that the correlation-aware Mix Law
  removes the classic equal-power-law loudness bump at Mix 50% for strongly
  correlated Dry/Wet.
- **BaselineRegression** — schema-1 RMS/peak/checksum fingerprints for three
  fixed signals, plus (following a request to distinguish "numerically
  regression-compatible" from a literal claim of bit-for-bit equality) a
  genuine sample-by-sample comparison against a literal reference buffer for
  a short, fully deterministic scenario.
- **Geometry** — that the helix's stored shape is identical at 0.02 Hz and
  4 Hz (measured difference: 0.0 rad), that `phi == pi` puts the centroid at
  zero for every stored sample (residual 1.7e-15, the floating-point floor),
  that a phase error visibly displaces it, that a long message-thread stall
  resynthesises rather than looping, and that the projection fits inside the
  component at every supported window size and never folds over.

## 10. Where the build output lands

After `cmake --build build --config Release`:

- Standalone: `build/DNAOrbit_artefacts/Release/Standalone/DNA Orbit`
  (`.app` on macOS, `.exe` on Windows)
- VST3: `build/DNAOrbit_artefacts/Release/VST3/DNA Orbit.vst3`
- AU (macOS only): `build/DNAOrbit_artefacts/Release/AU/DNA Orbit.component`

## 11. JUCE version

**8.0.15**, pinned via `GIT_TAG` in `CMakeLists.txt`'s `FetchContent_Declare`.

## 12. Licensing

See `LICENSE_NOTES.md`. JUCE is dual-licensed (AGPLv3/free tiers vs. paid
Indie/Pro tiers); **confirm which tier applies before distributing a built
copy of DNA Orbit**, since obligations (source availability, splash screen,
attribution) differ between tiers. This build deliberately leaves JUCE's
splash-screen requirement at its default (enabled), matching the free
tiers' terms.

## 13. Known limitations

- **Built and tested on Linux in this environment** (VST3 + Standalone
  only — AU is macOS-only and could not be built or verified here). The
  CMake configuration targets macOS as primary and is Windows-buildable,
  but neither macOS nor Windows builds were exercised in this session; use
  the build commands above to verify on those platforms.
- No `pluginval` or DAW was available in this environment, so validation is
  limited to the automated CTest suite plus headless screenshot rendering.
- **This container has no audio device, so the Standalone build's visualiser
  is frozen** — with no audio callback, `processBlock` never runs and the
  orbit phase never advances. That is an environment limitation, not a
  plugin bug. The `RenderShots` tool (below) exists to work around it, and
  is what was used to verify the animated locked/drifting behaviour.
- Presets are applied from the UI menu; they are not exposed as DAW
  program-change slots (`getNumPrograms()` is still 1).
- Japanese text needs a Japanese font installed. macOS (Hiragino) and
  Windows (Yu Gothic UI) have one by default; on Linux install
  `fonts-noto-cjk`, otherwise the labels fall back to a face with no CJK
  coverage.
- Sync Division assumes a 4/4 time signature when converting bars to beats.
- The back-position filter/delay/gain model (section 5) is a set of
  perceptual approximations, not a physically modelled 3D audio engine —
  by design, per the spec (no HRTF, no true binaural front/back cues).
- The 3D view's vertical axis is time, not a third audio dimension. The
  audio itself uses only pan (x) and the front/back cues (z).
- Auto Gain does not compensate NULL CORE, and leaves wide/uncorrelated
  stereo input about 3.6 dB down — both deliberate, see section 4.
- Renderer performance was measured only on this container's software
  rasteriser. On macOS/Windows the same code runs on CoreGraphics/Direct2D
  and should be faster, but that has not been measured on real hardware.

### Developer tool: `RenderShots`

Renders editor screenshots headlessly by driving the processor directly and
pumping the message loop, so the visualiser animates without an audio device:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDNA_ORBIT_BUILD_TOOLS=ON
cmake --build build --config Release -j --target DNAOrbitRenderShots
./build/DNAOrbitRenderShots <output-directory>
```

It writes a locked shot, a detail-tab shot, two drift shots and a NULL CORE
shot. Off by default (`DNA_ORBIT_BUILD_TOOLS=OFF`).

## 14. Manual checks to run in a real DAW before shipping

- [ ] Load in at least one DAW as VST3 (and AU, on macOS) and confirm the
      plugin scans, loads, and its editor opens at 900×620, resizable
      between 780×540 and 1600×1100.
- [ ] Confirm all Japanese text renders correctly (no boxes or garbled
      characters) on the target OS, at both small and large window sizes.
- [ ] Switch between the 基本 and 詳細 tabs; confirm the selected tab and the
      window size are restored after closing and reopening the editor.
- [ ] Try each preset from the プリセット menu and confirm it sounds sensible
      on the source it is named for.
- [ ] Watch CPU while the editor is open vs closed, and with several
      instances open at once — the visualiser stops its timer when hidden,
      so confirm that actually happens in your host.
- [ ] Automate Rate, Radius, Depth, Symmetry, Twist, Core, Mix, Output from
      the DAW and confirm no clicks/zippering.
- [ ] Toggle Null Core repeatedly while playing audio; confirm the
      crossfade is click-free and the warning text appears/disappears.
- [ ] Set 対称性 to 100% and watch the centre line: it should be dead
      straight, sitting exactly on the reference axis, with 中心軸 固定 (0)
      and a readout of A +x / B −x / 中心 0.0000. Lower it and confirm the
      line visibly waves, turns orange, and the status becomes 中心軸 ゆらぎ.
      Return it to 100% and confirm it re-locks smoothly within ~0.2 s
      rather than snapping.
- [ ] Toggle 音量自動補正 while playing and sweep 効果量 0→100%; with it on
      the perceived level should stay put, with it off it should drop.
- [ ] Sum a HELIX-mode mix to mono and confirm the signal does **not**
      disappear. Sum a NULL CORE mix to mono and confirm the Wet portion
      **does** collapse toward silence while Dry (if Mix < 100%) remains.
- [ ] Enable Sync, change host tempo and Sync Division, confirm the orbit
      rate follows tempo; disable/break tempo reporting (e.g. a plain audio
      track host) and confirm it falls back to the free Rate value instead
      of glitching.
- [ ] Save a project, close and reopen it (or bounce state through your
      DAW's own load/save), confirm every knob, toggle, and the Division
      choice come back exactly as left.
- [ ] Feed mono and stereo tracks into the plugin (mono track routed to a
      stereo bus) and confirm both input configurations sound sensible and
      produce no channel-count errors.
- [ ] Listen on headphones and on speakers; confirm the "orbit" character
      doesn't degenerate into a flat left/right tremolo, especially with
      Depth raised above ~50%.
- [ ] Check CPU load with several instances running simultaneously.
