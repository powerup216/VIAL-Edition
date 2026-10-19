#!/usr/bin/env python3
"""Draw the keymap reference sheet for the custom A+ Vial firmware.

Letter page, four panels in the style of Ploopy's keymap.pdf:
  layer 0 left-handed base, layer 1 right-handed base,
  layer 3 mouse gestures, layer 2 control.

Writes keymap_vial.html next to this script; build.sh turns it into PDF/PNG.
Needs the Montserrat font installed. Edit the PANELS data to change labels.
"""
from html import escape
from pathlib import Path

RED, BLUE, GREEN, YELLOW = "#E3262E", "#3563F2", "#1FAF53", "#F0BE00"
INK, GREY, OUTLINE = "#111111", "#7A7A7A", "#5E5E5E"
FONT = "Montserrat"
DATE = "2026-10-19"

PANEL_W, PANEL_H = 306, 396


def text(x, y, s, size, weight=700, fill=INK, anchor="start"):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{escape(s)}</text>')


# ---------------------------------------------------------------- label blocks
# A label is a list of (text, kind). Kinds set size/colour/line height.
KINDS = {
    "main":  (13.0, 700, INK, 14.5),
    "head":  (9.5, 700, INK, 11.0),     # control-layer headings
    "side":  (11.0, 700, INK, 12.5),    # gesture labels left/right of the device
    "sub":   (6.4, 700, GREY, 7.6),
    "small": (7.4, 700, GREY, 10.0),    # "Turn:", "Press:"
}


def kind_style(kind, accent):
    if kind.startswith("acc"):          # coloured main line, e.g. "acc:#hex"
        return (13.0, 700, kind.split(":", 1)[1], 14.5)
    return KINDS[kind]


def block_height(lines):
    return sum(kind_style(k, None)[3] for _, k in lines)


def block(x, top, lines, anchor):
    out, y = [], top
    for s, k in lines:
        size, weight, fill, lh = kind_style(k, None)
        y += lh
        out.append(text(x, y - (lh - size) * 0.45, s, size, weight, fill, anchor))
    return "".join(out)


def leader(x1, y1, x2, y2):
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{INK}" '
            f'stroke-width="1.7" stroke-linecap="round" stroke-dasharray="0.1 3.1"/>')


# ---------------------------------------------------------------- device
def device(cx, cy, ring, faded=False):
    """A+ outline. cy is the ball centre. Returns (svg, anchor points)."""
    st = "#C9C9C9" if faded else OUTLINE
    sw = 1.8
    top = cy - 72
    parts = [
        f'<rect x="{cx-74}" y="{top}" width="148" height="166" rx="32" fill="white" stroke="{st}" stroke-width="{sw}"/>',
        f'<rect x="{cx-69}" y="{top+5}" width="138" height="156" rx="27" fill="white" stroke="{st}" stroke-width="{sw}"/>',
    ]
    keys = {
        "TLL": (cx-63, top+11, 30, 70),
        "TL":  (cx-30, top+9, 28, 36),
        "TR":  (cx+2,  top+9, 28, 36),
        "TRR": (cx+33, top+11, 30, 70),
        "BL":  (cx-63, cy+10, 30, 44),
        "BR":  (cx+33, cy+10, 30, 44),
    }
    for x, y, w, h in keys.values():
        parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="white" stroke="{st}" stroke-width="{sw}"/>')
    # Gap around the ball, then the ball with its coloured ring.
    parts.append(f'<circle cx="{cx}" cy="{cy}" r="41" fill="white" stroke="{st}" stroke-width="{sw}"/>')
    ring_col = "#D8D8D8" if faded and ring is None else (ring or st)
    parts.append(f'<circle cx="{cx}" cy="{cy}" r="33" fill="white" stroke="{ring_col}" stroke-width="6.5"/>')
    # Wheels.
    ky = cy + 63
    for kx in (cx - 21, cx + 21):
        parts.append(f'<circle cx="{kx}" cy="{ky}" r="23" fill="white"/>')
    for kx in (cx - 21, cx + 21):
        parts.append(f'<circle cx="{kx}" cy="{ky}" r="20" fill="white" stroke="{st}" stroke-width="{sw}"/>')

    anchors = {k: (x + w / 2, y + h / 2) for k, (x, y, w, h) in keys.items()}
    anchors["KL"] = (cx - 21, ky)
    anchors["KR"] = (cx + 21, ky)
    anchors["bottom"] = top + 166
    anchors["top"] = top
    return "".join(parts), anchors


def arrow(x1, y1, x2, y2, width=4.5, head=10, color=INK):
    import math
    ang = math.atan2(y2 - y1, x2 - x1)
    bx, by = x2 - head * math.cos(ang), y2 - head * math.sin(ang)
    px, py = -math.sin(ang), math.cos(ang)
    hw = head * 0.62
    pts = f"{x2:.1f},{y2:.1f} {bx+px*hw:.1f},{by+py*hw:.1f} {bx-px*hw:.1f},{by-py*hw:.1f}"
    return (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{bx:.1f}" y2="{by:.1f}" stroke="{color}" stroke-width="{width}"/>'
            f'<polygon points="{pts}" fill="{color}"/>')


# ---------------------------------------------------------------- panel chrome
def chrome(title_lines, colour, layer_note):
    out = [f'<rect x="3" y="3" width="{PANEL_W-6}" height="{PANEL_H-6}" fill="white" stroke="{INK}" stroke-width="0.8"/>',
           text(22, 44, "A+", 27, 800),
           text(23, 57, "Trackball · Vial", 7.6, 700, GREY),
           text(23, 67, layer_note, 7.6, 700, colour)]
    y = 38
    for s, weight in title_lines:
        out.append(text(PANEL_W - 22, y, s, 19, weight, colour, "end"))
        y += 21
    out.append(text(PANEL_W - 14, PANEL_H - 12, f"Custom Vial firmware · {DATE}", 6.6, 700, GREY, "end"))
    return "".join(out)


# ---------------------------------------------------------------- base layers
def base_panel(title, colour, note, labels, wheels):
    cx, cy = 153, 196
    dev, a = device(cx, cy, colour)
    out = [chrome(title, colour, note), dev]

    # Side labels: (key, side) -> block anchored to the key centre.
    for key, side in (("TLL", "L"), ("BL", "L"), ("TRR", "R"), ("BR", "R")):
        lines = labels[key]
        kx, ky = a[key]
        if side == "L":
            tx = cx - 86
            out.append(block(tx, ky - 11, lines, "end"))
            out.append(leader(tx + 5, ky - 3, kx - 2, ky))
        else:
            tx = cx + 86
            out.append(block(tx, ky - 11, lines, "start"))
            out.append(leader(tx - 5, ky - 3, kx + 2, ky))

    # Top labels sit above the device, leaders drop into the keys.
    for key, anchor, tx in (("TL", "end", cx - 10), ("TR", "start", cx + 10)):
        lines = labels[key]
        bottom = a["top"] - 10
        out.append(block(tx, bottom - block_height(lines), lines, anchor))
        kx, ky = a[key]
        out.append(leader(kx, bottom + 2, kx, ky))

    # Wheel labels below the device.
    for key, anchor, tx in (("KL", "end", cx - 7), ("KR", "start", cx + 7)):
        kx, ky = a[key]
        top = a["bottom"] + 12
        out.append(leader(kx, ky + 4, kx, top + 2))
        out.append(block(tx, top + 2, wheels[key], anchor))
    return "".join(out)


BOOT = [("Bootloader:", "sub"), ("Hold While", "sub"), ("Plugging", "sub"), ("In USB", "sub")]
CONTROL = [("Press:", "small"), ("Switch To", f"acc:{GREEN}"), ("Control Layer", f"acc:{GREEN}")]
GESTURE = [("Press and hold:", "small"), ("Activate Mouse", f"acc:{YELLOW}"), ("Gesture Layer", f"acc:{YELLOW}")]


def left_panel():
    labels = {
        "TLL": [("Middle", "main"), ("Click", "main"), ("Button 3", "sub")],
        "TL":  [("F7", "main")],
        "TR":  [("Back", "main"), ("Button 4", "sub")],
        "TRR": [("Forward", "main"), ("Button 5", "sub")],
        "BL":  [("Left", "main"), ("Click", "main")] + BOOT,
        "BR":  [("Right", "main"), ("Click", "main")],
    }
    wheels = {
        "KL": [("Turn:", "small"), ("Volume", "main")] + CONTROL,
        "KR": [("Turn:", "small"), ("Vertical Scroll", "main")] + GESTURE,
    }
    return base_panel([("Base Layer:", 300), ("Left-Handed", 700), ("Navigation", 300)],
                      RED, "Layer 0 · default at startup", labels, wheels)


def right_panel():
    labels = {
        "TLL": [("Back", "main"), ("Button 4", "sub")],
        "TL":  [("Forward", "main"), ("Button 5", "sub")],
        "TR":  [("Drag Scroll", "main")],
        "TRR": [("Right", "main"), ("Click", "main")],
        "BL":  [("Left", "main"), ("Click", "main")] + BOOT,
        "BR":  [("Middle", "main"), ("Click", "main"), ("Button 3", "sub")],
    }
    wheels = {
        "KL": [("Turn:", "small"), ("Vertical Scroll", "main")] + GESTURE,
        "KR": [("Turn:", "small"), ("Volume", "main")] + CONTROL,
    }
    return base_panel([("Base Layer:", 300), ("Right-Handed", 700), ("Navigation", 300)],
                      BLUE, "Layer 1 · switch with Mirror", labels, wheels)


# ---------------------------------------------------------------- gestures
def gesture_panel():
    import math
    cx, cy = 153, 196
    dev, a = device(cx, cy, None, faded=True)
    out = [chrome([("Mouse Gestures", 300), ("Layer", 300)], YELLOW, "Layer 3 · editable in Vial"), dev]

    # Small arrows on the keys: which layer-3 key edits which direction.
    mini = {"TLL": 225, "TL": 270, "TR": 315, "TRR": 0, "BR": 45, "KR": 90, "KL": 135, "BL": 180}
    for key, deg in mini.items():
        kx, ky = a[key]
        r = math.radians(deg)
        dx, dy = math.cos(r) * 7, math.sin(r) * 7
        out.append(arrow(kx - dx, ky - dy, kx + dx, ky + dy, width=1.8, head=5, color=YELLOW))

    # Big gesture arrows. Angle in SVG degrees (0 = east, 90 = south).
    gestures = [
        (270, 86, ["Play/Pause"], "middle", (0, -6)),
        (315, 92, ["Redo"], "start", (4, -4)),
        (0, 76, ["Next", "Virtual", "Desktop"], "start", (5, 0)),
        (45, 92, ["Paste"], "start", (4, 12)),
        (90, 90, ["Copy"], "middle", (0, 16)),
        (135, 92, ["Cut"], "end", (-4, 12)),
        (180, 76, ["Previous", "Virtual", "Desktop"], "end", (-5, 0)),
        (225, 92, ["Undo"], "end", (-4, -4)),
    ]
    for deg, length, words, anchor, (ox, oy) in gestures:
        r = math.radians(deg)
        x1, y1 = cx + math.cos(r) * 28, cy + math.sin(r) * 28
        x2, y2 = cx + math.cos(r) * length, cy + math.sin(r) * length
        out.append(arrow(x1, y1, x2, y2))
        lx, ly = x2 + ox, y2 + oy
        lines = [(w, "side" if deg in (0, 180) else "main") for w in words]
        h = block_height(lines)
        if deg in (270, 315, 225):          # above: grow upwards
            top = ly - h
        elif deg in (0, 180):               # sides: centre vertically
            top = ly - h / 2
        else:                               # below: grow downwards
            top = ly - 10
        out.append(block(lx, top, lines, anchor))

    notes = [
        ("Gesture instructions:", 7.8, 700, INK),
        ("Hold the gesture key, move the ball in a direction, then let go.", 6.6, 500, INK),
        ("The LEDs blink when a gesture is recognised. Redo and desktop", 6.6, 500, INK),
        ("switching send the right shortcut for Windows, Linux or macOS.", 6.6, 500, INK),
        ("In Vial, each direction is a key on layer 3 (the small arrows).", 6.6, 500, INK),
    ]
    y = 328
    for s, size, weight, fill in notes:
        out.append(text(18, y, s, size, weight, fill))
        y += 8.6
    return "".join(out)


# ---------------------------------------------------------------- control
def control_panel():
    cx, cy = 153, 196
    dev, a = device(cx, cy, GREEN)
    out = [chrome([("Control", 300), ("Layer", 300)], GREEN, "Layer 2 · press a wheel"), dev]

    def ctl(head, desc):
        return [(h, "head") for h in head] + [(d, "sub") for d in desc]

    side = {
        "TLL": ("L", ctl(["Cycle", "DPI"], ["600, 900, 1200,", "1600, 2400.", "LEDs blink once", "for 600, more", "for higher.", "900 is default."])),
        "BL":  ("L", ctl(["Mirror", "Base Layer"], ["Left-handed (L0):", "1 blink.", "Right-handed (L1):", "2 blinks."])),
        "TRR": ("R", ctl(["Toggle", "Horizontal", "Scroll Type"], ["For wheels set to", "Wheel H-Scrl:", "scroll events or", "left/right arrows.", "1 blink = arrows,", "2 = scroll."])),
        "BR":  ("R", ctl(["Toggle Drag", "Scroll Type"], ["Hold to activate", "(like Shift) or", "toggle (like Caps", "Lock). 1 blink =", "toggle, 2 = hold."])),
    }
    for key, (s, lines) in side.items():
        kx, ky = a[key]
        h = block_height(lines)
        top = ky - 12 if key in ("TLL", "TRR") else ky - 12
        if key in ("TLL", "TRR"):
            top = min(top, a["top"] + 2)
        if s == "L":
            tx = cx - 84
            out.append(block(tx, top, lines, "end"))
            out.append(leader(tx + 4, top + 7, kx - 2, ky))
        else:
            tx = cx + 84
            out.append(block(tx, top, lines, "start"))
            out.append(leader(tx - 4, top + 7, kx + 2, ky))

    tops = {
        "TL": ("end", cx - 6, ctl(["Adjust LED", "Brightness"], ["Cycles 25, 50,", "75, 100%."])),
        "TR": ("start", cx + 6, ctl(["Toggle Vertical", "Scroll Type"], ["Scroll events or", "up/down arrows.", "1 blink = arrows,", "2 = scroll."])),
    }
    for key, (anchor, tx, lines) in tops.items():
        bottom = a["top"] - 8
        out.append(block(tx, bottom - block_height(lines), lines, anchor))
        kx, ky = a[key]
        out.append(leader(kx, bottom + 2, kx, ky))

    # Both wheels: one centred caption.
    for key in ("KL", "KR"):
        kx, ky = a[key]
        out.append(leader(kx, ky + 4, cx + (kx - cx) * 0.4, a["bottom"] + 12))
    y = a["bottom"] + 21
    out.append(text(cx, y, "Same for both wheels", 6.4, 700, GREY, "middle"))
    out.append(text(cx - 4, y + 15, "Press:", 7.4, 700, GREY, "end"))
    out.append(text(cx, y + 15, "Switch To Base Layer", 11.5, 700, RED, "start"))
    out.append(text(cx - 4, y + 30, "Turn:", 7.4, 700, GREY, "end"))
    out.append(text(cx, y + 30, "Volume", 11.5, 700, INK, "start"))
    out.append(text(18, PANEL_H - 38, "LEDs blink briefly when a setting changes.", 6.4, 500, INK))
    out.append(text(18, PANEL_H - 30, "Settings are saved and survive unplugging.", 6.4, 500, INK))
    out.append(text(18, PANEL_H - 22, "LED colours & brightness: Vial \u2192 Lighting tab.", 6.4, 700, INK))
    return "".join(out)


# ---------------------------------------------------------------- page
def page():
    panels = [left_panel(), right_panel(), gesture_panel(), control_panel()]
    pos = [(0, 0), (PANEL_W, 0), (0, PANEL_H), (PANEL_W, PANEL_H)]
    body = "".join(f'<g transform="translate({x},{y})">{p}</g>' for p, (x, y) in zip(panels, pos))
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="1062px" height="1370px" '
           f'viewBox="0 0 612 792">{body}</svg>')
    return ("<!doctype html><html><head><meta charset='utf-8'><title>A+ Vial keymap</title>"
            "<style>html,body{margin:0;padding:0;background:white;overflow:hidden}svg{display:block}</style></head>"
            f"<body>{svg}</body></html>")


if __name__ == "__main__":
    out = Path(__file__).with_name("keymap_vial.html")
    out.write_text(page(), encoding="utf-8")
    print(out)
