/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#define DYNAMIC_KEYMAP_LAYER_COUNT 8

/* Wheels as encoders: 0 = left wheel, 1 = right wheel. */
#define NUM_ENCODERS 2
/* Don't build ploopyco.c's optical scroll-wheel driver; keymap.c drives the
   A+'s magnetic wheels instead. */
#define PLOOPY_NO_OPT_ENCODER

/* Vial: unique ID for this keyboard definition. */
#define VIAL_KEYBOARD_UID {0x88, 0x9B, 0xA3, 0x12, 0x6C, 0x71, 0x9C, 0x56}

/* Vial unlock combo: hold the two big bottom buttons (matrix 0,0 and 0,5). */
#define VIAL_UNLOCK_COMBO_ROWS { 0, 0 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 5 }

/* LED colours are edited from Vial's Lighting tab; keymap.c handles the
   lighting commands itself instead of QMK's stock rgblight handler. */
#define VIA_CUSTOM_LIGHTING_ENABLE

/* Storage for the settings and per-layer LED colours (see persistent_t). */
#define EECONFIG_USER_DATA_SIZE 16
