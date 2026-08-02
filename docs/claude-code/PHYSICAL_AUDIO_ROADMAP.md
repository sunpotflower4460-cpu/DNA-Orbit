# Physical Accuracy and Maximum Audio Quality Roadmap

## Guiding principle

Do not replace DNA Orbit’s musical identity with an expensive simulation merely because it is more physical. Build physical truth and musical usefulness as separately testable layers, then combine them only where evidence supports the result.

Recommended long-term product structure:

- **Musical** — current geometry-driven psychoacoustic mode, optimized for useful records.
- **Physical** — defensible model within an explicitly limited environment/output topology.
- **Hybrid** — physically grounded timing/geometry with intentionally musical range, tone, and stability controls.

These modes should not silently share misleading labels or incompatible assumptions.

## Stage 0 — Preserve and measure the current product

Before introducing new physics:

- compile and pass the current branch;
- freeze exact endpoint, state, bypass, stereo, and transport contracts;
- establish measurement fixtures and same-machine CPU baseline;
- complete Character and preset level-matched listening;
- validate Host Lock and state restore in real DAWs;
- document current psychoacoustic transfer behavior by orbit position.

Exit criterion: current behavior is understood well enough that future change can be compared rather than guessed.

## Stage 1 — Build an offline reference laboratory

Create tools/tests outside the real-time path for:

- double-precision source/listener trajectories;
- exact distance calculation;
- configurable speed of sound;
- retarded-time propagation;
- reference fractional delay using a high-quality offline method;
- selectable pressure-amplitude distance law with minimum-distance policy;
- optional frequency-dependent medium attenuation;
- optional source/receiver directivity;
- rendered WAV and machine-readable measurements.

Do not expose this as a product mode yet.

Exit criterion: analytical and numerical reference tests pass, assumptions are explicit, and the output can be compared to simplified real-time candidates.

## Stage 2 — Decide the physical product scope

Choose one target before real-time implementation:

### Option A — Stereo loudspeaker physical inspiration

Use geometric timing and level cues while preserving a stable stereo image. This cannot claim binaural localization.

### Option B — Generic binaural headphone mode

Requires licensed/redistributable HRTF data, convolution/interpolation policy, head orientation assumptions, distance behavior, and headphone validation.

### Option C — Multichannel/ambisonic internal representation

Requires a new bus/format/decoder architecture and much broader host validation.

### Option D — Free-field source propagation without listener anatomy

Can model delay/distance/medium more physically while clearly limiting localization claims.

Exit criterion: target topology, user scenario, data licensing, CPU/latency budget, and validation strategy are approved in an ADR.

## Stage 3 — Separate propagation effects

Do not implement one opaque “Physical Amount” block. Develop and validate components independently:

1. **Geometry** — source/listener position and orientation.
2. **Propagation delay** — distance divided by speed of sound.
3. **Distance amplitude** — pressure-domain curve and near-distance policy.
4. **Medium attenuation** — frequency/distance/environment dependence.
5. **Directivity** — source and receiver direction response.
6. **Doppler** — literal, suppressed, bounded, or artistic policy.
7. **Rendering** — stereo, binaural, or multichannel conversion.
8. **Reflections/environment** — direct path remains separate from room behavior.

For each component create an independent reference, error budget, tests, CPU measurement, and bypass path.

## Stage 4 — Real-time propagation prototype

Compare real-time candidates against the offline reference.

Required work:

- fractional-delay method shootout under the actual delay and modulation ranges;
- block-independent trajectory interpolation;
- sample-accurate or explicitly bounded host automation policy;
- discontinuity policy for teleport/seek/loop;
- maximum delay and memory sizing;
- silence/non-finite/denormal/reset behavior;
- no allocation/lock/I/O in callback;
- deterministic rendering at identical state/time;
- 44.1–192 kHz and multiple-instance CPU measurement.

Keep the prototype experimental until physical and audio-quality audits pass.

## Stage 5 — Doppler policy

Physical moving delay naturally changes pitch. Choose deliberately:

- **Literal Doppler** — most physically consistent with moving source/receiver assumptions.
- **Musical bounded Doppler** — scale or constrain pitch motion.
- **No-Doppler spatial motion** — decouple visual/position movement from propagation delay, honestly classified as Hybrid/Musical.
- **Independent Doppler control** — exposes a continuum, with 100% defined as the physical reference within scope.

Test tone sidebands, transient behavior, musical intonation, automation, and source speed limits. Avoid accidental Doppler from interpolation artifacts.

## Stage 6 — Spectral and directional model

Only after delay/distance is solid:

- evaluate measured or modeled air/medium attenuation;
- evaluate source directivity;
- for binaural, choose HRTF interpolation and distance treatment;
- separate head-related filtering from medium absorption and artistic rear darkening;
- compare minimum-phase, full-phase, partitioned convolution, and latency/CPU strategies as applicable.

Data license, personalization limits, front/back confusion, elevation accuracy, and headphone dependence must be documented.

## Stage 7 — Musical integration

Create controlled mappings rather than exposing raw physics everywhere.

Potential design:

- Physical mode uses defensible meters, medium and receiver assumptions.
- Musical mode retains intuitive Radius/Depth/Character.
- Hybrid mode maps intuitive controls onto bounded physical parameters plus artistic stabilization.
- Bass Anchor remains a musical mix-stability tool and should not be misrepresented as physical propagation.
- Character may remain an artistic layer after physical rendering, with level-matched tuning.

Every mode should have distinct wording, defaults, presets, tests, and manual listening.

## Stage 8 — Environment and impossible spaces

For future spaces such as Mars, deep sea, clouds, inverted rooms, or impossible geometry:

Classify each preset/engine as:

- **Physical** — properties and equations are defensible within stated regime;
- **Extended Physical** — extrapolates a valid model beyond well-validated conditions;
- **Hybrid** — uses physical components plus artistic interventions;
- **Impossible / Artistic** — deliberately violates ordinary geometry or medium behavior.

Avoid inventing exact scientific claims for environments where required inputs or models are unknown. An honest impossible-space mode can be more valuable than fake precision.

## Parallel audio-quality roadmap

### Q1 — Measurement harness

- reusable WAV/synthetic stimulus runner;
- impulse/sweep/phase/correlation/sideband analysis;
- JSON/CSV result output;
- commit/environment metadata;
- old-state fixtures and duplicate-render comparison.

### Q2 — Fractional delay evaluation

- static offsets;
- moving offsets;
- high-frequency error;
- transient behavior;
- sidebands;
- CPU at supported sample rates;
- level-matched musical listening.

### Q3 — Mix and gain refinement

- Dry/Wet sweep by source correlation;
- orbit-position gain variation;
- true-peak risk;
- Auto Gain attack/release and automation lag;
- perceptual loudness versus energy tradeoff;
- exact endpoint protection.

### Q4 — Character and presets

- blind/hidden-reference level-matched comparison;
- attribute-based ratings;
- source-diverse tuning;
- avoid one mode winning by loudness or brightness alone;
- complete-state preset determinism.

### Q5 — CPU optimization

- profiler-guided only;
- reference/null/error comparison;
- audio-only and editor-open paths;
- high sample rates and 10+ instances;
- avoid Control Rate approximations without interpolation/error evidence.

### Q6 — Production validation

- pluginval/VST3 Validator/auval;
- LUNA and other named DAWs;
- automation, loop, seek, restore, freeze, bounce;
- long sessions and multiple instances;
- clean-machine installation, signing and notarization.

## Decisions Claude must not make silently

Require an ADR and, where subjective, human listening input for:

- changing current default sound;
- adding latency;
- introducing literal or suppressed Doppler;
- selecting stereo versus binaural physical topology;
- adding third-party HRTF/IR datasets;
- changing old-session migration behavior;
- replacing the current interpolation/filter structure;
- changing Character/preset voicing;
- trading CPU for quality beyond the agreed budget;
- calling any mode “Physical” in the UI.

## Ultimate success condition

DNA Orbit reaches maximum quality when:

- the mathematics and implementation agree;
- physical claims are scoped and defensible;
- musical modes remain immediately useful;
- intended character wins fair listening tests;
- unintended artifacts are below defined limits;
- old sessions remain safe;
- behavior is deterministic where promised;
- CPU and latency are justified by measured value;
- real hosts and distribution paths pass;
- future agents can reproduce every important decision from committed evidence.
