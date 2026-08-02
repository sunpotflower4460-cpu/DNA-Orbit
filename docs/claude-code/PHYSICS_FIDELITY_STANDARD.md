# Physics and Numerical Fidelity Standard

## 1. Scope

DNA Orbit is a musical stereo spatial effect. Its current front/back impression is produced by panning, level, spectral darkening, and microdelay cues. That is a **PSYCHOACOUSTIC MODEL**. It is useful and musically valid, but it is not a complete simulation of a moving acoustic source around a listener.

Future work may add more physically grounded modes. This standard prevents physical terminology from outrunning implementation.

## 2. Fidelity hierarchy

### Level P0 — Exact signal geometry

Examples:

- Mid/Side conversion;
- equal-power panning equation;
- antipodal angular relation;
- centroid calculation;
- Dry/Wet endpoint identity;
- sample delay when delay is an integer.

These can be labeled **EXACT** within floating-point limits when the implementation directly follows the defined operation.

### Level P1 — Numerical signal model

Examples:

- fractional delay interpolation;
- digital filter approximation;
- parameter smoothing;
- continuous phase integrated in discrete time;
- Linkwitz–Riley crossover implementation.

These are **NUMERICAL APPROXIMATIONS**. Define the reference and quantify error across supported sample rates and parameter ranges.

### Level P2 — Psychoacoustic spatial model

Examples:

- rear darkening;
- level reduction behind the listener;
- microdelay used as depth;
- stereo-width bed;
- musical Character modes.

These are **PSYCHOACOUSTIC MODELS**. Validate perceptual usefulness and side effects; do not claim literal source propagation.

### Level P3 — Geometric propagation model

A minimally physical free-field model needs explicit source and receiver geometry:

```text
r_s(t) = source position [m]
r_r(t) = receiver position [m]
d(t)   = ||r_s(t) - r_r(t)|| [m]
tau(t) = d(t) / c [s]
```

The received signal must use retarded time:

```text
y(t) = A(d, direction, medium) * x(t - tau(t))
```

This is still incomplete without receiver/directivity and rendering topology.

### Level P4 — Listener/rendering model

For binaural output, a physical claim normally requires direction-dependent ear transfer functions:

```text
Y_L(f,t) = H_L(f, azimuth, elevation, distance) X(f,t)
Y_R(f,t) = H_R(f, azimuth, elevation, distance) X(f,t)
```

For loudspeaker stereo, the target is different: speaker feeds are designed to create an image in a listening geometry. Binaural HRTF convolution and stereo panning are not interchangeable.

### Level P5 — Environment model

Reflections, occlusion, diffraction, scattering, boundaries, and medium variation require additional models. Options include image-source methods, ray/path models, modal models, finite-difference time-domain, boundary-element methods, or measured impulse responses. Each has a validity range and computational cost.

DNA Orbit should not drift into P4/P5 claims through a few filters or delays.

## 3. Coordinate and time conventions

Every spatial change must state:

- right-handed or left-handed coordinates;
- axis directions;
- azimuth zero and positive direction;
- front/back sign;
- radians or degrees;
- world time, audio sample time, or host musical time;
- whether phase is wrapped or unwrapped;
- whether a state describes the start, centre, or end of a block.

Current conceptual convention should remain explicit:

```text
x = left/right pan coordinate
z = front/back perceptual coordinate
theta = orbit angle [rad]
```

At full symmetry:

```text
theta_B = theta_A + pi
```

For equal radius, the geometric centroid is:

```text
C = 0.5 * (r_A + r_B) = 0
```

This is an exact geometry statement. It does not imply audio cancellation, equal loudness, or a physically centred wavefield.

## 4. Host musical time

Host PPQ measures quarter-note position. It is not seconds.

For a cycle length in quarter notes `B_cycle`:

```text
phase = phase_0 + direction * 2*pi * PPQ / B_cycle
```

For bar-based divisions:

```text
quarterNotesPerBar = numerator * 4 / denominator
```

Requirements:

- define behavior when BPM, PPQ, time signature, or playing state is unavailable;
- treat stop, start, loop, seek, tempo changes, time-signature changes, offline render, and bypass;
- ensure both strands are derived deterministically when claiming full geometry determinism;
- decide whether timeline discontinuity should jump exactly, crossfade, or temporarily depart from physical/timeline accuracy;
- if smoothing a seek, document that the 30 ms path is an artistic/numerical transition rather than the exact host position during the correction.

## 5. Distance attenuation

In an ideal free field far from a point source:

- acoustic pressure amplitude is proportional to approximately `1/r`;
- intensity is proportional to approximately `1/r^2`.

Do not apply `1/r^2` directly to sample amplitude merely because it is called the inverse-square law.

A usable model must define:

- pressure or intensity domain;
- reference distance and gain;
- minimum distance to avoid singularity;
- near-field behavior;
- source directivity;
- listener/receiver response;
- whether artistic range compression is applied.

Any artistic attenuation curve must be labeled accordingly.

## 6. Speed of sound and medium

Propagation delay requires a speed of sound `c`. A fixed value near 343 m/s may be used as a documented reference for ordinary room-temperature air, but exact `c` depends on temperature and, more weakly, humidity, pressure/composition, and model choice.

Rules:

- expose environment parameters only if the product needs them;
- state the assumed environment and source for the equation;
- do not imply environmental accuracy from a fixed constant;
- clamp physically impossible or numerically unsafe values;
- test parameter changes for delay discontinuity.

For non-air fantasy environments, use separate labels:

- **physical** if based on defensible medium properties and equations;
- **extended physical** if extrapolated outside validated regimes;
- **artistic** if selected for sound.

## 7. Moving delay and Doppler

A time-varying propagation delay is not a neutral spatial cue:

```text
y(t) = x(t - tau(t))
```

The derivative of delay changes instantaneous pitch. For slowly varying delay, the local time scaling is related to:

```text
d/dt [t - tau(t)] = 1 - d tau / dt
```

Therefore:

- a literal moving-source model should expect Doppler/time scaling;
- a musical orbit may intentionally suppress, exaggerate, or decouple Doppler;
- changing fractional delay can create interpolation sidebands beyond intended Doppler;
- the chosen policy must be explicit and tested.

Never accidentally introduce pitch modulation while claiming only depth.

## 8. Spectral propagation and rear cues

Air absorption is frequency- and distance-dependent. A single rear low-pass filter is not an exact air-absorption model. It may still be a useful psychoacoustic cue.

Separate:

- medium absorption;
- source directivity;
- head/torso/pinna filtering;
- occlusion;
- artistic rear darkening.

If one filter represents several effects, label it as a combined psychoacoustic control, not physical decomposition.

## 9. Rendering topology

Before implementing spatial physics, choose the target:

- stereo loudspeaker image;
- headphones without individualized HRTF;
- binaural headphones with generic HRTF;
- individualized binaural;
- multichannel/ambisonic;
- internal artistic Mid/Side effect.

A model optimized for one topology may fail on another. DNA Orbit’s current product is an artistic stereo effect; preserve that role unless a separate mode and validation path are introduced.

## 10. Fractional delay quality

Evaluate candidate interpolation methods by:

- magnitude response versus fractional offset;
- phase/group-delay error;
- transient response;
- modulation sidebands under moving delay;
- stability in feedback if feedback is ever introduced;
- CPU and memory;
- behavior near Nyquist and at 192 kHz.

Possible methods have different tradeoffs:

- linear interpolation: low CPU, stronger high-frequency loss/error;
- Lagrange/FIR: useful magnitude behavior, frequency-dependent phase properties;
- all-pass: magnitude preserving, phase/transient tradeoffs;
- windowed-sinc/polyphase: higher fidelity and cost/latency complexity.

Do not select by order number alone. Measure the actual modulation range and content.

## 11. Filters and crossover

For every filter/crossover state:

- analog/reference prototype if applicable;
- digital transform and sample-rate dependence;
- magnitude and phase targets;
- Q and stability domain;
- coefficient update policy;
- reset and denormal behavior;
- recombination behavior;
- automation transition behavior.

For the Bass Anchor crossover, preserve and test the explicit Off compatibility path separately from the active Linkwitz–Riley path.

## 12. Numerical precision and phase

Use precision according to accumulated error:

- long-running phase, PPQ, and time calculations generally use `double`;
- audio samples may remain `float` when validated;
- wrap bounded display phase while preserving an appropriate unwrapped/history representation;
- avoid huge-angle trigonometric arguments;
- compare duplicate long renders for drift;
- test negative direction and backwards host position.

Do not claim bit identity across compilers/architectures unless deliberately guaranteed and tested.

## 13. Error budgets

Every approximation should have a budget appropriate to its purpose. Examples:

- endpoint exactness: zero tolerance where representable;
- crossover magnitude/recombination: stated dB or linear tolerance over frequency;
- phase/timeline mapping: radians or samples;
- delay interpolation: magnitude/phase/sideband thresholds;
- render determinism: sample difference or bounded residual;
- UI telemetry: looser display-only tolerance.

The error budget must come from mathematics, reference comparison, perceptual evidence, or product requirements—not convenience.

## 14. Required physical-model review questions

Before merging a physics-related change:

1. What exactly is being simulated?
2. What is deliberately not simulated?
3. Which equations and source assumptions apply?
4. What are the units and reference frame?
5. What is exact, approximate, perceptual, and artistic?
6. What regimes invalidate the model?
7. Does the discrete implementation preserve the intended invariants?
8. Does automation create unintended physical/audio behavior?
9. Is the output topology appropriate?
10. What measurement and listening evidence supports the claim?

## 15. Current product boundary

Until a separately designed and validated physical mode exists, use wording such as:

- “geometry-driven musical orbit”;
- “psychoacoustic front/back cues”;
- “physically inspired spatial motion”;
- “deterministic host-timeline geometry.”

Avoid:

- “physically accurate 3D acoustics”;
- “true source propagation”;
- “binaural simulation”;
- “HRTF-accurate”; 
- “real room physics.”

Musical quality is not diminished by honest classification. Clear boundaries make future physical work stronger.
