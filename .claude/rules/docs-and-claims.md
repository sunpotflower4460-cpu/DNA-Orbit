---
paths:
  - "README.md"
  - "MANUAL_REQUIRED.md"
  - "docs/**/*.md"
  - "docs/**/*.json"
---

# Documentation and product-claim rules

- Documentation must describe the current branch, not an intended future implementation.
- Use the truth labels EXACT, NUMERICAL APPROXIMATION, PSYCHOACOUSTIC MODEL, ARTISTIC EXTENSION, and UNKNOWN / UNVALIDATED where ambiguity is possible.
- Never write “physically accurate,” “transparent,” “zero latency,” “bit-perfect,” “world-class,” “release-ready,” or similar claims without a defined scope and evidence.
- Distinguish implemented, compiled, tested, measured, listened, host-validated, and release-ready.
- Equations must define symbols, units, assumptions, coordinate conventions, and discrete-time mapping.
- Update ADRs when a decision changes. Do not silently rewrite the rationale to make history look cleaner.
- Keep `MANUAL_REQUIRED.md` as the source of truth for work that cannot be automated.
- Reference primary sources for APIs, standards, and physical models. Mark external facts that may change with version/date.
- Do not copy long copyrighted passages; summarize and link.
- Keep the commercial-upgrade manifest aligned with implemented workstreams and remaining gates.
