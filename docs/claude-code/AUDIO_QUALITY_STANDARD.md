# DNA Orbit Audio Quality Standard

## 1. Definition of quality

Audio quality is the degree to which the plug-in produces the intended musical result while avoiding unintended artifacts, preserving contracts, remaining stable across hosts, and using acceptable CPU/latency.

Quality is multidimensional:

- correctness;
- transparency where transparency is intended;
- musical character where character is intended;
- stereo and mono behavior;
- automation smoothness;
- deterministic behavior;
- CPU and memory efficiency;
- usability and truthful controls.

No single metric proves overall quality.

## 2. Exact contracts

These receive the strongest tests because they are not subjective tuning targets.

### Dry and bypass

- Mix 0% produces exact Dry unless a documented latency architecture makes that impossible.
- Fully engaged Soft Bypass reaches exact Dry after the defined transition.
- Host bypass returns exact Dry while internal time-dependent state continues correctly.
- Output trim must not alter fully bypassed Dry unless explicitly specified.

### Compatibility paths

- Bass Anchor 20 Hz / Off preserves the documented transparent split path.
- Old state schemas migrate to values that preserve old sound.
- Factory presets set complete state.

### Safety

- output is finite and bounded for all finite/non-finite test inputs;
- silence remains silent apart from explicitly documented tails/noise;
- no persistent DC is introduced without design intent;
- no denormal CPU spike;
- no unannounced latency.

Exact contracts must not be traded for “better sound.”

## 3. Gain staging and loudness

### Requirements

- Keep internal headroom for coherent summation of strands, Core, low anchor, and Side bed.
- Do not use hard clipping as an undocumented safety mechanism.
- Auto Gain must be bounded and separated conceptually from artistic output trim.
- Measure sample peak and consider true-peak risk for time-varying/filtering paths.
- Dry/Wet comparisons must not be biased by a persistent loudness increase.

### Measurements

Record as appropriate:

- RMS or equivalent energy;
- loudness using a documented implementation when program material is used;
- sample peak and true-peak estimate;
- crest factor;
- gain through Mix sweep;
- gain by orbit position and Character;
- gain with correlated and anti-correlated channels.

A loudness standard is a measurement framework, not an automatic artistic target.

## 4. Linear transfer quality

For linear or locally linear paths measure:

- impulse response;
- magnitude response;
- phase response;
- group delay;
- channel-to-channel matching;
- crossover recombination;
- delay response at representative fractional offsets;
- reset transient;
- coefficient-transition transient.

Report frequency axis, smoothing, window, FFT length, sample rate, level, and steady-state/warm-up.

## 5. Time-varying quality

Orbit modulation, moving delays, changing filters, transport corrections, and automation are time-varying.

Check:

- clicks at parameter/mode/transport boundaries;
- zipper noise;
- modulation sidebands;
- unintended pitch modulation;
- transient smearing;
- stereo image discontinuity;
- difference between realtime and offline rendering;
- dependence on block boundaries.

Use steady tones and multitone signals to reveal sidebands, then confirm musical relevance through listening.

## 6. Nonlinear quality

DNA Orbit should not add nonlinear processing casually. If saturation, limiting, clipping, dynamics, or nonlinear spatial behavior is introduced, require:

- transfer curve;
- level-dependent frequency response;
- THD+N;
- IMD;
- alias spectrum;
- DC generation;
- oversampling comparison;
- latency and CPU;
- automation/state behavior;
- level-matched listening.

Oversampling is justified by measured alias reduction and musical benefit, not by marketing convention. Do not oversample a purely linear path.

## 7. Stereo and spatial quality

### Required channel relationships

Test:

- mono duplicated to stereo;
- identical correlated stereo;
- hard left and hard right;
- unequal L/R levels;
- decorrelated stereo;
- pure Mid;
- pure Side / exact anti-phase;
- partially negative correlation;
- polarity inversion on one channel;
- mono fold-down.

### Required observations

- L/R output energy;
- Mid and Side energy;
- correlation;
- centroid/pan behavior;
- bass-centre stability;
- width retention;
- mono cancellation;
- channel symmetry under mirrored input/parameters;
- behavior at Symmetry 100% and lower values.

Do not optimize only for correlation. Negative correlation can be intentional; it must be understood and warned where mono risk is high.

## 8. Bass Anchor quality

Evaluate at Off, 60, 80, 120, 180, 250, and 500 Hz as relevant.

Measure/listen for:

- low-frequency Side suppression;
- low-band mono recombination;
- crossover magnitude and phase;
- transient integrity of kick/bass;
- low-frequency image stability;
- high-band width preservation;
- automation artifacts;
- Character interaction;
- sample-rate invariance.

The product goal is not “maximum mono bass.” It is controllable low-frequency stability without hollowing or narrowing the rest of the source.

## 9. Delay and interpolation quality

For each interpolation strategy and delay range:

- static fractional-delay magnitude/phase error;
- impulse shape and pre/post ringing;
- sweep response;
- modulated-delay sidebands;
- rapid automation behavior;
- near-zero and maximum delay;
- high-frequency error at each sample rate;
- CPU per sample.

A higher interpolation order is not automatically more musical or more stable.

## 10. Filter and Character quality

For Natural, Vivid, and Deep:

- measure front and rear transfer functions;
- compare gain, cutoff, phase, and delay contribution;
- verify interpolation between modes remains finite and smooth;
- level-match before preference listening;
- test vocal consonants, cymbals, distorted guitar, pads, and full mixes;
- confirm the modes are distinct without one winning only by loudness.

Character should express intentional perspective, not hidden quality tiers.

## 11. Sample-rate matrix

Minimum technical coverage:

- 44.1 kHz;
- 48 kHz;
- 88.2 kHz;
- 96 kHz;
- 176.4 kHz;
- 192 kHz.

At high sample rates inspect:

- delay storage and scaling;
- coefficient ranges;
- CPU scaling;
- denormal behavior;
- smoothing times in seconds;
- near-Nyquist test frequency selection.

At low/typical rates inspect audible high-frequency interpolation and filter error.

## 12. Block-size matrix

At minimum:

- 0 samples where host/API may call it;
- 1 sample;
- 16/32;
- 64/128;
- 256/512;
- 1024/2048;
- changing sizes between calls;
- size up to prepared maximum;
- unexpected larger block handled safely or explicitly guarded.

Block size must not change musical timing, smoothing duration, or Host Lock geometry beyond defined numerical error.

## 13. Automation matrix

For every changed parameter:

- minimum to maximum and maximum to minimum;
- slow and abrupt changes;
- repeated toggles;
- changes while stopped and playing;
- changes during host bypass and Soft Bypass;
- changes at loop/seek boundaries;
- simultaneous changes with related controls;
- state save during transition and restore.

Listen and inspect waveforms/spectra for discontinuity.

## 14. Stimulus corpus

### Synthetic

- silence;
- DC;
- impulse and impulse train;
- low-frequency sine;
- crossover-frequency sine;
- 1 kHz reference;
- high-frequency and near-Nyquist sine;
- logarithmic sweep;
- white and pink noise;
- multitone/intermodulation stimulus;
- mono, Mid, Side, anti-phase and hard-pan variants.

### Musical

Maintain a legally usable local corpus containing representative excerpts:

- male and female vocal;
- spoken voice;
- acoustic guitar;
- clean and distorted electric guitar;
- bass with sub content;
- kick/snare/overheads and drum bus;
- piano;
- bright synth lead;
- wide pad;
- dense full mix;
- sparse ambient mix.

Do not commit copyrighted commercial recordings unless licensed. Store hashes/metadata and document local paths separately.

## 15. Listening protocol

### Level matching

Match candidates within 0.1 dB where possible. Recheck after every algorithm or preset adjustment.

### Bias reduction

- randomize A/B order;
- include hidden reference where practical;
- avoid seeing implementation names during critical comparisons;
- repeat on different days for subtle decisions;
- distinguish detection from preference.

### Attributes

Rate independently:

- centre stability;
- width;
- depth/front-back impression;
- motion continuity;
- tonal integrity;
- transient integrity;
- low-end focus;
- mono compatibility;
- fatigue/harshness;
- musical usefulness;
- artifact audibility.

### Monitoring

Use at least:

- trusted headphones;
- stereo monitors in a known position;
- mono check;
- low playback level;
- typical working level.

Document model/room only to the degree useful for reproduction.

## 16. Determinism

For deterministic modes:

- render twice from identical project state and position;
- compare sample residual;
- test different offline block sizes if the host permits;
- reopen project and repeat;
- compare after host bypass/un-bypass;
- compare looped versus direct start at the same PPQ.

State clearly whether the guarantee is mathematical, same-build numerical, or cross-platform.

## 17. CPU and memory

Measure:

- ns/sample/instance;
- percentage of real-time budget;
- 1 and 10 instances;
- supported sample rates and representative block sizes;
- Symmetry locked and drifting;
- Bass Anchor Off and active;
- visualiser open and closed separately;
- automation stress;
- debug/sanitizer separately from Release.

Do not use one machine’s percentage as a universal threshold. Track regressions on the same machine and power mode.

## 18. Acceptance categories

### Exact pass

All exact contracts pass at defined zero/numerical tolerance.

### Technical pass

Measurements satisfy predeclared error budgets and no blocker/high defect remains.

### Perceptual pass

Level-matched listening supports the intended result across representative material with documented tradeoffs.

### Host pass

Named DAWs/formats pass scan, load, automate, restore, bypass, render, and unload.

### Release pass

All applicable automated, perceptual, host, legal, signing, packaging, and clean-machine gates pass.

## 19. Forbidden quality claims

Do not say:

- “better quality” without comparison;
- “transparent” without scope and null/error evidence;
- “analog” without defined behavior;
- “no artifacts” after only listening one source;
- “CPU optimized” without before/after measurement;
- “mono safe” without fold-down tests;
- “physically accurate” for psychoacoustic cues;
- “release ready” while manual gates remain.

## 20. Decision principle

The best DNA Orbit algorithm is the one that:

- preserves the product’s unique double-helix musical identity;
- tells the truth about its model;
- satisfies exact contracts;
- produces fewer unintended artifacts;
- wins fair musical comparison;
- remains stable and efficient in real hosts;
- has evidence that future agents can reproduce.
