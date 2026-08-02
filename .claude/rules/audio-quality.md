---
paths:
  - "Source/**/*.{h,cpp}"
  - "Tests/**/*.cpp"
---

# Audio quality rules

- Do not equate complexity with quality. State the audible problem and the evidence that will show improvement.
- Preserve exact endpoints and compatibility paths before tuning the middle of a control range.
- Level-match comparisons to within 0.1 dB when judging timbre, width, depth, or “better.”
- Evaluate mono input, correlated stereo, decorrelated stereo, pure Side/anti-phase, hard-panned signals, silence, DC, impulses, sweeps, multitone, pink noise, transients, sustained vocals, bass, drums, guitars, pads, and full mixes as applicable.
- Check 44.1, 48, 88.2, 96, 176.4, and 192 kHz when filter, delay, modulation, or interpolation behavior may change.
- Check small, typical, large, zero, and host-varying block sizes.
- Parameter automation must not click, zipper, explode, or produce hidden discontinuities.
- Verify Dry/Wet endpoints, bypass, mono fold-down, correlation, channel energy, true peak risk, DC offset, silence noise, and deterministic duplicate renders.
- For nonlinear processing, measure alias energy, THD+N, IMD, gain dependence, and oversampling tradeoffs. Do not add oversampling to a linear path.
- For linear time-varying processing, inspect modulation sidebands and pitch coloration.
- For filters/crossovers, inspect magnitude, phase, group delay, recombination, coefficient transitions, and reset behavior.
- For delays, inspect interpolation error, feedback stability if present, transient response, and sample-rate scaling.
- Never hide a regression with output trim, Auto Gain, or a wider test tolerance.
- Factory presets must be level-aware, musically distinct, complete-state, and tested not to inherit previous hidden values.
