# Current Architecture Map

Last reviewed against branch `agent/world-class-dsp-phase3` on 2026-08-02.

This map accelerates orientation. The source remains authoritative; update this file when architecture changes.

## 1. Build and product boundary

- C++20, CMake, JUCE pinned in `CMakeLists.txt`.
- Product: DNA Orbit by Flower Pot Studio.
- Current formats: Standalone, VST3, AU.
- Current output topology: mono or stereo input to stereo output.
- Current front/back behavior: geometry-driven **psychoacoustic musical model**, not HRTF/binaural/free-field/room simulation.
- GitHub Actions is not enabled; local scripts are the automated validation path.

## 2. Source responsibility map

### `Source/Parameters.h`

- stable host parameter IDs and version hints;
- ranges, defaults, display functions, choices;
- schema constants;
- musical division and rate helpers.

Parameter IDs are external session/automation contracts. Never rename or repurpose them.

### `Source/Presets.h`

- complete-state factory presets;
- preset application and current-state matching;
- must set every audible/mode parameter to avoid hidden inheritance.

### `Source/PluginProcessor.*`

- JUCE processor and bus contract;
- APVTS ownership;
- raw atomic parameter pointers;
- conversion from host/APVTS state to `HelixEngine::Parameters`;
- host BPM, PPQ, time signature, and playing-state acquisition inside process callbacks;
- normal process path;
- host-bypass exact-Dry output while advancing the engine on preallocated scratch;
- state serialization, schema detection, migration, and legacy UI-state migration.

### `Source/dsp/HelixEngine.*`

- real-time state and smoothing;
- Free, Retrigger, and Host Lock phase behavior;
- strand geometry;
- active DSP signal flow;
- Auto Gain and correlation-aware Mix normalization;
- Soft Bypass;
- lock-free UI telemetry.

This is the highest-risk product core. Apply physics, audio-quality, real-time, state, and validation rules.

### `Source/dsp/OrbitMath.h`

- mathematical constants and orbit helpers;
- wrapped/shortest angular operations;
- equal-power pan;
- position, centroid, and rate-difference helpers.

Prefer pure, independently testable math here.

### `Source/dsp/LinkwitzRileyCrossover.h`

- Bass Anchor split;
- explicit 20 Hz / Off transparent path;
- active low/high Linkwitz–Riley behavior.

Off compatibility and active crossover behavior are separate contracts.

### `Source/dsp/OnePoleLowPass.h`

- rear spectral darkening helper;
- current use is psychoacoustic Character/depth, not exact air absorption or HRTF.

### `Source/dsp/StereoUtilities.h`

- Mid/Side and mixing utilities;
- exact signal-domain operations should remain small and property-tested.

### `Source/ui/*`

- `HelixView3D`: sampled visual representation of engine telemetry;
- `HelixHistory`: orbit-history resampling for display;
- projection/sprite/look-and-feel helpers.

The UI must not become an independent simulation that disagrees with DSP.

### `Source/PluginEditor.*`

- Basic/Detail views;
- APVTS attachments;
- parameter dependencies and warnings;
- presets and reset behavior;
- accessibility and resize behavior;
- low-rate readouts from DSP telemetry.

## 3. Current signal flow

Conceptual per-sample flow:

```text
Input L/R
  -> non-finite sanitization
  -> Bass Anchor L/R crossover
       lowL/highL, lowR/highR
  -> lowMid = 0.5 * (lowL + lowR)
  -> highMid  = 0.5 * (highL + highR)
  -> highSide = 0.5 * (highL - highR)

highMid
  -> Strand A: rear gain -> rear low-pass -> fractional delay -> equal-power pan
  -> Strand B: rear gain -> rear low-pass -> fractional delay + Twist -> equal-power pan
  -> centred highMid Core
  -> geometry-based Wet makeup

Then:
  -> add lowMid equally to L/R as Bass Anchor
  -> add stationary highSide bed scaled by Stereo Preserve
  -> optional NULL CORE converts complete Wet to Side-only
  -> correlation-aware Dry/Wet mix normalization
  -> Output trim
  -> Soft Bypass interpolation toward exact Dry
  -> finite output and UI telemetry
```

Important interpretation:

- only high-band common Mid content travels through moving strands;
- low-band Mid is centred when Bass Anchor is active;
- high-band Side is stationary and controlled by Stereo Preserve;
- NULL CORE acts after those components are assembled and is intentionally mono-unsafe;
- the front/back impression comes from artistic gain, spectral, and delay cues.

## 4. Geometry and time state

### Free

- Rate advances phase continuously;
- Sync may derive Rate from BPM/division, but phase is not tied to timeline position.

### Retrigger

- stopped-to-playing transition resets to Start Phase;
- phase then runs continuously.

### Host Lock

- nominal A and B phases derive from host PPQ, cycle length, direction, Start Phase, and Symmetry/rate relation;
- stopped transport freezes at reported PPQ;
- loop/seek/large discontinuity uses bounded correction;
- identical valid host state should reproduce complete geometry within the defined numerical scope.

The bounded correction is not exact timeline phase during its transition. It is an intentional artifact-control policy and must be evaluated as such.

## 5. State schema

Current schema: 3.

- schema 1: original Mid-only Wet path;
- schema 2: Stereo Preserve Side bed;
- schema 3: Bass Anchor, Character, deterministic transport phase controls.

Legacy migration values preserve older sound and may differ from fresh-instance defaults.

## 6. UI truth path

```text
Audio thread
  -> lock-free atomic telemetry in HelixEngine
  -> HelixView3D timer polling
  -> UI-side history and smoothing
  -> geometry/readouts/warnings
```

Telemetry is sampled, not sample-accurate. It must be visually truthful within that scope and reset/resynthesize after invalid history transitions.

## 7. Automated evidence

### Unit/regression tests

`Tests/` covers current categories including:

- orbit geometry;
- finite-output stress;
- baseline and endpoints;
- state migration and round-trip;
- bypass and Soft Bypass;
- Stereo Preserve and centred Side behavior;
- correlation-aware Mix;
- crossover/Bass Anchor/Character;
- Host Lock/time signatures/direction/Start Phase;
- complete presets.

### Tools

- `Tools/RenderShots.cpp`: drives processor/editor and writes UI screenshots.
- `Tools/BenchmarkDSP.cpp`: deterministic DSP performance matrix.

### Scripts

- `scripts/agent-preflight.sh`;
- `scripts/static-realtime-audit.sh`;
- `scripts/validate-local.sh`.

## 8. Documentation hierarchy

- `AGENTS.md`: universal agent contract;
- `CLAUDE.md`: Claude Code operating memory;
- `.claude/rules`: path-specific constraints;
- `.claude/skills`: repeatable workflows;
- `.claude/agents`: independent specialist reviews;
- `docs/claude-code`: deep standards and prompts;
- `docs/commercial-upgrade/decisions`: ADRs;
- `MANUAL_REQUIRED.md`: non-automatable release work;
- `README.md`: product/user/developer overview.

## 9. High-risk change zones

A change is high risk if it touches:

- parameter IDs, schema, defaults, presets, or state root;
- `processBlock`, bypass, prepare/reset, bus layout;
- phase/transport mapping;
- recursive filters, delay interpolation, coefficient changes;
- Dry/Wet normalization, output gain, NULL CORE;
- crossover Off behavior;
- lock-free telemetry types;
- UI claims about physical/DSP state;
- CMake product identifiers or formats.

Use plan mode, relevant skills, independent audits, and full validation.

## 10. Known boundaries and pending evidence

- current branch additions require successful local compile/test before readiness;
- physical front/back behavior remains psychoacoustic;
- Character and factory presets require level-matched listening/tuning;
- Host Lock requires real-host/offline-render validation;
- screenshot output requires human inspection;
- CPU needs same-machine baseline tracking;
- plugin validators, DAW matrix, signing, notarization, and clean-machine installation remain external/manual gates.
