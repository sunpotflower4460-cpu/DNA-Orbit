# Independent review: user-centred editor architecture

- Review ID: REV-20260802-user-centered-ui
- Date: 2026-08-02
- Reviewer role: quality governor / UI UX / validation synthesis
- Reviewer identity / agent: ChatGPT source review; not observed-user or owner approval
- Change / PR: CHG-20260802-user-centered-ui / PR #2
- Commit reviewed: evolving draft branch
- Risk tier: R3
- Owner approval: PENDING

## Review scope

Reviewed information architecture, Basic/Detail grouping, persistent preset and Bypass access, responsive layout logic, semantic colour system, control styling, warnings, diagnostics, accessibility intent, screenshot matrix, and manual validation plan. Compiled rendering and real interaction remain unverified.

## Constitutional alignment

- Pillars strengthened: P1, P2, P7, P8, P9.
- Pillars at risk: P7 if labels/cards add density; P8 if animation or status overlays do not match compiled telemetry.
- Product-identity drift: reduced by making Motion/Space/Output and centre state explicit.
- Whole-product versus local optimisation: redesign targets the first-use workflow, not visual novelty alone.

## Findings

### Blocker

- Do not mark UI complete until the editor compiles and the eight screenshots are generated and inspected.

### High

- Minimum-size Detail calculations may still reveal overlap or impractical control sizes only after rendering.
- Keyboard eligibility does not prove a logical focus order or DAW shortcut coexistence.
- Colour, font, popup, and custom LookAndFeel APIs require compiled platform checks.
- No first-time target user has completed the intended workflow without coaching.

### Medium

- English technical badges and Japanese labels should be judged together for hierarchy and localisation consistency.
- The persistent header may become tight at minimum size; screenshot evidence is decisive.
- Animated visualiser load must be measured separately from editor-closed DSP cost.

### Low

- Future reduced-motion preference may be useful if user testing indicates need.

### Questions

- Whether the final visual tone feels sufficiently distinctive without becoming decorative.
- Whether technical diagnostics should remain visible by default on Detail after user observation.

## Evidence examined

PluginEditor, LookAndFeel, visualiser code, screenshot harness, UI/UX standard, ADR-009, manual gates, and PR description.

## Adversarial cases

- Bypass appears global but is clipped at minimum width;
- disabled Speed lacks a visible explanation in one state;
- a warning sits behind the child visualiser;
- custom focus/painting behaves differently in a real host;
- attractive cards reduce usable visualiser area;
- first-time users misread centre status or Character;
- high DPI exposes font fallback or clipped popup text.

## Decision recommendation

REVISE / HOLD IN DRAFT until compiled renders, keyboard/DPI/host tests, performance measurement, and observed first-use evidence are complete.

## Resolution verification

Pending exact reviewed commit, eight images, issue list and fixes, keyboard order, DPI/host results, observed-user notes, and owner visual review.
