---
paths:
  - "Source/Parameters.h"
  - "Source/Presets.h"
  - "Source/PluginProcessor.{h,cpp}"
  - "Tests/*State*.cpp"
  - "Tests/*Preset*.cpp"
---

# Parameter and saved-state compatibility

- Parameter IDs, parameter version hints, manufacturer code, plug-in code, bundle ID, and state root names are external contracts.
- Never rename or reuse an existing parameter ID for different semantics.
- New persistent behavior requires a schema version decision and migration test.
- Older projects must load to values that preserve their previous sound, not merely the new product default.
- Fresh-instance defaults and legacy-migration defaults may intentionally differ; document both.
- Missing, malformed, future-version, and non-finite state must fail safely.
- Factory presets must set every audible parameter and every mode that can change interpretation.
- UI-only state must remain separated from host-automatable audio state.
- Round-trip state tests must include all parameters, schema property, preset application, and old-schema fixtures.
- Do not remove migration code until a documented major-version policy explicitly permits breaking old sessions.
