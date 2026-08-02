# ADR-008: Deterministic host phase modes

## Status

Accepted for draft implementation. Real-host and offline-render validation is
required before release.

## Context

The original Sync implementation converted BPM and Division into an orbit rate
but did not tie orbit phase to song position. Starting playback from different
positions, looping, reopening a project or rendering offline could therefore
produce a different spatial trajectory even with identical parameters.

Bar divisions also assumed 4/4.

## Decision

Expose three phase modes while retaining the existing Sync switch.

### Free

BPM may control speed, but phase remains continuous and independent of PPQ.
This is the compatibility mode for schema-1/2 projects.

### Retrigger

On stopped-to-playing transition, reset Strand A to Start Phase and Strand B
to the antipodal position, then run continuously.

### Host Lock

Calculate the block's nominal phase from host PPQ:

```text
phase = startPhase + direction * 2*pi * (PPQ / beatsPerCycle)
```

Bar-based divisions use:

```text
quarterNotesPerBar = numerator * 4 / denominator
```

Half-, quarter- and eighth-note divisions remain absolute quarter-note units.

A discontinuous seek or loop is detected by comparing the new host phase with
the phase expected after the previous block. A 30 ms bounded correction avoids
a hard audio discontinuity. If BPM or PPQ is unavailable, Host Lock falls back
to Free.

## Rationale

- song-position phase makes repeat playback and offline rendering reproducible;
- Free preserves the original behaviour and supports unsynchronised motion;
- Retrigger is useful for arrangements that need a repeated gesture without
  continuous timeline lock;
- time-signature-aware bars remove the 4/4-only limitation;
- a short bounded correction is predictable and avoids an unbounded one-pole
  convergence tail.

## Compatibility

Schema 3 introduces phase controls. Schema-1/2 projects migrate to:

- Phase Mode: Free;
- Start Phase: 0°;
- Direction: CW.

This retains previous speed-sync behaviour.

## Tests

- 3/4, 4/4 and 6/8 bar conversion;
- identical PPQ produces identical phase;
- quarter-cycle analytical mapping;
- direction reversal;
- Start Phase offset;
- state and preset determinism.

## Remaining risk

- host implementations differ in when PPQ, BPM and playing state are reported;
- offline render and loop behaviour must be checked in real DAWs;
- the 3D history may need an explicit timeline-revision reset so a seek does
  not draw a misleading bridge between unrelated positions;
- very frequent scrubbing may require a different visual and audio policy.

## Manual verification

Use Cubase first, then Logic, Ableton, REAPER, Studio One and FL Studio where
available. Test multiple start positions, loops, seeks, tempo and time-signature
changes, Start Phase, Direction, realtime playback and two identical offline
renders.
