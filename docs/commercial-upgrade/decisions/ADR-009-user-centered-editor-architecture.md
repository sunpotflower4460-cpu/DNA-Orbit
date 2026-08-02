# ADR-009: User-centred editor information architecture

## Status

Accepted for draft implementation. Screenshot, keyboard, real-host, and observed-user validation remain required.

## Context

The previous editor exposed a Basic and Detail page but had several workflow problems:

- Basic could show a disabled Speed control while hiding the Sync control that caused it;
- Soft Bypass was hidden in Detail even though comparison is a global action;
- Detail placed seven knobs in one row, producing small controls at minimum width;
- technical A/B and centroid readouts competed with the musical interface at all times;
- controls were grouped mainly by implementation location rather than the musician's task;
- the visual palette and stock controls did not yet form a consistent product system.

## Decision

### Persistent header

Always expose:

- product identity;
- Basic / Detail navigation;
- preset selection;
- modified-preset state and revert;
- Soft Bypass.

### Basic page

Expose only:

- Speed;
- Width;
- Depth;
- Mix;
- Sync;
- Division;
- level matching.

Sync remains visible because it controls whether Speed is effective.

### Detail page

Group controls as:

- **Motion** — Symmetry, Twist, Start Position, Sync, Division, Phase Mode, Direction;
- **Space** — Core, Stereo Preserve, Bass Anchor, Character;
- **Output** — Output level, level matching, NULL CORE.

At widths below 1120 px, Detail changes to two rows rather than shrinking every control into one row.

### Diagnostics

- centre lock remains visible because it explains the product's core behaviour;
- detailed pan, centroid, correlation, and Bass Anchor readout appears only in Detail;
- NULL CORE uses a visible text warning, not colour alone.

### Visual system

- cyan: primary orbit and interaction;
- green: secondary strand and helpful active state;
- white: stable centre;
- orange: drift and modified state;
- red: bypass or experimental/destructive warning;
- surfaces and typography create hierarchy before additional colour.

### Validation matrix

Generate screenshots for minimum, standard, and wide editor sizes and for Sync, NULL CORE, and Soft Bypass states.

## Consequences

Positive:

- first-time workflow is shorter;
- hidden dependencies are reduced;
- bypass and preset actions remain reachable;
- Detail becomes understandable by intent;
- controls stay larger at minimum size;
- visual hierarchy is more consistent.

Tradeoffs:

- minimum editor height increases to 650 px;
- Detail uses more vertical space in compact mode;
- the header contains more persistent actions;
- exact visual quality still requires rendered screenshots and real-host observation.

## Rejected alternatives

### Keep every control in one horizontal row

Rejected because it optimises for code simplicity rather than control usability.

### Hide Sync and Auto Gain from Basic

Rejected because users need to understand disabled Speed and make level-fair comparisons without entering Detail.

### Put Soft Bypass only in Detail

Rejected because bypass is a global comparison action.

### Show all diagnostics on Basic

Rejected because technical telemetry competes with the first musical decision path.

## Required follow-up

- compile and render the expanded screenshot matrix;
- inspect Japanese text, values, and control bounds;
- verify keyboard traversal and DAW shortcut coexistence;
- test DPI and host scaling;
- observe at least one first-time user completing the primary journey;
- retune spacing or wording from evidence, not aesthetic preference alone.
