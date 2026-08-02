---
paths:
  - "Tests/**/*"
  - "scripts/**/*"
  - "CMakeLists.txt"
  - "Tests/CMakeLists.txt"
---

# Tests and validation rules

- A test must state the invariant, stimulus, measurement, tolerance, and why that tolerance is physically/numerically justified.
- Prefer property, analytical, metamorphic, and endpoint tests over snapshots alone.
- Use deterministic seeds and print enough context to reproduce failures.
- Do not make tests pass by weakening assertions without a documented error budget.
- Separate exact tests (`0` tolerance where representable), numerical-error tests, psychoacoustic proxies, performance measurements, and manual listening gates.
- Include warm-up where recursive filters, delays, smoothers, or correlation estimates require settling.
- Avoid testing only a single block size, sample rate, channel relationship, or initial phase.
- Tests must not depend on wall-clock scheduling unless explicitly testing performance; performance tests report data and should not use fragile universal pass thresholds.
- Every bug fix gets a regression test that fails for the actual root cause.
- Every new mode gets state round-trip, preset-completeness, automation, reset, and extreme-value coverage as applicable.
- Keep automated pass/fail separate from `MANUAL_REQUIRED.md`; do not mark listening or DAW work complete from unit tests.
- Local validation scripts must stop on failure, preserve logs, avoid destructive cleanup, and work from any current directory by resolving the repository root.
