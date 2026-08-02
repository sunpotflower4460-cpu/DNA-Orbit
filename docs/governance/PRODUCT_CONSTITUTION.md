# DNA Orbit Product Constitution

**Constitution version:** 1.0.0  
**Status:** Normative  
**Change authority:** Product owner only, through the constitutional amendment process

## 1. Owner intent

DNA Orbit exists to give music a living spatial orbit while preserving a trustworthy centre, excellent sound, and immediate musical usefulness.

The product must continue toward the best achievable combination of:

- meaningful musical motion;
- physically and mathematically honest behavior;
- the highest practical audio quality;
- simple, humane operation;
- reliable use in real DAWs;
- long-term coherence rather than feature accumulation.

“Best” does not mean “most complex,” “most physical-looking,” “most animated,” or “most features.” It means the strongest user and musical result that satisfies the non-negotiable constraints below.

## 2. Product identity

DNA Orbit is:

- a musical spatial-motion effect built around two related orbit strands and a meaningful centre;
- a tool that lets a musician reach a useful result quickly, then deepen control when needed;
- a product where DSP, visualisation, wording, presets, and state behavior describe the same underlying system;
- a platform that may contain Musical, Physical, and Hybrid modes, provided their truth classes and boundaries remain explicit.

DNA Orbit is not:

- a generic effects rack with unrelated modules;
- a decorative visualiser disconnected from audio behavior;
- a claimed physical simulator when the implementation is psychoacoustic or artistic;
- a collection of technically impressive features without a clear musical purpose;
- a product that sacrifices old sessions, host stability, or hearing safety for novelty.

## 3. Constitutional pillars

### P1 — Musical purpose before mechanism

Every feature must solve a real musical or user problem. Internal sophistication is not user value by itself.

### P2 — Centre and relationship are foundational

The relationship between the two strands, their centre, and the user’s perception of stable versus drifting motion is core product identity. Changes may extend this idea but must not erase it accidentally.

### P3 — Physical and mathematical honesty

Every model and claim must be labelled as EXACT, NUMERICAL APPROXIMATION, PSYCHOACOUSTIC MODEL, ARTISTIC EXTENSION, or UNKNOWN / UNVALIDATED. A useful artistic model is acceptable; a false physical claim is not.

### P4 — Audio quality is judged by sound and evidence

No change is “higher quality” merely because it is newer or more complex. Exact contracts, measurements, level-matched listening, musical context, and artifact analysis decide whether quality improved.

### P5 — Safety and reliability are prerequisites

Hearing safety, bounded output, real-time safety, host stability, deterministic restore, and protection of user sessions are hard constraints, not tradeable quality dimensions.

### P6 — Compatibility is part of the sound

Parameter IDs, state schemas, presets, defaults, bypass behavior, and old-session sound are product contracts. A migration must be intentional, tested, documented, and reversible.

### P7 — Simplicity is a quality attribute

The default experience must remain understandable and musically useful. Advanced capability belongs behind coherent concepts, not exposed as uncontrolled parameter growth.

### P8 — Visual and verbal truth

The interface, animation, meters, documentation, and marketing language must reflect actual DSP state and model limits. Beauty supports comprehension; it must not create false certainty.

### P9 — Evidence outranks confidence

Agent confidence, expert intuition, and attractive demos do not replace reproducible evidence. Unknowns remain labelled unknown until tested.

### P10 — Evolution must remain reversible

High-risk changes require a rollback boundary, compatibility plan, and kill switch or safe fallback where feasible. Irreversible change needs explicit product-owner approval.

## 4. Non-negotiable invariants

The following may not be waived for convenience:

- no crash, hang, corruption, unbounded output, or hearing-risk level event;
- no allocation, lock, blocking I/O, logging, or UI call in the audio callback path;
- no NaN or infinity escaping the processor;
- exact Dry and documented bypass contracts remain exact;
- saved-session and parameter contracts are preserved or migrated deliberately;
- deterministic modes remain reproducible at identical state and timeline position;
- physical/product claims do not exceed the implemented model;
- visualisation presented as state is derived from published DSP telemetry;
- evidence that was not run is never reported as completed;
- unresolved Blocker findings prevent release.

## 5. Decision order

When values conflict, decide lexicographically:

1. protect hearing, files, sessions, and user data;
2. preserve real-time and host safety;
3. preserve compatibility and deterministic restore;
4. preserve truth and model validity;
5. maximize audible musical quality;
6. maximize immediate usability and accessibility;
7. preserve product coherence and simplicity;
8. improve performance using measured evidence;
9. improve visual polish and implementation elegance.

A lower item may not silently override a higher item.

## 6. The constitutional test

A substantial change must answer all of these:

1. What musician or user outcome becomes better?
2. Why is this DNA Orbit rather than an unrelated feature?
3. What truth class does it belong to?
4. Which protected invariants can it affect?
5. What evidence could falsify the proposed benefit?
6. What happens to old sessions and presets?
7. What complexity is added, and what complexity is removed?
8. How is the change disabled or rolled back?
9. What would make us reject it even after implementation?
10. Does it improve the whole product, or only a local metric?

An unanswered question is a design gap, not permission to assume.

## 7. Quality ratchet

Once a meaningful invariant, regression test, validator requirement, measurement baseline, or user-critical workflow is established, future work may strengthen it but may not silently weaken it.

A regression is allowed only when:

- the tradeoff is explicit;
- the higher-order product benefit is demonstrated;
- the product owner approves it when constitutional or user-contract behavior changes;
- an ADR, migration/rollback plan, and evidence record exist.

## 8. Amendment process

This constitution may not be edited as a side effect of feature work.

A constitutional amendment requires:

1. a dedicated change record classified `R4 / CONSTITUTIONAL`;
2. a dedicated ADR describing motivation, alternatives, affected pillars, and rejected options;
3. independent product, physics/audio, safety, compatibility, and UI review as applicable;
4. an explicit statement of what previous promise is changing;
5. migration and rollback plans;
6. explicit product-owner approval recorded in the review record;
7. an updated constitution version and SHA-256 lock;
8. successful governance audit in release mode.

When owner intent is ambiguous, preserve the existing constitution and record the question. Agents may propose amendments but may not self-authorise them.

## 9. Interpretation rule

The constitution governs the spirit of the product; tests and manifests govern specific enforceable contracts. If wording conflicts:

- the stricter safety or compatibility interpretation wins immediately;
- the ambiguity is recorded;
- a dedicated governance decision resolves it before release.

This document is intentionally stable. Detailed procedures belong in the governance system, ADRs, scoped rules, skills, tests, and evidence records.
