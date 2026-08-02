# ADR-004: Stereo Preserve as centred Mid orbit plus Side bed

- Status: Accepted for Phase 2.5 branch
- Date: 2026-08-02
- Supersedes the fresh-instance DSP interpretation in ADR-003
- Legacy schema-1 behaviour at Stereo Preserve 0% remains unchanged

## Context

ADR-003 introduced `sourceA = M + pS` and `sourceB = M - pS`. This fixes the anti-phase-silence failure and retains L/R identity, but it also feeds different programme content into the two moving strands.

The geometric midpoint of two antipodal coordinates is still zero, but that fact alone does not guarantee an energy-weighted or perceptual centre of zero. With L-only, R-only, or strongly unbalanced stereo material, one strand can carry more energy than the other. The listener can therefore hear the orbit leaning toward one side even though the visual midpoint remains centred.

That conflicts with the primary product idea: the two moving DNA strands should form one centred organism, while the source's original stereo width should remain available around it.

## Decision

The signal is separated into Mid and Side:

```text
M = 0.5 * (L + R)
S = 0.5 * (L - R)
```

The moving DNA strands and Core are driven only by `M`:

```text
strandA source = M
strandB source = M
core source L/R = M
```

Stereo Preserve controls a separate stationary Side bed:

```text
sideBed = p * S
wetL += sideBed
wetR -= sideBed
```

The geometry-based Auto Gain is applied to the Mid-orbit section before the Side bed is added.

NULL CORE is applied after the Side bed so its definition remains literal: the complete Wet output becomes Side-only.

## Why this is preferred

1. Both moving strands always carry the same programme content.
2. At Symmetry 100%, content imbalance cannot make one moving strand dominate merely because the source L/R channels differ.
3. A pure Side signal remains exactly centred in energy: equal L/R magnitude and zero Mid.
4. Strongly anti-phase stereo input remains audible.
5. Stereo Preserve 0% is the exact legacy path.
6. The original Side component is not multiplied by the Mid-orbit Auto Gain estimate.
7. The product concept becomes clearer: a centred moving helix surrounded by the source's retained stereo atmosphere.

## Trade-offs

- The original Side component is stationary rather than orbiting.
- Stereo Preserve no longer means that Strand A directly receives L and Strand B directly receives R.
- At high Preserve values, the effect can sound like a moving centred layer plus a stable wide layer. This is intentional but requires real listening to select the final default.
- Dry/Wet correlation-aware normalization remains a separate Phase 3 task.

## Compatibility

- Schema 1 loads with Stereo Preserve 0%, exactly preserving the old Mid-only path.
- Schema 2 projects created on the short-lived ADR-003 branch use the same parameter ID and range, but the interpretation at values above 0 changes. This branch has not been released commercially; the model is corrected before release rather than preserving a known conceptual flaw.

## Automated acceptance criteria

- At Preserve 0%, baseline regression fingerprints remain unchanged.
- For pure Side input, output Mid is zero and L/R RMS is equal.
- Enabling Auto Gain does not alter the preserved pure Side bed.
- For identical input and orbit state, the difference between Preserve 0% and 100% is exactly the stationary Side bed.
- NULL CORE remains Side-only after preservation.

## Manual acceptance criteria

Test 0/25/50/70/100% on:

- mono vocal
- L-only/R-only material
- wide pad
- uncorrelated stereo
- anti-phase stress material
- full mix

Choose the shipping default only after confirming that the centred DNA remains audible without the original stereo bed masking the motion.
