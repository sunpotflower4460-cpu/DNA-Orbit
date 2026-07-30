# Manual verification required

This file lists everything the commercial-upgrade package
(`docs/commercial-upgrade/`) asks for that **cannot be executed or verified
automatically in this development environment** (a Linux container with no
audio device, no macOS/Windows, no real DAW, no Apple developer certificate,
and no installed `pluginval`/VST3 Validator/`auval`). Nothing in this file
has been run. Where a section below is not listed, it means the corresponding
package requirement either doesn't apply or has been automated already (see
`docs/commercial-upgrade/00_README_使い方.md` for the source spec and the git
log for what has actually been done).

Per the package's own rule ("実行していないビルド・試聴・DAW確認を確認済みと
書かない" — never mark unexecuted verification as done), everything here is
listed as **not done**, with the exact command or setup needed to do it.

## Other platforms

- **macOS build** (Standalone/VST3/AU, Apple Silicon + Intel/Universal):
  ```sh
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --config Release -j
  ctest --test-dir build --output-on-failure
  ```
  Not run here — this environment has no macOS. AU format specifically only
  builds on macOS at all (`FORMATS Standalone VST3 AU` in `CMakeLists.txt`
  only produces AU there).

- **Windows build** (Standalone/VST3, MSVC):
  ```powershell
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
  ctest --test-dir build -C Release --output-on-failure
  ```
  Not run here. The `M_PI` portability fix (this repo now uses
  `orbitmath::pi` everywhere instead) was made specifically to reduce risk on
  this platform, but the actual MSVC compile has not been attempted.

## Validators

- **pluginval** (strictness 10): not installed here; would need to be built
  or downloaded on a machine with an internet connection and then run against
  the built VST3/AU/Standalone artifacts, e.g.
  `pluginval --strictness-level 10 --validate "build/DNAOrbit_artefacts/Release/VST3/DNA Orbit.vst3"`.
- **Steinberg VST3 Validator**: not installed; run against the built `.vst3`
  bundle on macOS/Windows.
- **`auval`**: macOS-only tool; not available here. Once an AU build exists
  on macOS: `auval -v aufx DnaO FpSt`.

## Real DAW verification

None of Logic Pro, Ableton Live, Cubase, Studio One, REAPER, or FL Studio are
installed here. The full per-DAW checklist in
`docs/commercial-upgrade/04_QA_商用リリース仕様書.md` §6 and
`docs/commercial-upgrade/08_手動試聴_DAW検証仕様書.md` needs a human with
those DAWs installed. This includes: scan, load, editor resize, presets,
save/reopen project, automation, tempo change, loop, bypass, offline bounce,
freeze, mono/stereo track routing, multiple instances, remove/reinsert.

## Manual listening

`docs/commercial-upgrade/08_手動試聴_DAW検証仕様書.md` requires actual
listening (headphones, stereo speakers, mono fold-down, various source
material) to judge musical/perceptual quality — comb-filtering character,
whether fast Motion is nauseating, whether Bass Anchor's crossover sounds
seamless, whether Character presets feel distinct, whether Stereo Preserve's
default (70%, see the Phase 2 commit for the reasoning) sounds right across
real vocal/pad/guitar material. Nothing here substitutes automated DSP tests
(which do run, and do check the numeric claims — e.g. anti-phase no longer
silencing Wet, Mix-sweep RMS deviation bounds) for actually listening.

- **Stereo Preserve default (70%) and high-value character** (see
  `docs/commercial-upgrade/decisions/ADR-004-stereo-preserve-bed.md`, which
  supersedes ADR-003's now-abandoned design): Phase 2.5 re-verified Stereo
  Preserve's physics and found the original `sourceA = M+p*S`/`sourceB = M-p*S`
  design could bias the *energy-weighted* centre toward one strand for
  asymmetric input (e.g. L-only) even though the two strands stayed exactly
  antipodal in *position* — up to 94% of the geometric radius at the shipped
  70% default for hard-panned material. The current design (both strands
  always Mid-fed, Side content added as a separate non-orbiting "bed")
  eliminates that by construction, verified in
  `Tests/StereoPreserveTests.cpp`. Two things still need real ears rather
  than automated tests:
  1. **70% is a candidate, not a confirmed final default.** Pick the value
     via `08_手動試聴_DAW検証仕様書.md` across real vocal/pad/guitar
     material, then update `Parameters.h`'s `stereoPreserveDefaultPercent`
     deliberately (with a commit explaining the listening result), not by
     guessing further.
  2. **High Stereo Preserve values (especially with hard-panned or very
     wide/uncorrelated source material) measurably push the output
     correlation strongly negative** (see ADR-004's measurement table -
     down to roughly -0.7 to -0.9 for L-only material at 70-100%). This is
     the input's own Side content being passed through, not synthesized
     decorrelation, and the mono-summed level stays essentially unaffected
     (also measured), but whether it *sounds* natural/wide versus
     unnaturally phasey at high settings needs a listening judgement this
     environment cannot make.
  3. The spec's proposed extra loudness-matching normalizer for this knob
     (independent of the above) was deliberately NOT implemented, for the
     reasons in ADR-003/ADR-004 (conflict with schema-1 numerical-regression
     compatibility at the spec's literal values). If real listening finds
     moving Stereo Preserve noticeably changes perceived Wet loudness,
     ADR-003 already has a concrete fallback design (a relative normalizer
     anchored to 0 dB at 0%) ready to implement as a future ADR.
  `Tools/StereoPreserveAnalysis.cpp` (build with `-DDNA_ORBIT_BUILD_TOOLS=ON`)
  reproduces the numeric measurements cited in ADR-004 on demand.

- **Bass Anchor crossover character and default (120Hz)** (see
  `docs/commercial-upgrade/decisions/ADR-005-bass-anchor.md`): the
  Linkwitz-Riley 4th-order crossover's flat reconstruction is verified
  numerically (a 20Hz-20kHz sweep, `Tests/CrossoverFilterTests.cpp`), but
  whether the transition band *sounds* seamless on real kick/bass material
  (no audible dip, smearing, or phasiness right around the crossover
  frequency) needs real ears. 120Hz is the shipped default per the spec;
  it has not been tuned by listening. If it needs adjusting, update
  `Parameters.h`'s `bassAnchorDefaultHz` deliberately with a commit
  explaining the listening result.
- **CPU cost of Bass Anchor + everything else, on real hardware/DAWs, with
  multiple instances**: ADR-005 explicitly scoped OUT the broader CPU
  optimizations from spec §11 (control-rate batching for the existing
  trig/coherence/Mix-gain code, `sin(θ+π)=-sin(θ)` reuse, conditional
  `fmod`) because this container cannot produce a trustworthy CPU
  measurement and there is no evidence yet that it is needed. If a real DAW
  session with several instances shows meaningful CPU pressure, that is the
  signal to revisit ADR-005's optimizations one at a time, each verified
  against `Tests/BaselineRegressionTests.cpp` before/after.

- **Character (Natural/Vivid/Deep) intensity and quality** (see
  `docs/commercial-upgrade/decisions/ADR-006-character.md`): Vivid/Deep's
  attenuation/cutoff/delay numbers are new values chosen to progress
  sensibly from Natural (which is deliberately this plugin's original fixed
  sound, unchanged), not values verified by listening. Whether Vivid feels
  meaningfully different from Deep, whether either sounds musically useful
  rather than just "more filtered," and whether the single-pole cutoff
  shift is a strong enough spectral cue (the spec's §6.2 high-shelf/
  presence EQ was deliberately not implemented - see the ADR) all need real
  ears on real source material.

- **Host Phase Lock behaviour in a real DAW** (see
  `docs/commercial-upgrade/decisions/ADR-007-host-phase-lock.md`): the PPQ
  math, the 30ms proportional-controller convergence, and offline-bounce
  determinism are all verified numerically in
  `Tests/HostPhaseLockTests.cpp` against a synthetic playhead, but none of
  the following have been checked against a real host transport:
  1. **Loop points.** Whether Host Lock's re-target-every-block design
     actually feels seamless (no audible click/glide) at the exact instant
     a DAW's loop wraps back to the loop start, across a few different
     DAWs' loop implementations (some report PPQ discontinuously at the
     wrap, some interpolate).
  2. **Tempo changes and scrubbing.** Whether the 30ms time constant feels
     right (not too sluggish, not clicky) when a host's tempo changes
     mid-playback or the user scrubs the timeline, since this container has
     no way to drive a real, human-operated transport.
  3. **Offline bounce reproducibility end-to-end.** `Tests/HostPhaseLockTests.cpp`
     proves determinism for a synthetic identical PPQ sequence fed twice to
     a fresh engine; it does not prove that a real DAW's offline-bounce
     engine actually feeds PPQ deterministically block-to-block (this is a
     property of the host, not of this plugin, but the end-to-end result
     still needs a real bounce-and-compare check).
  4. **Retrigger vs. Host Lock, chosen musically.** Whether Retrigger's
     simpler "snap once on play" is what most users actually want by
     default versus Host Lock's continuous tracking is a UX judgement call,
     not something the numeric tests can answer.

- **Host Bypass toggle audibility** (see `docs/commercial-upgrade/decisions/ADR-001-bypass-continuity.md`):
  Phase 1 fixed the *host's own* Bypass's internal state freezing (orbit
  phase, filters, and smoothers now keep advancing on a scratch buffer), but
  deliberately did NOT add an audio crossfade to it — that was a documented,
  reasoned trade-off, not an oversight (Soft Bypass, added in Phase 5, is a
  separate in-plugin control - see the next bullet - and does not change
  this). If a real DAW listening session finds an audible click/
  discontinuity when toggling the *host's* Bypass specifically, ADR-001
  already lists a crossfade as the fallback design if this turns out to be
  needed.

- **Soft Bypass (in-plugin) real-DAW feel** (see
  `docs/commercial-upgrade/decisions/ADR-008-soft-bypass.md`): the 30ms
  crossfade's exact convergence and "no state freeze" property are verified
  numerically in `Tests/SoftBypassTests.cpp`, but two things need real ears/
  real DAW use:
  1. Whether 30ms feels right (immediate enough for a "kill the effect now"
     control, without being so fast it clicks on material with strong
     transients right at the moment of engaging/disengaging).
  2. Whether having two conceptually different Bypass controls — the host's
     own Bypass (top of most DAW channel strips) and this plugin's own
     バイパス toggle in the top bar — is clear to users rather than
     confusing. If real usage shows confusion, the tooltip wording and/or
     button placement is the first thing to revisit.

- **Mono Preview real-ear check** (see
  `docs/commercial-upgrade/decisions/ADR-009-ui-ux-batch.md`): the mono
  fold-down math and its 30ms ramp are verified numerically in
  `Tests/MonoPreviewTests.cpp`, but whether it is actually useful as a
  mono-compatibility check on real, wide-stereo/Stereo-Preserve-heavy
  material (does it clearly reveal cancellation the way engineers expect
  from this kind of utility) needs a real listening session.

- **Undo/Redo end-to-end in a real host** (see
  `docs/commercial-upgrade/decisions/ADR-009-ui-ux-batch.md`): the
  Undo/Redo mechanism itself (a single parameter change, a multi-parameter
  transaction, `Presets::apply`'s one-transaction-per-preset behaviour) is
  verified in `Tests/UndoRedoTests.cpp` by driving APVTS's ValueTree
  directly, because this headless container's test runner has no message
  loop to pump (`JUCE_MODAL_LOOPS_PERMITTED` is off for plugin targets, so
  `MessageManager::runDispatchLoopUntil()` isn't available) and
  AudioProcessorValueTreeState only mirrors parameter changes into that
  ValueTree - the thing Undo/Redo actually records - via its own internal
  ~10Hz timer. In other words: the *mechanism* is tested, but the full path
  from "drag a knob in a real DAW" through to "Ctrl+Z undoes it" has not
  been exercised end-to-end. Also unverified: whether the host's own
  Ctrl+Z (if any) conflicts or interacts oddly with the plugin window's
  Ctrl+Z when the plugin editor has keyboard focus, across different hosts.

## Signing, notarization, installers

- macOS: Developer ID Application signing, Hardened Runtime, notarization
  (`notarytool`), stapling, `codesign --verify --deep --strict`,
  `spctl --assess`. Requires an active Apple Developer Program membership and
  a macOS machine.
- Windows: code-signing certificate, `signtool`, installer signing,
  SmartScreen reputation (builds over time from real-world downloads).
- Installers: DMG/PKG for macOS, Inno Setup/WiX for Windows — not created.

## CI

Per explicit user decision (this session), **no GitHub Actions workflow has
been added**, despite the package requesting a Linux/macOS/Windows CI
matrix. What *has* been added locally, and does run in this environment:

```sh
# Standard build + test:
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure

# Sanitizer build (ASan+UBSan), Debug config, GCC/Clang only:
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DDNA_ORBIT_SANITIZERS="address,undefined"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

If GitHub Actions (or another CI provider) is wanted later, the sanitizer
option above (`DNA_ORBIT_SANITIZERS`) is already structured to drop straight
into a workflow matrix step without further CMake changes.

## clang-tidy / warnings-as-errors

Not run. `juce::juce_recommended_warning_flags` is linked (enables a broad
warning set), but no `-Werror`/`/WX` configuration or `clang-tidy` pass has
been added or executed.

## Screenshot / audio regression artifacts

`Tools/RenderShots.cpp` (build with `-DDNA_ORBIT_BUILD_TOOLS=ON`) can
regenerate current-state screenshots on demand for manual before/after
comparison; PNGs are not committed to the repository to avoid binary bloat,
so there is no automated pixel-diff regression gate — only the numeric
`Tests/BaselineRegressionTests.cpp` fingerprints (RMS/peak/checksum of fixed
signals) serve as the audio regression baseline described in the QA spec §8.
