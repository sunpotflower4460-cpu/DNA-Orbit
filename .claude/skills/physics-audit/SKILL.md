---
name: physics-audit
description: Audit or design DNA Orbit geometry, motion, phase, delay, attenuation, filtering, propagation, transport synchronization, or physical-acoustics claims. Use for every non-trivial DSP physics change and whenever “physical,” “accurate,” “realistic,” or “simulation” is discussed.
---

# Physics and numerical-fidelity audit

Read `docs/claude-code/PHYSICS_FIDELITY_STANDARD.md` and the relevant source, tests, ADRs, and product claims. Perform the audit before approving or implementing the change.

## Required audit

### A. Define the model

- What entities exist: source, listener/receiver, medium, coordinate frame, host timeline, signal channels?
- What is input, state, output, and parameter?
- What is continuous-time and what is discrete-time?
- Define every symbol and unit.
- State the truth class: EXACT, NUMERICAL APPROXIMATION, PSYCHOACOUSTIC MODEL, ARTISTIC EXTENSION, or UNKNOWN.

### B. Check physical consistency

- dimensional analysis;
- coordinate handedness and sign conventions;
- causality and non-negative propagation delay;
- energy/amplitude interpretation;
- pressure versus intensity;
- distance singularities and near-field limits;
- medium assumptions and speed of sound;
- moving-source Doppler implications;
- receiver/directivity/HRTF assumptions;
- mono/stereo/binaural rendering topology;
- whether the claimed percept can actually follow from the implemented cues.

### C. Check numerical consistency

- integration/phase accumulation error;
- wrap and long-duration drift;
- sample-rate scaling;
- interpolation magnitude/phase error;
- time-varying delay/filter sidebands;
- coefficient stability and transition behavior;
- floating-point range, denormals, NaN/Inf propagation;
- deterministic restore and offline render;
- block-boundary and transport discontinuities.

### D. Test limiting cases

At minimum consider:

- radius/depth/rate/twist/core at zero and maximum;
- Symmetry 100% and below threshold;
- Start Phase boundaries and reverse direction;
- stopped transport, start, loop, seek, tempo and time-signature change;
- silence, DC, impulse, single sine, near-Nyquist sine, pure Side, and mono;
- 44.1 through 192 kHz and variable block sizes.

### E. Compare alternatives

For every proposed model, identify:

- simplest correct reference implementation;
- current implementation;
- higher-fidelity option;
- musical/artistic option;
- CPU/latency/state/automation consequences;
- what evidence would justify moving to the more complex option.

## Output format

Return:

1. **Verdict:** sound / conditionally sound / physically misleading / insufficient evidence.
2. **Truth class and scope.**
3. **Reference equations and units.**
4. **Assumptions and invalid regimes.**
5. **Implementation mismatches, ranked by severity.**
6. **Required tests and measurements.**
7. **Required wording changes to UI/docs.**
8. **Recommended smallest next step.**

Do not approve “physically accurate” wording unless the implemented scope and validation support it. A musically excellent psychoacoustic model is acceptable when honestly labeled.
