# DNA Orbit UI / UX Standard

## 1. Product principle

The interface exists to help a musician reach a useful sound quickly and understand what the plug-in is doing. Visual novelty, animation, and technical density are secondary.

A change is not an improvement merely because it is more detailed, more colourful, or more visually complex.

## 2. Primary user journey

A first-time user should be able to:

1. load DNA Orbit;
2. choose a relevant preset or remain on the default;
3. adjust Width, Depth, Mix, and Speed;
4. understand whether tempo sync controls Speed;
5. compare processed and dry sound with a globally visible bypass;
6. reach a musically useful result without opening the Detail page.

The Basic page therefore exposes only the highest-value controls plus Sync, Division, and level matching.

## 3. Information architecture

### Global header

Always available:

- product identity;
- Basic / Detail navigation;
- preset selection;
- modified-preset state and revert action;
- Soft Bypass.

### Basic page

Prioritise musical intent:

- Speed;
- Width;
- Depth;
- Mix;
- tempo Sync and Division;
- level matching.

Do not hide the cause of a disabled Basic control. If Sync disables Speed, Sync and Division must remain visible.

### Detail page

Group by the user's mental model rather than parameter implementation:

- **Motion** — Symmetry, Twist, Start Position, Sync, Division, Phase Mode, Direction;
- **Space** — Core, Stereo Preserve, Bass Anchor, Character;
- **Output** — Output level, level matching, NULL CORE.

## 4. Visual hierarchy

Use colour only for meaning:

- cyan: primary orbit / primary interaction;
- green: secondary strand / enabled assistance;
- white: stable centre;
- orange: drift or modified state;
- red: bypass or destructive/experimental warning.

Use brightness, spacing, border weight, typography, and grouping for hierarchy before adding more colours.

The visualiser is the focal object. Controls must support it without competing with it.

## 5. Control language

- Prefer musician language over implementation language.
- Use a short primary label and a short explanatory hint.
- Tooltips explain consequence and dependency, not merely repeat the label.
- Use `Off` where a minimum value is a literal bypass.
- Disabled controls must have a visible reason in nearby guidance or tooltip.
- Avoid unexplained abbreviations except common audio conventions or where a tooltip resolves them.

## 6. Responsive layout

Required screenshot sizes:

- 820 x 650 minimum;
- 960 x 700 standard;
- 1440 x 900 wide.

At every size:

- no overlap;
- no clipped text;
- no control smaller than a practical mouse target;
- no parameter value hidden by its label;
- no card with unusable remaining content area;
- the visualiser retains meaningful space;
- warnings remain visible without covering essential controls.

The Detail page may change from one row to two rows. It must not merely shrink seven knobs until they are technically present but practically unusable.

## 7. Interaction requirements

- keyboard focus on all interactive controls;
- logical focus order matching the visual hierarchy;
- double-click returns continuous parameters to defaults;
- parameter automation updates warnings and disabled states;
- global bypass remains reachable from both pages;
- presets define complete state;
- modified preset state is visible and reversible;
- tooltips appear quickly enough to help without obstructing dragging;
- hidden editors stop or reduce animation work.

## 8. Visual truth

The 3D view may be beautiful, but it must represent published DSP telemetry. Decorative motion must never be presented as the physical or signal state.

Technical diagnostics belong in Detail. Basic should communicate only the information needed for musical decisions.

## 9. Accessibility

Release requirements:

- Japanese-capable fonts;
- meaningful component names;
- readable disabled states;
- sufficient text/background and state contrast;
- keyboard access;
- no warning conveyed by colour alone;
- no essential state conveyed only by animation;
- DAW shortcut coexistence checked manually;
- scaling and DPI tested in real hosts.

## 10. Evidence

Before UI work is called complete:

1. generate the full screenshot matrix;
2. inspect it at 100% scale and reduced preview size;
3. test keyboard traversal;
4. test preset modification/revert;
5. test Sync, NULL CORE, and Soft Bypass states;
6. resize repeatedly between minimum, standard, and wide;
7. profile animation and hidden-editor behaviour;
8. validate in at least LUNA/Cubase plus another host where available;
9. record unresolved subjective decisions rather than silently choosing them.

Screenshots prove layout only. They do not prove usability, accessibility, or audio behaviour.
