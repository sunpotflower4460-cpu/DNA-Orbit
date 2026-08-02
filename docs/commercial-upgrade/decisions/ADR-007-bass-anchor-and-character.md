# ADR-007: Bass Anchor and musical depth Character

## Status

Accepted for draft implementation. Final defaults require listening validation.

## Context

The original DNA Orbit moved the entire Mid signal through time-varying pan,
filter and delay cues. This is attractive on vocals, guitars and pads, but
sub/bass movement can destabilise kick, bass and complete mixes. The original
single depth model also forced every source into the same front/back colour.

## Decision

### Bass Anchor

Use a fourth-order Linkwitz–Riley crossover before orbit processing.

- low-band L/R are converted to Low Mid and reintroduced equally to Wet L/R;
- high-band Mid drives both moving strands and Core;
- high-band Side is restored by Stereo Preserve;
- `bassAnchorHz = 20 Hz` is an explicit bit-transparent bypass, not a 20 Hz
  crossover;
- new instances default to 120 Hz;
- schema-1/2 projects migrate to 20 Hz to retain the prior signal path.

### Character

Add three bounded perceptual depth profiles:

| Mode | Rear attenuation | Rear cutoff | Rear delay |
|---|---:|---:|---:|
| Natural | 4 dB | 5 kHz | 8 ms |
| Vivid | 6 dB | 8 kHz | 10 ms |
| Deep | 8 dB | 3.5 kHz | 14 ms |

The control interpolates between adjacent profiles during automation. It does
not add saturation, oversampling, HRTF or a generic reverb.

## Rationale

- Linkwitz–Riley is designed for flat low/high recombination and matching
  phase at crossover.
- A mono low anchor improves practical mix stability without removing
  high-frequency stereo atmosphere.
- A literal bypass at 20 Hz is necessary for old-project compatibility.
- Three named profiles expose useful musical intent without turning DNA Orbit
  into a multiband spatial workstation.

## Rejected alternatives

### Orbit the entire signal and only narrow the low Side

This would retain low Mid delay/pan motion and still allow bass transients to
wander.

### Generic multiband processing

It would greatly increase state, UI, CPU and tuning complexity before the core
product is validated.

### Oversampling

The new stages are linear filters, gain and delay. Oversampling is not added
without evidence of nonlinear aliasing.

## Compatibility

Schema 3 introduces Bass Anchor and Character. Older state is migrated to:

- Bass Anchor 20 Hz / exact bypass;
- Natural Character, which matches the original constants.

## Tests

- crossover recombination at multiple frequencies;
- anti-phase low-frequency Side removal;
- preservation of high-frequency Side;
- finite and distinct Character responses;
- complete state and preset determinism.

## Manual verification

- transient integrity around crossover;
- low-end solidity on bass-heavy sources;
- Character identity under level-matched listening;
- automation clicks and fast-motion pitch behaviour;
- final 120 Hz and preset defaults.
