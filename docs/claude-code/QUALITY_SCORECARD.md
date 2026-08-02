# DNA Orbit Quality Scorecard

Use this scorecard for substantial PRs and before changing a PR from draft. Scores support discussion; they do not override the Product Constitution, policy gates, Blockers, or unresolved High findings.

## Scoring

- **0 — absent / failing**
- **1 — major gaps**
- **2 — partial / experimental**
- **3 — solid release-candidate evidence**
- **4 — strong multi-condition evidence**
- **5 — exceptional, reproducible, independently reviewed evidence**

Any constitutional conflict, Blocker, expired active exception, or unresolved release-relevant High finding means **NOT READY**, regardless of average.

## A. Product outcome

| Item | Score | Evidence / open issue |
|---|---:|---|
| Musical goal is explicit | | |
| Change preserves DNA Orbit identity | | |
| Default behavior is useful without technical setup | | |
| Complexity is justified by user value | | |
| Whole-product benefit exceeds local optimisation | | |
| Tradeoffs and rejection conditions are documented | | |

## B. Physical and mathematical fidelity

| Item | Score | Evidence / open issue |
|---|---:|---|
| Truth class is explicit | | |
| Equations, units and coordinates are defined | | |
| Assumptions and invalid regimes are recorded | | |
| Limiting cases and invariants pass | | |
| Discrete implementation matches reference | | |
| Product claims match actual model | | |
| Independent physics audit complete | | |

## C. Audio quality

| Item | Score | Evidence / open issue |
|---|---:|---|
| Exact endpoints and compatibility paths pass | | |
| Magnitude/phase/delay behavior measured | | |
| Stereo, Side and mono behavior measured | | |
| Automation/transport artifacts checked | | |
| Nonlinear/alias/noise metrics checked if applicable | | |
| Level-matched listening complete | | |
| Representative musical corpus used | | |
| Independent audio-quality audit complete | | |

## D. Real-time and host safety

| Item | Score | Evidence / open issue |
|---|---:|---|
| No allocation/locks/I/O/logging in callback | | |
| Work is bounded at extreme conditions | | |
| Non-finite and denormal behavior safe | | |
| Variable/zero/large block behavior safe | | |
| Prepare/reset/sample-rate transitions safe | | |
| Bypass preserves state and output contract | | |
| Host playhead use follows API contract | | |
| Independent realtime review complete | | |

## E. Compatibility and state

| Item | Score | Evidence / open issue |
|---|---:|---|
| Parameter IDs/version hints preserved | | |
| Schema decision and migration implemented | | |
| Old fixtures preserve old sound | | |
| New defaults are intentional | | |
| Presets set complete state | | |
| Malformed/future/missing state safe | | |
| Project restore verified in hosts | | |

## F. Automated validation

| Item | Score | Evidence / open issue |
|---|---:|---|
| Relevant regression/property tests exist | | |
| Full CTest passes | | |
| ASan/UBSan passes | | |
| Tests cover sample rates/block sizes/channels | | |
| Test tolerances have a documented basis | | |
| pluginval strictness 10 passes | | |
| VST3 Validator passes | | |
| `auval` passes | | |

## G. Performance

| Item | Score | Evidence / open issue |
|---|---:|---|
| Baseline captured on same machine | | |
| Release benchmark covers 44.1–192 kHz | | |
| Multiple block sizes covered | | |
| Multiple instances covered | | |
| Visualiser open/closed separated | | |
| Optimization quality regression checked | | |
| CPU claim has before/after evidence | | |

## H. UI and accessibility

| Item | Score | Evidence / open issue |
|---|---:|---|
| Basic view reflects musical intent | | |
| Dependencies and disabled states are clear | | |
| Visualiser reflects DSP truth | | |
| Resize/DPI/text clipping checked | | |
| Keyboard focus and resets work | | |
| Accessible names and warning states work | | |
| Hidden editor avoids waste | | |
| Screenshot matrix inspected | | |
| First-use workflow observed where applicable | | |

## I. Documentation and decision trail

| Item | Score | Evidence / open issue |
|---|---:|---|
| README reflects current branch | | |
| ADR captures rationale and alternatives | | |
| Assumptions register updated | | |
| Change record reflects actual current scope | | |
| Manual gates updated | | |
| Claims distinguish implemented/tested/listened | | |
| Primary references recorded | | |
| PR body is complete and honest | | |

## J. DAW and distribution

| Item | Score | Evidence / open issue |
|---|---:|---|
| Cubase/LUNA primary workflow passes | | |
| Additional named DAWs pass | | |
| VST3/AU/Standalone scan and use pass | | |
| Realtime/offline/freeze behavior passes | | |
| Clean-machine install passes | | |
| Signing/notarization passes | | |
| License obligations confirmed | | |
| Artifact hashes and versioning recorded | | |

## K. Constitution and governance

| Item | Score | Evidence / open issue |
|---|---:|---|
| Product Constitution version/hash verified | | |
| Affected pillars and constitutional fit are explicit | | |
| Risk tier matches worst credible consequence | | |
| Rejection and rollback conditions are concrete | | |
| Quality-ratchet items are named and preserved | | |
| Required ADR and independent review exist | | |
| Scope expansion updated the change contract | | |
| Exceptions are scoped, owned, approved, and unexpired | | |
| Quality debt has owner and objective exit condition | | |
| Development governance audit passes | | |
| Release governance audit passes where applicable | | |
| Product-owner approval is recorded where required | | |
| Release decision is recorded separately from merge | | |

## Blocker checklist

Any checked item blocks readiness:

- [ ] crash, hang, corruption or unbounded output;
- [ ] audio-thread allocation, lock or blocking operation;
- [ ] hearing-risk level event;
- [ ] old session changes sound without migration policy;
- [ ] parameter ID/state contract broken;
- [ ] false or materially misleading physical/product claim;
- [ ] exact Dry/bypass contract broken;
- [ ] deterministic mode is nondeterministic;
- [ ] required test/build/validator fails;
- [ ] known major artifact in normal use;
- [ ] unlicensed or unsigned distribution action;
- [ ] evidence claimed but not actually run;
- [ ] constitutional conflict or unapproved constitutional amendment;
- [ ] required R2–R4 change record missing or stale relative to actual scope;
- [ ] required R3/R4 ADR or independent review missing;
- [ ] expired active exception or exception outside its written scope;
- [ ] Blocker debt, or High debt inside shipping scope;
- [ ] established quality ratchet weakened without explicit regression decision;
- [ ] release inferred from merge/build rather than separately authorised.

## Verdict

- Current commit:
- Constitution version/hash:
- Change record:
- Risk tier proposed / reviewed:
- Reviewer(s):
- Governance mode/result:
- Product-owner approval status:
- Active exceptions:
- Active quality debt:
- Blockers:
- High findings:
- Scores by section:
- Automated status:
- Listening status:
- UI/UX status:
- Host status:
- Distribution status:
- Final decision: ACCEPT / ACCEPT WITH DEBT / EXPERIMENT ONLY / REVISE / REJECT / ROLL BACK
- Release decision: HOLD / eligible for owner decision / authorised
- Next action:
