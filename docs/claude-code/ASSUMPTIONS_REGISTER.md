# Assumptions Register

This register prevents undocumented assumptions from becoming product facts. Update it when implementation, evidence, or product scope changes.

## Status values

- **Accepted** — supported for current scope.
- **Provisional** — reasonable but incomplete evidence.
- **Rejected** — evidence contradicts it.
- **Superseded** — replaced by a newer decision.
- **Unknown** — must not support a claim or default decision.

## Current assumptions

| ID | Assumption | Truth class | Status | Evidence / source | Invalid regimes / risk | Required next evidence |
|---|---|---|---|---|---|---|
| A-001 | Gain, spectral darkening and microdelay can create a useful musical front/back impression. | PSYCHOACOUSTIC MODEL | Provisional | Existing design and pending listening | Depends on source, speakers/headphones and listener; not literal propagation | Level-matched Character and rear-position listening matrix |
| A-002 | Current DNA Orbit is an artistic stereo effect, not a binaural/HRTF or room-physics renderer. | EXACT product-scope statement | Accepted | Current signal topology and feature set | Becomes stale if a separate validated physical mode is added | Update scope and ADR when topology changes |
| A-003 | At full Symmetry, equal-radius antipodal strand coordinates have zero geometric centroid. | EXACT | Accepted | Algebra and geometry tests | Does not imply audio cancellation or equal perceived loudness | Keep invariant tests |
| A-004 | Host PPQ and cycle length can define repeatable orbit geometry at identical timeline state. | EXACT mapping with numerical implementation | Provisional | Host Lock design and tests; real-host checks pending | Hosts may omit or vary PPQ/BPM/time signature; correction temporarily departs from nominal phase | Duplicate offline/realtime renders in named DAWs |
| A-005 | A bounded 30 ms correction is less objectionable than a hard phase jump after seek/loop. | ARTISTIC / NUMERICAL TRANSITION | Provisional | Design rationale only | Could smear motion or produce audible pitch/image movement | Seek/loop listening and waveform/sideband analysis |
| A-006 | A fourth-order Linkwitz–Riley split is suitable for centring low-frequency Mid while allowing high-band orbit motion. | NUMERICAL APPROXIMATION + product design | Provisional | Crossover tests; listening pending | Transients, phase and source dependence; automation can color sound | Bass/drum/full-mix measurement and level-matched listening |
| A-007 | Bass Anchor 20 Hz / Off is the compatibility path and must bypass the split exactly. | EXACT contract | Accepted | Implementation and exact test | Any future refactor that still filters at 20 Hz breaks compatibility | Keep zero-tolerance regression test |
| A-008 | High-band Side may be reintroduced as a stationary bed without being multiplied by the Mid geometry compensation. | PSYCHOACOUSTIC / signal-design model | Provisional | Stereo Preserve architecture and tests | May reduce perceived motion or affect mono/correlation | Source-diverse width/motion listening and Mid/Side measurements |
| A-009 | Correlation-aware Mix normalization reduces persistent level bias more fairly than equal-power gains alone for similar Dry/Wet signals. | NUMERICAL / psychoacoustic level model | Provisional | Correlation tests; program listening pending | Correlation estimate may lag automation and may not track perceived loudness | Mix sweep on synthetic and musical material, true-peak review |
| A-010 | Natural, Vivid and Deep should differ in perspective rather than quality tier. | ARTISTIC PRODUCT INTENT | Accepted | Product design | One mode may win only through level/brightness | Level-matched blind preference/attribute test |
| A-011 | No oversampling is required for the current primarily linear signal path. | NUMERICAL / architecture | Accepted for current path | No intended nonlinear stage | Becomes false if nonlinear processing is added | Add nonlinear/alias evaluation before oversampling decision |
| A-012 | JUCE 8.0.15 remains the stable production baseline while a JUCE 9 migration is evaluated separately. | TOOLCHAIN POLICY | Provisional | Current CMake pin and upgrade strategy | Security/host/compiler changes may require migration | Periodic release-note and validation review |
| A-013 | Local validation is the primary automated workflow because GitHub Actions is not enabled. | WORKFLOW FACT | Accepted | Project constraint | Local environment may drift or results may not be shared | Preserve logs/tool versions; consider CI later |
| A-014 | Float audio with double phase/time state can meet current quality goals when measured. | NUMERICAL APPROXIMATION | Provisional | Current architecture | Long renders, high-rate modulation, platform differences | Long-duration drift, duplicate render and high-frequency error tests |
| A-015 | The 3D view can truthfully represent low-rate DSP telemetry when clearly treated as a sampled visualization. | NUMERICAL UI MODEL | Provisional | Current telemetry design | UI polling/smoothing is not sample accurate; seeks may create history artifacts | Visual timeline tests and real-host loop/seek inspection |

## New assumption template

```markdown
### A-XXX — Short name

- **Assumption:**
- **Truth class:** EXACT / NUMERICAL APPROXIMATION / PSYCHOACOUSTIC MODEL / ARTISTIC EXTENSION / UNKNOWN
- **Status:** Accepted / Provisional / Rejected / Superseded / Unknown
- **Scope:**
- **Evidence:**
- **Source/version/date:**
- **Invalid regimes:**
- **Risk if wrong:**
- **Required test/experiment:**
- **Owner / next review:**
- **Related ADR/code/tests:**
```

## Review triggers

Review affected assumptions when:

- a parameter default or mode changes;
- an old schema is migrated;
- a physical or audio-quality claim changes;
- a new host/toolchain version is adopted;
- listening contradicts measurements;
- a test tolerance changes;
- optimization changes numerical behavior;
- a new rendering topology or physical mode is proposed.
