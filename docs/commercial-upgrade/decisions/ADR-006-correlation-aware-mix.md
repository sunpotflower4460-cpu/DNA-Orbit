# ADR-006: Bounded correlation-aware Dry/Wet normalization

- Status: Accepted on draft commercial-upgrade branch
- Date: 2026-08-02

## Context

DNA Orbit used an equal-power Dry/Wet law:

```text
gD = cos(m*pi/2)
gW = sin(m*pi/2)
```

This is correct for uncorrelated Dry and Wet. When they are strongly
correlated, however, the 50% point adds amplitudes rather than powers. If Dry
and Wet are identical and level matched, the unnormalised result is `sqrt(2)`
times the input, or +3.01 dB.

Geometry Auto Gain could make the Wet endpoint fair while still leaving this
mid-Mix bump.

## Decision

Estimate stereo Dry/Wet correlation from the previous processed block:

```text
rho = sum(Dry*Wet) / sqrt(sum(Dry^2) * sum(Wet^2))
```

Smooth the estimate over 250 ms and use it to predict the main-mix power:

```text
P = gD^2 + gW^2 + 2*rho*gD*gW
normalizer = 1/sqrt(P)
```

Safety constraints:

- predicted power is clamped to `[0.5, 2.0]`;
- correction is therefore limited to approximately ±3.01 dB;
- silence drives the estimate gradually toward zero;
- the estimate is block-rate and one block delayed, not sample-following;
- Auto Gain OFF disables both geometry makeup and correlation normalization;
- Mix 0% and 100% remain mathematically unchanged;
- Soft Bypass occurs after the normalized main mix.

## Why block-delayed and slow

The purpose is to correct a persistent correlation bias, not to make a
loudness rider. A fast RMS/correlation follower could pump on drums and vocal
transients. A previous-block estimate with a 250 ms time constant is stable,
real-time safe, deterministic for a given render and inexpensive.

## Trade-offs

- The first few hundred milliseconds after a radically different source or
  preset may use the previous correlation estimate.
- Negative Dry/Wet correlation cannot be fully repaired without risking large
  gain. The ±3 dB bound deliberately prioritises safety.
- Perceived loudness still requires real listening across programme material.
- The feature shares the Auto Gain switch rather than adding another public
  parameter, keeping the UI and automation surface smaller.

## Automated acceptance criteria

- Identical, level-matched Dry/Wet at 50% settles within 0.25 dB of the input.
- Mix 0% remains exact Dry.
- Mix 100% remains governed by Wet endpoint level matching only.
- Auto Gain OFF produces a measurably different uncompensated path.
- Existing level, legacy and bypass regression tests remain valid.

## Manual acceptance criteria

Sweep Mix 0→100→0 on:

- mono vocal;
- sustained pad;
- drums;
- wide stereo material;
- full mix;
- NULL CORE.

Listen for gain movement, pumping, delayed correction after silence and
transient overreaction. If the 250 ms time constant feels audible, adjust it
only with measurement and listening evidence.
