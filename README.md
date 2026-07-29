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
**"NULL CORE — MONO MAY DISAPPEAR"** — both in the top bar and inside the
orbit display. Switching NULL CORE on/off crossfades over ~120 ms to avoid
clicks. The **Core** knob (adds a centred copy of the source into the Wet
signal) has no audible effect while NULL CORE is on, since NULL CORE removes
exactly the Mid content Core would add — this is expected, not a bug.

## 3. Parameters

| Parameter | ID | Range | Default | Notes |
|---|---|---|---|---|
| Rate | `rate` | 0.02 – 4.0 Hz | 0.12 Hz | Orbit speed (log-skewed control) |
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

When host tempo is unavailable while Sync is on, the plugin falls back
safely to the free-running Rate knob rather than guessing a tempo.

### Factory presets (reference values — not stored as DAW-loadable preset
files in v1; dial these in manually or use as automation starting points)

- **Stable Helix** — Rate 0.10 Hz, Radius 75%, Depth 45%, Symmetry 100%, Twist 4 ms, Core 0%, Mix 30%, Null Core off
- **Wide DNA** — Rate 0.18 Hz, Radius 100%, Depth 65%, Symmetry 100%, Twist 7 ms, Core 10%, Mix 45%
- **Living Axis** — Rate 0.08 Hz, Radius 80%, Depth 60%, Symmetry 88%, Twist 6 ms, Core 15%, Mix 40%
- **Hollow Core** — Rate 0.06 Hz, Radius 100%, Depth 70%, Symmetry 100%, Twist 9 ms, Core 0%, Mix 45%
- **Null Experiment** — Rate 0.04 Hz, Radius 100%, Depth 50%, Symmetry 100%, Twist 8 ms, Core 0%, Mix 30%, Null Core **on**

## 4. Signal flow

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

## 5. Building on macOS

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

## 6. Building on Windows

Requirements: Visual Studio 2022 (Desktop C++ workload), CMake ≥ 3.22.

```powershell
git clone <this-repo> DNA-Orbit
cd DNA-Orbit
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Formats built: **Standalone**, **VST3** (AU is not available on Windows).

## 7. Building on Linux (used to validate this project during development)

Requires ALSA, X11, FreeType/Fontconfig, and GTK3 development packages
(`libasound2-dev libjack-jackd2-dev libcurl4-openssl-dev libfreetype6-dev
libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev
libxrandr-dev libxrender-dev libgtk-3-dev` on Debian/Ubuntu). Same commands
as macOS above. Formats built: **Standalone**, **VST3** (no AU on Linux).

## 8. Testing

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
  bus-layout support/rejection, bypass pass-through.

## 9. Where the build output lands

After `cmake --build build --config Release`:

- Standalone: `build/DNAOrbit_artefacts/Release/Standalone/DNA Orbit`
  (`.app` on macOS, `.exe` on Windows)
- VST3: `build/DNAOrbit_artefacts/Release/VST3/DNA Orbit.vst3`
- AU (macOS only): `build/DNAOrbit_artefacts/Release/AU/DNA Orbit.component`

## 10. JUCE version

**8.0.15**, pinned via `GIT_TAG` in `CMakeLists.txt`'s `FetchContent_Declare`.

## 11. Licensing

See `LICENSE_NOTES.md`. JUCE is dual-licensed (AGPLv3/free tiers vs. paid
Indie/Pro tiers); **confirm which tier applies before distributing a built
copy of DNA Orbit**, since obligations (source availability, splash screen,
attribution) differ between tiers. This build deliberately leaves JUCE's
splash-screen requirement at its default (enabled), matching the free
tiers' terms.

## 12. Known limitations

- **Built and tested on Linux in this environment** (VST3 + Standalone
  only — AU is macOS-only and could not be built or verified here). The
  CMake configuration targets macOS as primary and is Windows-buildable,
  but neither macOS nor Windows builds were exercised in this session; use
  the build commands above to verify on those platforms.
- No `pluginval` or DAW was available in this environment, so validation is
  limited to the automated CTest suite plus a headless Standalone launch
  under Xvfb (confirmed it starts and stays running without crashing; no
  audio device was present in the container to exercise a live audio path).
- Factory presets are documented as reference parameter values (section 3)
  rather than shipped as loadable preset files in this v1.
- Sync Division assumes a 4/4 time signature when converting bars to beats.
- The back-position filter/delay/gain model (section 4) is a set of
  perceptual approximations, not a physically modelled 3D audio engine —
  by design, per the spec (no HRTF, no true binaural front/back cues).

## 13. Manual checks to run in a real DAW before shipping

- [ ] Load in at least one DAW as VST3 (and AU, on macOS) and confirm the
      plugin scans, loads, and its editor opens at 760×480, resizable down
      to 640×400.
- [ ] Automate Rate, Radius, Depth, Symmetry, Twist, Core, Mix, Output from
      the DAW and confirm no clicks/zippering.
- [ ] Toggle Null Core repeatedly while playing audio; confirm the
      crossfade is click-free and the warning text appears/disappears.
- [ ] Set Symmetry to 100%, watch the orbit display: the white centre dot
      should sit still and "AXIS LOCKED" should show. Lower Symmetry and
      confirm the dot visibly drifts and "AXIS DRIFT" (orange) appears.
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
