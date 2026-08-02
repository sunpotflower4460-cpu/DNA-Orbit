---
name: ui-ux-audit
description: Audit and improve DNA Orbit UI/UX for clarity, hierarchy, responsiveness, accessibility, visual truth, and real musician workflow. Use for editor layout, LookAndFeel, control grouping, labels, warnings, presets, navigation, screenshots, and visualizer presentation.
---

# UI / UX Audit

## Required references

Read:

- `docs/claude-code/UI_UX_STANDARD.md`
- `.claude/rules/ui-visual-truth.md`
- `docs/claude-code/CURRENT_ARCHITECTURE_MAP.md`
- relevant parameter dependencies and UI state code

## 1. Establish the user journey

State:

- primary user and task;
- first useful result;
- controls required before Detail;
- global actions that must never be hidden;
- dangerous or experimental states;
- expected minimum, standard, and wide sizes.

## 2. Audit the current interface

Check:

- visual hierarchy and first eye target;
- control grouping by musician intent;
- hidden dependencies;
- disabled-state explanation;
- preset selection, modification, and revert;
- bypass reachability;
- label and unit clarity;
- diagnostics density;
- minimum-size control usability;
- keyboard focus and accessibility names;
- visualizer truth and animation cost.

Report findings by severity:

- blocker;
- high friction;
- visual inconsistency;
- polish opportunity.

## 3. Design before coding

Define:

- information architecture;
- page or group structure;
- persistent header actions;
- responsive breakpoints;
- colour semantics;
- typography hierarchy;
- warning presentation;
- screenshot matrix.

Do not add a control merely because a parameter exists.

## 4. Implement safely

- keep DSP and parameter IDs unchanged unless explicitly in scope;
- use APVTS attachments;
- track automation and preset changes;
- avoid message-thread work that scales without bounds;
- keep visualizer state derived from DSP telemetry;
- preserve Japanese UTF-8 handling;
- retain practical mouse targets and value readability.

## 5. Verify

Run or prepare:

```sh
bash scripts/validate-local.sh
```

Inspect at least:

- Basic minimum free;
- Basic minimum Sync;
- Detail minimum;
- Basic standard;
- Detail standard;
- Detail wide;
- NULL CORE warning;
- Soft Bypass active.

Then manually test:

- tab and keyboard traversal;
- parameter reset;
- preset modified/revert;
- rapid resizing;
- host scaling;
- DAW shortcut coexistence;
- tooltip usefulness;
- hidden editor CPU.

## 6. Completion report

Separate:

- implemented;
- screenshot-verified;
- keyboard-tested;
- host-validated;
- user-listened/observed;
- remaining subjective decisions.

Never call a UI world-class based only on source review or screenshots.
