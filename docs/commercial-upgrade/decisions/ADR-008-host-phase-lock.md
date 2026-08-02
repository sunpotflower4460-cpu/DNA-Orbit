# ADR-008: Deterministic host phase modes

## Status

Accepted for draft implementation. Real-host and offline-render validation is
required before release.

## Context

The original Sync implementation converted BPM and Division into an orbit rate
but did not tie orbit phase to song position. Starting playback from different
positions, looping, reopening a project or rendering offline could therefore
produce a different spatial trajectory even with identical parameters.

Bar divisions also assumed 4/4. A first Host Lock draft fixed Strand A to PPQ
but left drifting Strand B dependent on previous playback history; that was not
sufficient because the complete DNA geometry still changed between renders.

## Decision

Expose three phase modes while retaining the existing Sync switch.

### Free

BPM may control speed, but phase remains continuous and independent of PPQ.
This is the compatibility mode for schema-1/2 projects.

### Retrigger

On stopped-to-playing transition, reset Strand A to Start Phase and Strand B
to the antipodal position, then run continuously.

### Host Lock

Calculate Strand A from host PPQ:

```text
phaseA = startPhase + direction * 2*pi * (PPQ / beatsPerCycle)
```

At full Symmetry, Strand B is exactly antipodal. Below full Symmetry, its
slightly different rate is also derived from PPQ:

```text
phaseB = startPhase + pi
       + direction * 2*pi * (PPQ / beatsPerCycle) * bRateScale
```

This means both strands, their relative phase and centroid are independent of
previous playback history.

Bar-based divisions use:

```text
quarterNotesPerBar = numerator * 4 / denominator
```

Half-, quarter- and eighth-note divisions remain absolute quarter-note units.

A discontinuous seek or loop is detected by comparing the new host phase with
the phase expected after the previous block. Both strands use a bounded 30 ms
correction rather than a hard audio jump. The corrected phase, not merely the
nominal PPQ phase, is published to the visualiser.

When transport is stopped, PPQ-locked phase stays frozen even if the host keeps
calling the audio callback. If BPM or PPQ is unavailable, Host Lock falls back
to Free.

Host playhead information is queried only from `processBlock` or
`processBlockBypassed`, never from `prepareToPlay`.

## Rationale

- song-position phase makes repeat playback and offline rendering reproducible;
- both strands must be deterministic for the DNA shape itself to be repeatable;
- Free preserves the original behaviour and supports unsynchronised motion;
- Retrigger provides a repeated gesture without continuous timeline lock;
- time-signature-aware bars remove the 4/4-only limitation;
- freezing stopped transport avoids movement while PPQ is stationary;
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
- identical PPQ produces identical Strand A phase;
- both strands remain identical after different amounts of prior free-running
  playback when Symmetry is below 100%;
- stopped Host Lock remains frozen;
- quarter-cycle analytical mapping;
- direction reversal;
- Start Phase offset;
- state and preset determinism.

## Remaining risk

- host implementations differ in when PPQ, BPM and playing state are reported;
- offline render and loop behaviour must be checked in real DAWs;
- very frequent scrubbing may require a different visual/audio policy;
- explicit visual history reset should be judged in a real host. The visualiser
  currently follows the same bounded correction as the audio and already
  resynthesises on backward or extreme phase movement.

## Manual verification

Use Cubase first, then Logic, Ableton, REAPER, Studio One and FL Studio where
available. Test multiple start positions, stopped callbacks, loops, seeks,
tempo and time-signature changes, Start Phase, Direction, Symmetry below 100%,
realtime playback and two identical offline renders.
