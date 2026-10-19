/* Copyright 2023 Colin Lam (Ploopy Corporation)
 * Copyright 2020 Christopher Courtney, aka Drashna Jael're  (@drashna) <drashna@live.com>
 * Copyright 2019 Sunjun Kim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

#include "rgblight.h"
#include "printf.h"

#include "../../../common/tmag5273wheel.h"
#include "pointing_device_gestures.h"

#ifdef DYNAMIC_KEYMAP_ENABLE
#    include "dynamic_keymap.h"
#endif

/* vial-qmk predates the LCG() shorthand (Left Control + Left GUI). */
#ifndef LCG
#    define LCG(kc) (QK_LCTL | QK_LGUI | (kc))
#endif

/* State variables for wheels. */
uint16_t leftwheel_deadzone_center = 0;
uint16_t rightwheel_deadzone_center = 0;

uint16_t leftwheel_current_position = 0;
uint16_t rightwheel_current_position = 0;
uint32_t last_scroll_time = 0;

/* Layer lighting, used to blink when making gestures.
   Custom: left-handed layer is red and the control layer is green
   (swapped from the stock firmware). */
const rgblight_segment_t PROGMEM righty_nav_layer_colour[] =        RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_NAVBLUE} );
const rgblight_segment_t PROGMEM lefty_nav_layer_colour[] =         RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_RED} );
const rgblight_segment_t PROGMEM control_layer_colour[] =           RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_NAVGREEN} );
const rgblight_segment_t PROGMEM gesture_layer_colour[] =           RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_GESTUREYELLOW} );
const rgblight_segment_t PROGMEM option_changed_layer_colour[] =    RGBLIGHT_LAYER_SEGMENTS( {0, 2, HSV_OPTIONCHANGED} );

/* Lighting layer definitions. Later layers take precedence, so these
   are defined in the order that they will be displayed in. */
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    righty_nav_layer_colour,
    lefty_nav_layer_colour,
    control_layer_colour,
    gesture_layer_colour,
    option_changed_layer_colour
);

/* Colour layer names. */
enum {
    RIGHTY_NAV_LAYER_COLOUR = 0,
    LEFTY_NAV_LAYER_COLOUR,
    CONTROL_LAYER_COLOUR,
    GESTURE_LAYER_COLOUR,
    OPTION_CHANGED_LAYER_COLOUR    
};

 /* EEPROM memory for the various persistent states that the A+ can have. 
    See eeconfig_init_user() for default values and meanings. */
typedef union {
  uint32_t raw;
  struct {
    bool    right_handed :1;               /* 0 = left-handed (default). */
    bool    horizontal_scroll_arrows :1;   /* Native horizontal scroll sends left/right arrows. */
    bool    vertical_scroll_arrows :1;     /* Native vertical scroll sends up/down arrows. */
    bool    drag_scroll_mode :1;
    uint8_t led_brightness: 8;
    uint8_t config_version: 2;             /* Settings are reset once when this changes. */
  };
} user_config_t;

user_config_t user_config;

/* Bump to reset the saved A+ settings once on the next flash (0-3). */
#define USER_CONFIG_VERSION 3 /* Earlier builds stored 0-2 here. */

/* Custom keycodes. These start at QK_KB_2 (right after ploopyco.c's DPI_CONFIG
   and DRAG_SCROLL) so that they line up with "customKeycodes" in vial.json and
   show up in Vial's "User" tab in this exact order. */
enum my_keycodes {
  PKC_TGL_MIRROR = QK_KB_2,
  PKC_TGL_VERT_SCRL,
  PKC_TGL_HORIZ_SCRL,
  PKC_TGL_DRAG_SCRL,
  PKC_GESTURE,
  PKC_DRAG_SCROLL,
  PKC_ADJUST_LED_BRIGHTNESS,
  PKC_BLINKY_DPI_CONFIG,
  /* Wheel keycodes, for the encoder slots in Vial. Put one on a wheel to get
     the stock smooth (hi-res) scrolling instead of key taps. In the CW slot it
     scrolls normally; in only the CCW slot it scrolls reversed. */
  PKC_WHEEL_VSCROLL,
  PKC_WHEEL_HSCROLL
};

/* Layer names. Layer 0 is the left-handed base (the default) and layer 1 the
   right-handed base. Layer 0 is always on; layer 1 sits on top of it and is
   switched on only when right-handed (mirror) mode is saved. */
enum {
    LAYER_NAV_LEFT_HANDED = 0,
    LAYER_NAV_RIGHT_HANDED,
    LAYER_CONTROL,
    LAYER_GESTURES      /* Never switched on; used as an editable gesture table. */
};

/* ===================== Vial-editable mouse gestures =====================

   Layer 3 is a lookup table for the 8 gesture directions, so each gesture
   can be changed in Vial like any other key. The table is laid out going
   clockwise around the A+, starting at the top-left button:

       Vial key (matrix)        Gesture direction   Default
       Top Left Left  (0,1)  -> North-West          Undo
       Top Left       (0,2)  -> North               Play/Pause
       Top Right      (0,3)  -> North-East          Redo        (OS-aware)
       Top Right Right(0,4)  -> East                Next desktop (OS-aware)
       Bottom Right   (0,5)  -> South-East          Paste
       Knob Right     (0,7)  -> South               Copy
       Knob Left      (0,6)  -> South-West          Cut
       Bottom Left    (0,0)  -> West                Prev desktop (OS-aware)

   A transparent (▽) or empty key keeps the built-in, OS-aware default.
   A gesture can be any key, modifier combo (e.g. LCtrl+C), media key,
   mouse key, or a Vial macro (M0, M1, ...). Macros let you do anything else.

   The module numbers directions 0-7 starting at East and going clockwise
   (E, SE, S, SW, W, NW, N, NE). This maps each to its matrix column. */
static const uint8_t gesture_direction_to_col[NUM_GESTURE_DIRECTIONS] = {
    4, /* 0 East       -> Top Right Right */
    5, /* 1 South-East -> Bottom Right    */
    7, /* 2 South      -> Knob Right      */
    6, /* 3 South-West -> Knob Left       */
    0, /* 4 West       -> Bottom Left     */
    1, /* 5 North-West -> Top Left Left   */
    2, /* 6 North      -> Top Left        */
    3, /* 7 North-East -> Top Right       */
};

/* Send a user-assigned gesture keycode. Returns false if the keycode can't be
   sent from a gesture, so the caller can fall back to the built-in default. */
static bool send_gesture_keycode(uint16_t keycode) {
    if (keycode == KC_NO || keycode == KC_TRNS) {
        return false;
    }
#ifdef DYNAMIC_KEYMAP_ENABLE
    if (IS_QK_MACRO(keycode)) {
        dynamic_keymap_macro_send(keycode - QK_MACRO);
        return true;
    }
#endif
    /* Plain keys and modifier combos, including media, system and mouse keys. */
    if (IS_QK_BASIC(keycode) || IS_QK_MODS(keycode)) {
        tap_code16(keycode);
        return true;
    }
    return false;
}

/* Override default function in submodule code so that we can blink whenever
   a successful gesture is registered. */
void pointing_device_gestures_trigger(uint8_t direction) {

    if (direction < NUM_GESTURE_DIRECTIONS) {
        uint16_t user_keycode = keymap_key_to_keycode(LAYER_GESTURES,
            (keypos_t){ .row = 0, .col = gesture_direction_to_col[direction] });

        if (send_gesture_keycode(user_keycode)) {
            rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
            return;
        }
    }

    /* Built-in defaults. Directions are numbered off 0-7, starting with east. */
    switch( direction ) {
        case 0: /* East, Next Virtual Desktop. */
            /* On Windows, send CTRL + LGUI + RIGHT. */
            if ( detected_host_os() == OS_WINDOWS ) {
                tap_code16( LCG( KC_RIGHT ) );
            }
            /* On Linux, send CTRL + ALT + RIGHT. */
            else if( detected_host_os() == OS_LINUX ) {
                tap_code16( LCA( KC_RIGHT ) );
            }
            /* On MacOs, send Control + RIGHT. */
            else {
                tap_code16( LCTL( KC_RIGHT ) );
            }
            break;
        case 1: /* South-East, Paste. */
            tap_code16( C(KC_V) );
            break;
        case 2: /* South, Copy. */
            tap_code16( C(KC_C) );
            break;
        case 3: /* South-West, Cut. */
            tap_code16( C(KC_X) );
            break;
        case 4: /* West, Previous Virtual Desktop. */
            /* On Windows, send CTRL + LGUI + LEFT. */
            if ( detected_host_os() == OS_WINDOWS ) {
                tap_code16( LCG( KC_LEFT ) );
            }
            /* On Linux, send CTRL + ALT + LEFT. */
            else if( detected_host_os() == OS_LINUX ) {
                tap_code16( LCA( KC_LEFT ) );
            }
            /* On MacOs, send Control + LEFT. */
            else {
                tap_code16( LCTL( KC_LEFT ) );
            }
            break;
        case 5: /* North-West, Undo. */
            tap_code16( C(KC_Z) );
            break;
        case 6: /* North, Play/Pause Audio. */
            tap_code16( KC_MEDIA_PLAY_PAUSE );
            break;
        case 7: /* North-East, Redo. */
            /* On Windows/Linux, send CTRL + Y. */
            if ( detected_host_os() == OS_WINDOWS || detected_host_os() == OS_LINUX ) {
                tap_code16( C(KC_Y) );
            }
            /* On MacOs, send CMD + SHIFT + Z. */
            else {
                register_code(KC_LCMD);
                register_code(KC_LSFT);
                tap_code(KC_Z);
                unregister_code(KC_LCMD);
                unregister_code(KC_LSFT);
            }
            break;
        default:
            break;
    }

    /* Flash light to indicate successful gesture processing. */
    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
}

/* Mouse gestures keymap. Since we cannot use custom keycodes in this array and
   we've overridden the function that uses it, we define this here with
   KC_NO (no action) to prevent a compiler error. See pointing_device_gestures_trigger()
   for what the gestures actually do. */
const uint16_t PROGMEM pointing_device_gestures[NUM_GESTURE_DIRECTIONS] =
    GESTURES_CARDINAL_AND_ORDINAL_DIRECTIONS( KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO );

/* Keymap. */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    // Layer 0: left-handed base, the default at startup. Custom layout:
    //   top row      Mouse 3, F7, Mouse 4, Mouse 5
    //   big buttons  Mouse 1 (left), Mouse 2 (right)
    //   wheel press  Control layer (left), Gesture (right)
    [LAYER_NAV_LEFT_HANDED] = LAYOUT(   MS_BTN3, KC_F7, MS_BTN4, MS_BTN5,
                                        MS_BTN1, MS_BTN2,
                                        TG(LAYER_CONTROL), PKC_GESTURE ),
    // Layer 1: right-handed base (Ploopy's stock layout). Active in mirror mode.
    [LAYER_NAV_RIGHT_HANDED] = LAYOUT(  MS_BTN4, MS_BTN5, PKC_DRAG_SCROLL, MS_BTN2,
                                        MS_BTN1, MS_BTN3,
                                        PKC_GESTURE, TG(LAYER_CONTROL) ),
    // Layer for all of the customization options.
    [LAYER_CONTROL] = LAYOUT(           PKC_BLINKY_DPI_CONFIG, PKC_ADJUST_LED_BRIGHTNESS, PKC_TGL_VERT_SCRL, PKC_TGL_HORIZ_SCRL,
                                        PKC_TGL_MIRROR, PKC_TGL_DRAG_SCRL, 
                                        TG(LAYER_CONTROL), TG(LAYER_CONTROL) ),
    // Mouse gesture table (see gesture_direction_to_col above). Never switched on.
    // Transparent keys use the built-in OS-aware actions.
    //                                  NW: Undo  N: Play/Pause    NE: Redo  E: Next desktop
    [LAYER_GESTURES] = LAYOUT(          C(KC_Z),  KC_MPLY,         _______,  _______,
    //                                  W: Prev desktop  SE: Paste
                                        _______,         C(KC_V),
    //                                  SW: Cut   S: Copy
                                        C(KC_X),  C(KC_C) ),
    [4] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [5] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [6] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ ),
    [7] = LAYOUT( _______, _______, _______, _______, _______, _______, _______, _______ )
};

/* Wheel map, editable in Vial: { left wheel, right wheel }, each as
   ENCODER_CCW_CW(counter-clockwise keycode, clockwise keycode).
   "Clockwise" is the direction that turns the volume up.
   PKC_WHEEL_VSCROLL / PKC_WHEEL_HSCROLL give the stock smooth scrolling. */
#define WHEEL_VOLUME  ENCODER_CCW_CW(KC_VOLD, KC_VOLU)
#define WHEEL_VSCROLL ENCODER_CCW_CW(PKC_WHEEL_VSCROLL, PKC_WHEEL_VSCROLL)
#define WHEEL_TRNS    ENCODER_CCW_CW(_______, _______)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    //                                  Left wheel     Right wheel
    [LAYER_NAV_LEFT_HANDED]  =        { WHEEL_VOLUME,  WHEEL_VSCROLL },
    [LAYER_NAV_RIGHT_HANDED] =        { WHEEL_VSCROLL, WHEEL_VOLUME  },
    [LAYER_CONTROL]          =        { WHEEL_VOLUME,  WHEEL_VOLUME  },
    [LAYER_GESTURES]         =        { WHEEL_TRNS,    WHEEL_TRNS    },
    [4]                      =        { WHEEL_TRNS,    WHEEL_TRNS    },
    [5]                      =        { WHEEL_TRNS,    WHEEL_TRNS    },
    [6]                      =        { WHEEL_TRNS,    WHEEL_TRNS    },
    [7]                      =        { WHEEL_TRNS,    WHEEL_TRNS    },
};

/* Custom encoder driver: the wheels are read in pointing_device_task_user(),
   which queues encoder events itself. Nothing to do here. */
void encoder_driver_init(void) {}
void encoder_driver_task(void) {}

/* Called whenever layers are modified. */
layer_state_t layer_state_set_user(layer_state_t state) {
    /* Set layer colours. */
    switch (get_highest_layer(state)) {
        case LAYER_NAV_RIGHT_HANDED:
            rgblight_set_layer_state(RIGHTY_NAV_LAYER_COLOUR, true);
            
            rgblight_set_layer_state(LEFTY_NAV_LAYER_COLOUR, false);
            rgblight_set_layer_state(CONTROL_LAYER_COLOUR, false);            

            dprintf("change layer: right-handed nav\n");
            break;
        case LAYER_NAV_LEFT_HANDED:
            rgblight_set_layer_state(LEFTY_NAV_LAYER_COLOUR, true);

            rgblight_set_layer_state(RIGHTY_NAV_LAYER_COLOUR, false);
            rgblight_set_layer_state(CONTROL_LAYER_COLOUR, false);  

            dprintf("change layer: left-handed nav\n");
            break;
        case LAYER_CONTROL:
            rgblight_set_layer_state(CONTROL_LAYER_COLOUR, true);
            
            rgblight_set_layer_state(RIGHTY_NAV_LAYER_COLOUR, false);
            rgblight_set_layer_state(LEFTY_NAV_LAYER_COLOUR, false); 

            dprintf("change layer: control\n");
            break;
        default:
            rgblight_setrgb(RGB_CYAN);
            dprintf("change layer: oopsie, invalid layer!\n");
            break;
    }
    return state;
}

/* Set sane defaults when EEPROM is reset. */
void eeconfig_init_user(void) {

    user_config.raw = 0;

    user_config.right_handed = false;               // Left-handed (layer 0) by default.
    user_config.horizontal_scroll_arrows = false;   // False = Scroll events, True = left/right arrows.
    user_config.vertical_scroll_arrows = false;     // False = Scroll events, True = up/down arrows.
    user_config.drag_scroll_mode = false;           // Hold to activate.
    user_config.led_brightness = 4;                 // Brightest mode by default.
    user_config.config_version = USER_CONFIG_VERSION;

    eeconfig_update_user(user_config.raw);
}

void keyboard_pre_init_user(void) {

    /* Set up GPIO for both TMAG sensors. */
    gpio_set_pin_output_push_pull(TMAG5273_D0_PWR_PIN);
    gpio_set_pin_output_push_pull(TMAG5273_D1_PWR_PIN);
    gpio_write_pin_low(TMAG5273_D0_PWR_PIN);
    gpio_write_pin_low(TMAG5273_D1_PWR_PIN);

}

void keyboard_post_init_user(void) {
    /* ============================ Debug Setup ============================ */

    /* If you turn these on, also uncomment CONSOLE_ENABLE = yes in
       post_rules.mk! */
    //debug_enable = true;
    //debug_matrix=true;
    //debug_keyboard=true;
    //debug_mouse=true;

    /* ========================== Scroll Wheel Setup ======================= */

    /* Start up the two wheels. */
    tmag5273_init();

    /* Init TMAG5273 D0 (left wheel) */
    gpio_write_pin_high(TMAG5273_D0_PWR_PIN);
    wait_us(370);  // TMAG tstart_power_up, plus 100us margin
    tmag5273_init_device(TMAG5273_D0_I2C_ADDRESS);
    wait_ms(20);
    leftwheel_current_position = tmag5273_get_angle(TMAG5273_D0_I2C_ADDRESS);
    leftwheel_deadzone_center = leftwheel_current_position;


    /* Init TMAG5273 D1 (right wheel) */
    gpio_write_pin_high(TMAG5273_D1_PWR_PIN);
    wait_us(370);  // TMAG tstart_power_up, plus 100us margin
    tmag5273_init_device(TMAG5273_D1_I2C_ADDRESS);
    wait_ms(20);
    rightwheel_current_position = tmag5273_get_angle(TMAG5273_D1_I2C_ADDRESS);
    rightwheel_deadzone_center = rightwheel_current_position;


    /* =================== Persistent Configuration Setup ================== */

    /* Load up configuration details from the EEPROM. */
    user_config.raw = eeconfig_read_user();

    /* Settings saved by the stock firmware or an older build of this one
       meant different things (e.g. which hand is the default), so start
       fresh from the defaults once. */
    if( user_config.config_version != USER_CONFIG_VERSION ) {
        eeconfig_init_user();
    }

    /* Set the startup layer based on handed-ness. Layer 0 (left-handed) is
       always on and is the default; layer 1 goes on top for right-handed. */
    if( user_config.right_handed ) {
        layer_on(LAYER_NAV_RIGHT_HANDED);
    }


    /* ============================ Lighting Setup ========================= */

    /* Enable the LED layers. */
    rgblight_layers = my_rgb_layers;

    /* Enable RGB lighting for layer indicators. */
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

    /* Set an initial colour to establish a baseline brightness for the RGB layers. */
    rgblight_sethsv(0, 0, RGBLIGHT_VAL_STEP * user_config.led_brightness);
}  

bool process_record_user(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case PKC_TGL_MIRROR:
            if (record->event.pressed) {
                /* Toggle handed-ness, record new state to EEPROM. */
                user_config.right_handed ^= 1;
                eeconfig_update_user(user_config.raw);

                /* Activate the correct navigation layer and blink the LEDs.
                   1 blink = left-handed, 2 blinks = right-handed. */
                if( !user_config.right_handed ) {
                    dprintf( "Lefty nav mode.\n");
                    layer_off(LAYER_NAV_RIGHT_HANDED);
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
                }
                else {
                    dprintf( "Righty nav mode.\n");
                    layer_on(LAYER_NAV_RIGHT_HANDED);
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                }
            }
            return true;
        case PKC_TGL_VERT_SCRL:
            if (record->event.pressed) {
                user_config.vertical_scroll_arrows ^= 1;
                eeconfig_update_user(user_config.raw);

                if( user_config.vertical_scroll_arrows ) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
                    dprintf("Vert. scroll = arrows.\n");
                }
                else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Vert. scroll = scroll events.\n");
                }
            }
            return true;
        case PKC_TGL_HORIZ_SCRL:
            if (record->event.pressed) {
                user_config.horizontal_scroll_arrows ^= 1;
                eeconfig_update_user(user_config.raw);

                if( user_config.horizontal_scroll_arrows ) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
                    dprintf("Horiz. scroll = arrows.\n");
                }
                else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Horiz. scroll = scroll events.\n");
                }
            }
            return true;
        case PKC_TGL_DRAG_SCRL:
            if (record->event.pressed) {
                user_config.drag_scroll_mode ^= 1;
                eeconfig_update_user(user_config.raw);

                if( user_config.drag_scroll_mode ) {
                    rgblight_blink_layer(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT*2);
                    dprintf("Drag scroll: toggle to activate.\n");
                }
                else {
                    rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, 2);
                    dprintf("Drag scroll: hold to activate.\n");
                }
            }
            return true;
        case PKC_GESTURE:
            if (record->event.pressed) {
                /* Send correct gesture mode activation based on preference. */
                pointing_device_gestures_start();
                rgblight_set_layer_state(GESTURE_LAYER_COLOUR, true);
            }
            /* Switch is released. */
            else {
                pointing_device_gestures_end();
                rgblight_set_layer_state(GESTURE_LAYER_COLOUR, false);
            }
            return true; 
        case PKC_DRAG_SCROLL:
            if( user_config.drag_scroll_mode ) {
                if (record->event.pressed) {
                    toggle_drag_scroll();
                }
            }
            /* Press and hold mode. */
            else {
                is_drag_scroll = record->event.pressed;
            }
            return true;
        case PKC_ADJUST_LED_BRIGHTNESS:
            /* Increase brightness in RGBLIGHT_VAL_STEP steps. 
               Go to minimum brightness if the max brightness is hit. */
            if (record->event.pressed) {
                if( rgblight_get_val() >= RGBLIGHT_VAL_STEP * 4 ) {
                    rgblight_decrease_val_noeeprom();
                    rgblight_decrease_val_noeeprom();
                    rgblight_decrease_val_noeeprom();
                    user_config.led_brightness = 1;
                    eeconfig_update_user(user_config.raw);
                }
                else {
                    rgblight_increase_val_noeeprom();
                    user_config.led_brightness += 1;
                    eeconfig_update_user(user_config.raw);
                }

                dprintf("Adjusted LED brightness. Current brightness = %d/255\n", rgblight_get_val());
            }
            return true;
        case PKC_BLINKY_DPI_CONFIG:
            if (record->event.pressed) {
                cycle_dpi();

                dprintf("DPI changed. Current DPI = %d\n", dpi_array[keyboard_config.dpi_config]);
                rgblight_blink_layer_repeat(OPTION_CHANGED_LAYER_COLOUR, OPTION_CHANGE_BLINK_TIMEOUT, keyboard_config.dpi_config + 1);
            }
            return true;         
        case PKC_WHEEL_VSCROLL:
        case PKC_WHEEL_HSCROLL:
            /* Only meaningful on a wheel; handled in pointing_device_task_user(). */
            return false;
        default:
            return true;
    }
}

/* ============================== Scroll wheels ==============================

   Each wheel (0 = left, 1 = right) does whatever its slot in the encoder map
   says for the current layer, so both wheels can be remapped in Vial:

   - PKC_WHEEL_VSCROLL / PKC_WHEEL_HSCROLL: stock smooth hi-res scrolling
     (or arrow keys, if that's toggled on the control layer). In the CW slot it
     scrolls normally; placed in only the CCW slot it scrolls reversed.
   - Anything else: every TMAG5273_VOLUME_SCROLL_TICK_SIZE of rotation taps
     the CW or CCW keycode, the same step size as the stock volume wheel.

   Positive wheel delta is "clockwise": the direction that turned the volume
   up and scrolled down on the stock firmware. */

typedef enum {
    WHEEL_ROLE_KEYS = 0,
    WHEEL_ROLE_VSCROLL,
    WHEEL_ROLE_HSCROLL
} wheel_role_t;

#define NUM_WHEELS 2

/* Matrix columns of the wheel buttons; a wheel is ignored while pressed. */
static const uint8_t wheel_button_col[NUM_WHEELS] = { 6, 7 };

static int16_t wheel_key_tick[NUM_WHEELS]    = { 0, 0 };
static int16_t wheel_arrow_tick[NUM_WHEELS]  = { 0, 0 };
static int16_t wheel_lowres_tick[NUM_WHEELS] = { 0, 0 };

uint32_t leftwheel_timeout = 0;
uint32_t rightwheel_timeout = 0;

/* Look up the wheel's keycode on the highest active layer that isn't
   transparent for it, like QMK does for keys. */
static uint16_t wheel_keycode(uint8_t index, bool clockwise) {
    keypos_t pos = { .row = clockwise ? KEYLOC_ENCODER_CW : KEYLOC_ENCODER_CCW, .col = index };
    return keymap_key_to_keycode(layer_switch_get_layer(pos), pos);
}

static wheel_role_t wheel_role(uint8_t index, bool *reversed) {
    uint16_t cw  = wheel_keycode(index, true);
    uint16_t ccw = wheel_keycode(index, false);

    *reversed = false;
    if( cw == PKC_WHEEL_VSCROLL ) return WHEEL_ROLE_VSCROLL;
    if( cw == PKC_WHEEL_HSCROLL ) return WHEEL_ROLE_HSCROLL;
    *reversed = true;
    if( ccw == PKC_WHEEL_VSCROLL ) return WHEEL_ROLE_VSCROLL;
    if( ccw == PKC_WHEEL_HSCROLL ) return WHEEL_ROLE_HSCROLL;
    *reversed = false;
    return WHEEL_ROLE_KEYS;
}

/* Turn one wheel's movement into keys or scroll for this report. */
static void process_wheel(uint8_t index, int16_t delta, int16_t deadzone_distance, report_mouse_t *mouse_report) {

    /* Ignore a wheel while its button is held down, like the stock firmware. */
    if( matrix_is_on(0, wheel_button_col[index]) ) {
        wheel_key_tick[index] = 0;
        return;
    }

    bool reversed;
    wheel_role_t role = wheel_role(index, &reversed);

    /* ---- Key mode: tap the encoder-map keycode every tick. ---- */
    if( role == WHEEL_ROLE_KEYS ) {
        wheel_key_tick[index] += delta;
        if( wheel_key_tick[index] > TMAG5273_VOLUME_SCROLL_TICK_SIZE ) {
            encoder_queue_event(index, true);
            wheel_key_tick[index] = 0;
        }
        else if( wheel_key_tick[index] < -TMAG5273_VOLUME_SCROLL_TICK_SIZE ) {
            encoder_queue_event(index, false);
            wheel_key_tick[index] = 0;
        }
        return;
    }

    wheel_key_tick[index] = 0;
    if( reversed ) {
        delta = -delta;
    }
    bool vertical = (role == WHEEL_ROLE_VSCROLL);

    /* ---- Arrow mode (toggled on the control layer). Same on every OS. ---- */
    if( vertical ? user_config.vertical_scroll_arrows : user_config.horizontal_scroll_arrows ) {
        int16_t tick_size = vertical ? TMAG5273_VERT_SCROLL_TICK_SIZE : TMAG5273_HORIZ_SCROLL_TICK_SIZE;

        wheel_arrow_tick[index] += delta;
        if( wheel_arrow_tick[index] > tick_size ) {
            tap_code(vertical ? KC_DOWN : KC_RIGHT);
            wheel_arrow_tick[index] = 0;
        }
        else if( wheel_arrow_tick[index] < -tick_size ) {
            tap_code(vertical ? KC_UP : KC_LEFT);
            wheel_arrow_tick[index] = 0;
        }
        return;
    }

    /* ---- Hi-res scroll events on Windows and Linux. ---- */
    if ( detected_host_os() == OS_WINDOWS || detected_host_os() == OS_LINUX ) {
        if ( deadzone_distance > (TMAG5273_WHEEL_DEADZONE - 10) ||
             deadzone_distance < (-TMAG5273_WHEEL_DEADZONE + 10) ) {
            if( vertical ) {
                mouse_report->v += -delta / TMAG5273_VERTICAL_WHEEL_SPEED_DIV;
            }
            else {
                mouse_report->h += delta / TMAG5273_HORIZONAL_WHEEL_SPEED_DIV;
            }
        }
        return;
    }

    /* ---- Regular scroll events on other OSes. ----
       Certain operating systems, like MacOS, don't play well with the
       high-res scrolling implementation. For more details, see:
       https://github.com/qmk/qmk_firmware/issues/17585#issuecomment-2325248167 */
    wheel_lowres_tick[index] += delta;
    if( wheel_lowres_tick[index] > TMAG5273_LOWRES_TICK_SIZE ) {
        if( vertical ) { mouse_report->v -= 1; } else { mouse_report->h += 1; }
        wheel_lowres_tick[index] = 0;
    }
    else if( wheel_lowres_tick[index] < -TMAG5273_LOWRES_TICK_SIZE ) {
        if( vertical ) { mouse_report->v += 1; } else { mouse_report->h -= 1; }
        wheel_lowres_tick[index] = 0;
    }
}

report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {
    // throttle reads
    if (timer_elapsed32(last_scroll_time) > 10) {
        uint16_t leftwheel_rawangle = tmag5273_get_angle(TMAG5273_D0_I2C_ADDRESS);
        uint16_t rightwheel_rawangle = tmag5273_get_angle(TMAG5273_D1_I2C_ADDRESS);

        int16_t leftwheel_deadzone_distance = calculate_deadzone_distance( leftwheel_rawangle, &leftwheel_deadzone_center );
        int16_t rightwheel_deadzone_distance = calculate_deadzone_distance( rightwheel_rawangle, &rightwheel_deadzone_center );

        int16_t leftwheel_delta = calculate_wheel_delta( leftwheel_rawangle, leftwheel_current_position );
        leftwheel_current_position = leftwheel_rawangle;

        int16_t rightwheel_delta = calculate_wheel_delta( rightwheel_rawangle, rightwheel_current_position );
        rightwheel_current_position = rightwheel_rawangle;

        /* If either wheel moved, we reset the timeout counter. Else, we start 
           counting inactivity. */
        if( abs(leftwheel_delta) > 2 ) { 
            leftwheel_timeout = 0;
        }
        else {
            /* Don't increment forever, you'll get rollover. */
            if( leftwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT ) {
                leftwheel_timeout += timer_elapsed32(last_scroll_time);
            }
        }
        if( abs(rightwheel_delta) > 2 ) { 
            rightwheel_timeout = 0; 
        }
        else {
            /* Don't increment forever, you'll get rollover. */
            if( rightwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT ) {
                rightwheel_timeout += timer_elapsed32(last_scroll_time);
            }
        }

        /* Reset the counter so we know when to next perform all this scroll processing. */
        last_scroll_time = timer_read32();

        /* If either of the wheels times out, we move the deadzone window to
           where the position is. This prevents spurious scroll events. */
        if( leftwheel_timeout >= TMAG5273_DEADZONE_PROTECTOR_TIMEOUT && 
            leftwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 2 ) {

            leftwheel_deadzone_center = leftwheel_current_position;
            dprintf("Left wheel timeout, deadzone moved to: %d\n", leftwheel_deadzone_center);
            
            /* Set the timeout to a large number so that we only shift the window once. */
            leftwheel_timeout = TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 4;
        }
        if( rightwheel_timeout >= TMAG5273_DEADZONE_PROTECTOR_TIMEOUT && 
            rightwheel_timeout < TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 2 ) {

            rightwheel_deadzone_center = rightwheel_current_position;
            dprintf("Right wheel timeout, deadzone moved to: %d\n", rightwheel_deadzone_center);
            
            /* Set the timeout to a large number so that we only shift the window once. */
            rightwheel_timeout = TMAG5273_DEADZONE_PROTECTOR_TIMEOUT * 4;
        }

        process_wheel(0, leftwheel_delta,  leftwheel_deadzone_distance,  &mouse_report);
        process_wheel(1, rightwheel_delta, rightwheel_deadzone_distance, &mouse_report);
    }

    return mouse_report;
}
