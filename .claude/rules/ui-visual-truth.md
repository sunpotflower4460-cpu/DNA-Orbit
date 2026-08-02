---
paths:
  - "Source/ui/**/*.{h,cpp}"
  - "Source/PluginEditor.{h,cpp}"
---

# UI and visual-truth rules

- The visualiser must represent published DSP state, not run an independent decorative simulation presented as truth.
- If UI smoothing differs from audio smoothing, document the difference and never imply sample accuracy.
- A transport seek, loop, reset, schema migration, or mode discontinuity must not draw a misleading continuous history.
- Disabled or ineffective parameters must be visibly and accessibly disabled, with a clear reason in the tooltip or label.
- Keyboard navigation, readable names, double-click reset, resize limits, DPI behavior, reduced-motion needs, contrast, and Japanese text rendering are release requirements.
- UI timers stop or reduce work when hidden. Paint code must avoid avoidable allocation and unbounded geometry growth.
- UI actions must communicate through parameters, attachments, or lock-free telemetry; never call UI code from the audio thread.
- Warning states such as NULL CORE must track the actual parameter under automation and preset loading.
- Screenshots are evidence of layout, not evidence of audio behavior.
- Do not add controls merely because a DSP parameter exists. Basic view prioritizes musical intent; advanced view exposes technical control without obscuring dependencies.
