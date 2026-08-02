---
paths:
  - "Source/ui/**/*.{h,cpp}"
  - "Source/PluginEditor.{h,cpp}"
  - "Tools/RenderShots.cpp"
---

# UI, UX, and visual-truth rules

Read `docs/claude-code/UI_UX_STANDARD.md` before non-trivial editor work.

- Start from the musician's shortest path to a useful sound, not the list of available parameters.
- Basic view prioritizes musical intent. Advanced view exposes technical control without obscuring dependencies.
- Preset selection and Soft Bypass are global actions and must not be hidden behind Detail.
- If Sync disables Speed, the cause and controlling Division must remain visible on Basic.
- Group Detail controls by user mental model: Motion, Space, and Output unless evidence supports a better structure.
- The visualiser must represent published DSP state, not run an independent decorative simulation presented as truth.
- If UI smoothing differs from audio smoothing, document the difference and never imply sample accuracy.
- A transport seek, loop, reset, schema migration, or mode discontinuity must not draw a misleading continuous history.
- Disabled or ineffective parameters must be visibly and accessibly disabled, with a clear nearby reason or tooltip.
- Technical diagnostics belong in Detail unless the value is required for a Basic musical decision.
- Modified preset state must be visible and reversible without destroying unrelated work silently.
- Keyboard navigation, readable names, double-click reset, resize limits, DPI behavior, reduced-motion needs, contrast, and Japanese text rendering are release requirements.
- UI timers stop or reduce work when hidden. Paint code must avoid avoidable allocation and unbounded geometry growth.
- UI actions must communicate through parameters, attachments, or lock-free telemetry; never call UI code from the audio thread.
- Warning states such as NULL CORE must track the actual parameter under automation and preset loading.
- Colour has semantic roles; do not add colour only for decoration. Warnings cannot depend on colour alone.
- Minimum-size controls must remain practical mouse targets with readable values; presence alone is not usability.
- Required screenshots cover 820x650, 960x700, and 1440x900, Basic/Detail, Sync, NULL CORE, and Soft Bypass.
- Screenshots are evidence of layout, not evidence of usability, accessibility, or audio behavior.
- Do not call the UI polished or world-class until it has been keyboard-tested, host-tested, resized repeatedly, and observed in real use.
