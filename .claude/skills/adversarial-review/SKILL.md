---
name: adversarial-review
description: Perform a hostile independent review of a DNA Orbit change before completion. Use after implementation, especially for DSP, state migration, transport, bypass, UI truth, tests, performance, or release work.
---

# Adversarial review

Assume the implementation is wrong until evidence survives review. Do not defend the author’s intent; inspect actual code, tests, diff, and documentation.

## Review lenses

Run independent passes for:

1. **Physics and mathematics** — equations, units, assumptions, invariants, limiting cases, claimed realism.
2. **Audio quality** — endpoint identity, stereo/mono, phase, filters, interpolation, modulation, alias/noise, automation, listening evidence.
3. **Real-time safety** — allocation, locks, I/O, logging, unbounded work, recursive-state poisoning, variable blocks.
4. **Host behavior** — transport, PPQ/BPM availability, bypass, reset, sample-rate change, offline render, parameter automation.
5. **Compatibility** — IDs, schema migration, old sessions, presets, UI state, defaults.
6. **Tests** — false positives, weak tolerance, inadequate warm-up, single-rate assumptions, missing regression cause.
7. **Performance** — benchmark validity, hidden algorithmic growth, high-rate/multi-instance scaling.
8. **Claims and UX** — documentation truth, disabled controls, warning state, visualiser fidelity, accessibility.

Use the specialized project subagents when available and compare their findings rather than merging them prematurely.

## Required questions

- What input or host sequence breaks this?
- What old project changes sound silently?
- What passes the current tests for the wrong reason?
- What is being called physical but is perceptual or artistic?
- Where can a parameter discontinuity create a click or pitch event?
- Where does float precision accumulate long-term error?
- What happens at 192 kHz, block size 1, block size 0, and changing block size?
- What happens with silence, DC, NaN/Inf, pure Side, and mono?
- Does bypass truly preserve internal state?
- Can two identical offline renders differ?
- Is CPU measured on the same code path users run?
- Does the UI tell the truth under automation and host restore?

## Output

Create a severity-ranked table:

- **Blocker** — crash, corruption, hearing risk, session incompatibility, false physical claim, major audible defect.
- **High** — likely host failure, significant sound regression, nondeterminism, invalid validation.
- **Medium** — edge-case defect, misleading UX, missing coverage, maintainability risk.
- **Low** — polish or future improvement.

For every finding include exact file/line, reproduction or reasoning, affected invariant, and smallest safe fix. End with a go/no-go verdict and the tests required after fixes.
