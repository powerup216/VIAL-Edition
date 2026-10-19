# A+ Trackball — custom Vial firmware

Ready-to-flash file: [`firmwares/aplus_vial.uf2`](../../firmwares/aplus_vial.uf2)

Keymap sheet: [`docs/img/keymap_vial.pdf`](../../docs/img/keymap_vial.pdf)

This is Ploopy's A+ firmware (from the `ploopyco/a+` branch of
[ploopyco/qmk_firmware](https://github.com/ploopyco/qmk_firmware/tree/ploopyco/a%2B))
ported to [vial-qmk](https://github.com/vial-kb/vial-qmk), with these changes:

| Change | Stock | This firmware |
|---|---|---|
| Wheels | Fixed functions | **Remappable per layer in Vial** (any key, or smooth scrolling) |
| Wheel defaults on the base layers | One scrolls, one horizontal-scrolls | **Volume instead of horizontal scroll**; smooth vertical scroll kept |
| Layers | 0 = right-handed (default), 1 = left-handed | **0 = left-handed (default, custom layout below)**, 1 = right-handed |
| Left-handed base layer LED | Green | **Red** |
| Control layer LED | Red | **Green** |
| Configurator | VIA (needs a JSON sideload) | **Vial** (auto-detected, no JSON needed) |
| Mouse gestures | Hard-coded | **Editable in Vial** (layer 3) |

Everything else (DPI cycling, drag scroll, LED brightness, mirror mode, OS-aware
shortcuts, settings saved across power cycles) works as on the stock firmware.

## Flashing

1. Unplug the A+.
2. Hold the bottom-left button (Mouse 1 on the default layout) and plug it in.
   A drive called `RPI-RP2` appears.
3. Drag `aplus_vial.uf2` onto it. It reboots by itself.

To go back to stock, flash `firmwares/aplus.uf2` the same way.

## Default layout

**Layer 0: left-handed base** (active at startup, red LED)

| | | | |
|---|---|---|---|
| Mouse 3 | F7 | Mouse 4 | Mouse 5 |
| Mouse 1 | | | Mouse 2 |
| Left wheel press: TG(2) (control layer) | | | Right wheel press: Gesture |
| Left wheel turn: Volume | | | Right wheel turn: Smooth vertical scroll |

**Layer 1: right-handed base** (blue LED), Ploopy's stock right-handed layout:
left wheel scrolls, right wheel is volume, left wheel press is Gesture, right
wheel press is the control layer. The control layer's **Mirror** button
switches between layer 0 and layer 1 (1 blink = left, 2 blinks = right), and
the choice is saved. Layer 0 is always on underneath layer 1, so a transparent
(▽) key on layer 1 falls through to layer 0.

Flashing this build resets the saved A+ settings once (handedness, scroll
modes, LED brightness), so it always starts on layer 0.

**Layer 2: control** (green LED), as in Ploopy's keymap PDF. Both wheels turn
the volume.

## Remapping the wheels

In Vial each wheel shows up as a pair of round keys under the layout:
**↺ counter-clockwise** and **↻ clockwise**, separately for every layer.
Clockwise is the direction that turns the volume up.

- **Any keycode** (volume, media, arrows, `MS_WHLU`/`MS_WHLD`, a macro, a layer
  key …) is tapped once per notch, with the same notch size as the stock volume
  wheel.
- **Wheel V-Scrl** / **Wheel H-Scrl** (User tab) give Ploopy's smooth, hi-res
  scrolling. Put it in both slots for normal direction. To reverse it, put it in
  only the ↺ slot.
- **▽ (transparent)** uses the wheel's setting from the layer below.
- A wheel is ignored while its own button is held down, as on the stock firmware.

The control layer's **Toggle Vertical/Horizontal Scroll Type** buttons still
switch the smooth-scroll wheels between scroll events and arrow keys (1 blink =
arrows, 2 blinks = scroll events).

## Using Vial

Open [vial.rocks](https://vial.rocks) in Chrome/Edge, or the Vial desktop app,
and plug in the A+. It is detected automatically.

- **Layer 0** — left-handed base (startup). **Layer 1** — right-handed base. **Layer 2** — control.
- **Layers 4–7** are free for your own use.
- A+ functions (gesture key, drag scroll, LED brightness, DPI, mode toggles,
  mirror, wheel scroll modes) are in the **User** tab.
- Flashing a new build resets the Vial keymap to the defaults built into it.
- **Unlocking** (needed only for the matrix tester or assigning `QK_BOOT`): hold
  the two big bottom buttons when Vial asks.

## Mouse gesture layer (layer 3)

Layer 3 is never switched on. It is a table: each of its 8 keys is what one
gesture direction sends. Hold the gesture key (right wheel press on layer 0),
move the ball in a direction, release.

The keys go clockwise around the trackball, starting at the top-left button:

| Vial key on layer 3 | Gesture | Default |
|---|---|---|
| Top-left outer | ↖ North-west | Undo (Ctrl+Z) |
| Top-left inner | ↑ North | Play/Pause |
| Top-right inner | ↗ North-east | ▽ Redo (Ctrl+Y / ⌘⇧Z on macOS) |
| Top-right outer | → East | ▽ Next virtual desktop |
| Bottom-right | ↘ South-east | Paste (Ctrl+V) |
| Right wheel | ↓ South | Copy (Ctrl+C) |
| Left wheel | ↙ South-west | Cut (Ctrl+X) |
| Bottom-left | ← West | ▽ Previous virtual desktop |

- **▽ (transparent) or empty** keeps the built-in OS-aware action (it detects
  Windows / Linux / macOS and sends the right shortcut).
- A gesture can be **any key, modifier combo, media key or mouse key**.
- For anything more complex, record a **Vial macro** (Macros tab) and put
  `M0`, `M1`, … on the gesture key.
- Layer keys (MO, TG …) and A+ User keycodes can't be sent from a gesture.

## Building it yourself

```sh
cd firmware/vial
./build.sh            # clones vial-qmk next to the script, or:
./build.sh ~/vial-qmk # use an existing checkout
```

Needs a normal QMK build environment. The script pulls drashna's community
modules at the commit Ploopy used (`ac529c8`), copies `keyboards/` into
vial-qmk and builds `ploopyco/aplus/rev2_001:vial`.

The changes are in `keyboards/ploopyco/aplus/keymaps/vial/` (keymap, wheel
handling and encoder map in `keymap.c`), plus a two-line guard in `ploopyco.c`
so its optical scroll-wheel driver isn't built for the A+. The keymap's `vial.json` is also at
`firmwares/aplus_vial.json` for reference; Vial does not need it.
