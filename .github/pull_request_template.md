## Governance identity

- Change record:
- Risk tier: R0 / R1 / R2 / R3 / R4
- Product Constitution version/hash:
- Affected constitutional pillars:
- Decision: PROPOSED / ACCEPT / ACCEPT WITH DEBT / EXPERIMENT ONLY / REVISE / REJECT / ROLL BACK
- Product-owner approval required: yes / no
- Product-owner approval status: not required / pending / approved / rejected

## Product outcome

<!-- What musical/user result should improve? Avoid describing only code mechanics. -->

## Constitutional fit and concept-drift test

<!-- Why is this DNA Orbit? Does it preserve the two-strand/centre relationship, physical honesty, sound quality, simplicity, and whole-product coherence? -->

- [ ] user value is clearer than implementation novelty
- [ ] change belongs in DNA Orbit rather than a generic effects rack
- [ ] default path remains understandable and useful
- [ ] visual/product language does not exceed DSP truth
- [ ] whole-product benefit outweighs added complexity
- [ ] rollback remains practical

## Current problem and root cause

<!-- Evidence, reproduction, and traced root cause. -->

## Truth class and model

- [ ] EXACT
- [ ] NUMERICAL APPROXIMATION
- [ ] PSYCHOACOUSTIC MODEL
- [ ] ARTISTIC EXTENSION
- [ ] UNKNOWN / UNVALIDATED

<!-- Define equations/state machine, symbols, units, coordinates, timing, assumptions, invalid regimes, and primary references where relevant. -->

## Rejection conditions

<!-- What result would make us reject, narrow to experiment-only, or roll back this change? -->

## What changed

<!-- Small coherent summary. Link ADR/experiment where applicable. -->

## Protected invariants and quality ratchet

- [ ] parameter IDs/product identifiers unchanged or migration explicitly approved
- [ ] old-session behavior preserved by tested schema migration
- [ ] exact Dry/bypass/compatibility paths preserved
- [ ] finite bounded output and reset behavior preserved
- [ ] audio thread remains allocation-free, lock-free, bounded, and free of I/O/logging/UI calls
- [ ] deterministic behavior preserved where promised
- [ ] UI/visual claims reflect actual DSP state
- [ ] established tests, validators, baselines, and user workflows were not silently weakened

<!-- Name every ratchet item affected. -->

## Audio-quality evidence

<!-- Exact residuals, response/phase/delay, stereo/mono, sidebands, nonlinear metrics if applicable, true peak, automation artifacts, etc. State N/A with reason. -->

## Listening evidence

- [ ] level matched within approximately 0.1 dB where applicable
- [ ] representative sources used
- [ ] hidden/randomized comparison used where decision importance warrants it
- [ ] mono and headphones/monitors checked as applicable

<!-- List material, monitoring, attributes, results, uncertainty. Do not invent or infer unrun listening. -->

## UI/UX and accessibility evidence

- [ ] required screenshot matrix generated and inspected
- [ ] minimum/standard/wide and DPI behavior checked
- [ ] keyboard traversal and activation checked
- [ ] dependencies, warnings, modified state, and bypass are understandable
- [ ] first-use workflow observed where importance warrants it

<!-- State N/A with reason for non-UI changes. -->

## Real-time and host safety

<!-- Callback call path, bounded work, variable block sizes, sample-rate/reset, bypass, transport, offline-render considerations. -->

## Compatibility and state

<!-- Schema/defaults/legacy fixtures/presets/project restore. -->

## Validation

Commands run:

```sh
bash scripts/agent-preflight.sh
bash scripts/quality-gate.sh
bash scripts/static-realtime-audit.sh
bash scripts/validate-local.sh
```

- [ ] governance development audit
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

- [ ] quality-governor review
- [ ] physics/numerical audit where applicable
- [ ] audio-quality audit where applicable
- [ ] real-time safety audit where applicable
- [ ] UI/UX audit where applicable
- [ ] validation architecture review
- [ ] adversarial review findings resolved or documented

Review records:

## Exceptions and quality debt

- Active exceptions:
- Exception owner/expiry/approval:
- Quality debt entries:
- Why debt is non-blocking:
- Exit condition:

- [ ] no Blocker debt
- [ ] no High debt in shipping scope
- [ ] no expired active exception

## Performance

<!-- Same-machine before/after ns/sample/instance, real-time %, sample rates, block sizes, instances, visualizer state. -->

## Completion state

- [ ] Designed
- [ ] Implemented
- [ ] Compiled
- [ ] Tested
- [ ] Measured
- [ ] Governance-audited
- [ ] Screenshot-verified
- [ ] Keyboard-tested
- [ ] Listened
- [ ] Host-validated
- [ ] Observed-user validated
- [ ] Owner-approved where required
- [ ] Release-ready

## Remaining blockers, manual gates, and risk

<!-- Blocker/High/Medium/Low. Include owner and next action. Keep draft while required gates remain. -->

## Rollback

<!-- Smallest safe rollback, feature-disable, compatibility fallback, or migration reversal. -->

## Release authorisation

Before shipping:

```sh
bash scripts/quality-gate.sh --release
```

- [ ] release-mode governance audit passed
- [ ] release decision recorded in `docs/quality/RELEASE_DECISION_LOG.md`
- [ ] exact commit and artifact hashes recorded
