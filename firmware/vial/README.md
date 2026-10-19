# A+ Trackball — custom Vial firmware

Ready-to-flash file: [`firmwares/aplus_vial.uf2`](../../firmwares/aplus_vial.uf2)

This is Ploopy's A+ firmware (from the `ploopyco/a+` branch of
[ploopyco/qmk_firmware](https://github.com/ploopyco/qmk_firmware/tree/ploopyco/a%2B))
ported to [vial-qmk](https://github.com/vial-kb/vial-qmk), with these changes:

| Change | Stock | This firmware |
|---|---|---|
| Horizontal wheel on the base layers | Horizontal scroll | **Volume up/down**, same as the wheels on the control layer |
| Left-handed base layer LED | Green | **Red** |
| Control layer LED | Red | **Green** |
| Configurator | VIA (needs a JSON sideload) | **Vial** (auto-detected, no JSON needed) |
| Mouse gestures | Hard-coded | **Editable in Vial** (layer 3) |

Everything else (DPI cycling, drag scroll, LED brightness, mirror mode, OS-aware
shortcuts, settings saved across power cycles) works as on the stock firmware.

## Flashing

1. Unplug the A+.
2. Hold the bottom-left button (left click, right-handed layout) and plug it in.
   A drive called `RPI-RP2` appears.
3. Drag `aplus_vial.uf2` onto it. It reboots by itself.

To go back to stock, flash `firmwares/aplus.uf2` the same way.

## Horizontal wheel modes

The control layer's **Toggle Horizontal Scroll Type** button (outer top-right
button on the control layer) now cycles through three modes. The choice is saved.

| Mode | LED blinks |
|---|---|
| Volume (default) | 1 |
| Horizontal scroll (stock behaviour) | 2 |
| Left/right arrow keys | 3 |

## Using Vial

Open [vial.rocks](https://vial.rocks) in Chrome/Edge, or the Vial desktop app,
and plug in the A+. It is detected automatically.

- **Layer 0** — right-handed base. **Layer 1** — left-handed base. **Layer 2** — control.
- **Layers 4–7** are free for your own use.
- A+ functions (gesture key, drag scroll, LED brightness, DPI, mode toggles,
  mirror) are in the **User** tab.
- **Unlocking** (needed only for the matrix tester or assigning `QK_BOOT`): hold
  the two big bottom buttons when Vial asks.

## Mouse gesture layer (layer 3)

Layer 3 is never switched on. It is a table: each of its 8 keys is what one
gesture direction sends. Hold the gesture key (left wheel press, right-handed),
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

The changes are all in `keyboards/ploopyco/aplus/keymaps/vial/keymap.c`
(look for "Custom:"). The keymap's `vial.json` is also at
`firmwares/aplus_vial.json` for reference; Vial does not need it.
