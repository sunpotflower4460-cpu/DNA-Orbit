## Product outcome

<!-- What musical/user result should improve? Avoid describing only code mechanics. -->

## Current problem and root cause

<!-- Evidence, reproduction, and traced root cause. -->

## Truth class and model

- [ ] EXACT
- [ ] NUMERICAL APPROXIMATION
- [ ] PSYCHOACOUSTIC MODEL
- [ ] ARTISTIC EXTENSION
- [ ] UNKNOWN / UNVALIDATED

<!-- Define equations/state machine, symbols, units, coordinates, timing, assumptions, and invalid regimes where relevant. -->

## What changed

<!-- Small coherent summary. Link ADR/experiment where applicable. -->

## Protected invariants

- [ ] parameter IDs/product identifiers unchanged or migration explicitly approved
- [ ] old-session behavior preserved by tested schema migration
- [ ] exact Dry/bypass/compatibility paths preserved
- [ ] finite bounded output and reset behavior preserved
- [ ] audio thread remains allocation-free, lock-free, bounded, and free of I/O/logging/UI calls
- [ ] deterministic behavior preserved where promised
- [ ] UI/visual claims reflect actual DSP state

## Audio-quality evidence

<!-- Exact residuals, response/phase/delay, stereo/mono, sidebands, nonlinear metrics if applicable, true peak, automation artifacts, etc. State N/A with reason. -->

## Listening evidence

- [ ] level matched within approximately 0.1 dB where applicable
- [ ] representative sources used
- [ ] hidden/randomized comparison used where decision importance warrants it
- [ ] mono and headphones/monitors checked as applicable

<!-- List material, monitoring, attributes, results, uncertainty. Do not invent or infer unrun listening. -->

## Real-time and host safety

<!-- Callback call path, bounded work, variable block sizes, sample-rate/reset, bypass, transport, offline-render considerations. -->

## Compatibility and state

<!-- Schema/defaults/legacy fixtures/presets/project restore. -->

## Validation

Commands run:

```sh
bash scripts/agent-preflight.sh
bash scripts/static-realtime-audit.sh
bash scripts/validate-local.sh
```

- [ ] Release build
- [ ] full CTest
- [ ] ASan/UBSan where supported
- [ ] screenshots generated and inspected
- [ ] DSP benchmark recorded
- [ ] pluginval strictness 10
- [ ] VST3 Validator
- [ ] `auval` on macOS
- [ ] named DAW/format tests
- [ ] duplicate realtime/offline render checks

<!-- Paste concise results, environment, logs/artifacts, and skipped reasons. -->

## Independent review

- [ ] physics/numerical audit
- [ ] audio-quality audit
- [ ] real-time safety audit
- [ ] validation architecture review
- [ ] adversarial review findings resolved or documented

## Performance

<!-- Same-machine before/after ns/sample/instance, real-time %, sample rates, block sizes, instances, visualizer state. -->

## Completion state

- [ ] Designed
- [ ] Implemented
- [ ] Compiled
- [ ] Tested
- [ ] Measured
- [ ] Listened
- [ ] Host-validated
- [ ] Release-ready

## Remaining blockers, manual gates, and risk

<!-- Blocker/High/Medium/Low. Include owner and next action. Keep draft while required gates remain. -->

## Rollback

<!-- Smallest safe rollback or feature-disable boundary. -->
