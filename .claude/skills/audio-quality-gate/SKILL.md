---
name: audio-quality-gate
description: Design or review tests and listening for DNA Orbit sound quality, transparency, stereo behavior, modulation, filters, delays, mix law, bypass, presets, and CPU tradeoffs. Use before claiming any DSP change improves sound quality.
---

# Audio quality gate

Read `docs/claude-code/AUDIO_QUALITY_STANDARD.md` and the relevant implementation/tests.

## 1. Define the audible claim

Replace vague language such as “better,” “cleaner,” “more analog,” “wider,” or “more physical” with a falsifiable claim. Examples:

- lower modulation sidebands above 10 kHz;
- flatter crossover recombination within a stated error;
- less level bias across the Dry/Wet sweep;
- improved mono retention of bass;
- fewer clicks during automated delay changes;
- more repeatable spatial trajectory at identical PPQ.

## 2. Protect exact contracts first

Check exact Dry, Soft Bypass, Bass Anchor Off, state restore, preset completeness, silence, and finite output before subjective tuning.

## 3. Build the signal test matrix

Choose relevant stimuli from:

- impulse and impulse train;
- logarithmic sine sweep;
- single tones at low, crossover, mid, high, and near-Nyquist frequencies;
- CCIF/SMPTE-style multitone or equivalent intermodulation stimuli;
- white and pink noise;
- DC and silence;
- mono, correlated stereo, hard-panned, decorrelated, and pure Side/anti-phase material;
- transient drums, bass, vocal, acoustic guitar, distorted guitar, piano, pad, lead synth, and full mix.

Run applicable tests across sample rates, block sizes, phases, automation directions, and parameter extremes.

## 4. Measure

Select metrics appropriate to the algorithm:

- null residual and endpoint identity;
- magnitude, phase, group delay, impulse response, and recombination;
- L/R energy, Mid/Side energy, correlation, mono fold-down, and centroid;
- RMS/LUFS-style level, crest factor, sample peak, true-peak risk, and DC;
- THD+N, IMD, alias energy, and noise for nonlinear paths;
- modulation sidebands and pitch error for time-varying paths;
- duplicate-render difference;
- CPU ns/sample/instance and memory.

Record tool, version, sample rate, block size, stimulus, parameters, and tolerance.

## 5. Listen fairly

- Match loudness within 0.1 dB where possible.
- Randomize A/B when practical and include a hidden reference for important choices.
- Use short loops for focused artifacts and full musical passages for context.
- Judge movement, centre stability, low-end focus, depth, width, tone, transient integrity, fatigue, mono compatibility, and automation smoothness separately.
- Do not tune only on one source, monitor, headphone, or playback level.

## 6. Review CPU tradeoffs

Do not accept a quality increase that causes unexplained CPU spikes, unstable high-rate behavior, or unbounded instance scaling. Do not accept a CPU reduction that changes sound without an error budget and listening evidence.

## Output format

1. Claim being tested.
2. Exact contracts status.
3. Automated measurements and results.
4. Listening protocol and results.
5. CPU/memory impact.
6. Regressions or uncertainties.
7. Verdict: reject / iterate / accept as experimental / accept for release candidate.
8. Next smallest evidence-producing action.

If listening or measurement was not run, say so explicitly and do not call the change higher quality.
