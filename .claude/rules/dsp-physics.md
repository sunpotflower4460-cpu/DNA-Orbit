---
paths:
  - "Source/dsp/**/*.{h,cpp}"
  - "Source/PluginProcessor.{h,cpp}"
---

# DSP and physics rules

## Model before code

- Write the continuous-time or discrete-time reference equation before changing DSP behavior.
- Define coordinate system, sign convention, units, reference frame, sample timing, and parameter domain.
- Classify the behavior as EXACT, NUMERICAL APPROXIMATION, PSYCHOACOUSTIC MODEL, or ARTISTIC EXTENSION.
- Check dimensions. Never mix seconds, milliseconds, samples, Hz, radians, degrees, quarter notes, or PPQ without an explicit conversion boundary.
- List invariants and limiting cases: zero depth, zero radius, full symmetry, silence, DC, Nyquist-adjacent input, stopped transport, reverse direction, and sample-rate change.

## Physical honesty

- The current front/back model is psychoacoustic. Do not call gain darkening and microdelay a physical orbit renderer.
- A future physical propagation mode must separately model geometry, distance, propagation delay, attenuation, medium absorption, receiver/directivity behavior, and rendering topology.
- Pressure amplitude and intensity are different quantities. Do not apply an inverse-square law directly to pressure without deriving the intended model.
- Moving propagation delay can create Doppler/pitch modulation. Do not add or remove it accidentally; decide and test the intended behavior.
- Host PPQ is timeline position, not elapsed wall time. Treat transport loops, seeks, tempo changes, stops, and offline render explicitly.

## Numerical implementation

- Compare the implementation to a higher-precision or analytical reference where practical.
- Keep phase accumulators bounded without introducing discontinuities.
- Fractional delay interpolation must be evaluated for magnitude, phase, modulation sidebands, and high-frequency error.
- Time-varying filter/delay parameters require bounded smoothing or interpolation whose audible and numerical effect is tested.
- Coefficient changes must remain stable at every supported sample rate and extreme parameter value.
- Use `double` for long-running phase/time calculations where drift matters; converting to float for audio is acceptable only at a documented boundary.

## Real-time constraints

- No allocation, locks, logging, file/network access, UI calls, or unbounded work from the audio callback.
- Prepare all buffers and maximum delay storage before processing.
- Handle zero-length and variable-size blocks.
- Sanitize non-finite host/parameter/audio values before they reach recursive state.
- Keep UI telemetry lock-free and non-authoritative.

## Required evidence

For a DSP behavior change, add at least one analytical/property test and one signal-level regression test. Include sample-rate coverage where the algorithm’s behavior depends on sample rate.
