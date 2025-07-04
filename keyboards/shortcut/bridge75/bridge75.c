// 05-15-2025 @devex modifications
// Copyright 2024 Wind (@yelishang)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#ifdef WIRELESS_ENABLE
#    include "wireless.h"
#endif

typedef union {
    uint32_t raw;
    struct {
        uint8_t flag : 1;
        uint8_t devs : 3;
    };
} confinfo_t;
confinfo_t confinfo;

uint32_t post_init_timer = 0x00;

void eeconfig_confinfo_update(uint32_t raw) {

    eeconfig_update_kb(raw);
}

uint32_t eeconfig_confinfo_read(void) {

    return eeconfig_read_kb();
}

void eeconfig_confinfo_default(void) {

    confinfo.flag = true;
#ifdef WIRELESS_ENABLE
    confinfo.devs = DEVS_USB;
#endif

    eeconfig_confinfo_update(confinfo.raw);
}

void eeconfig_confinfo_init(void) {

    confinfo.raw = eeconfig_confinfo_read();
    if (!confinfo.raw) {
        eeconfig_confinfo_default();
    }
}

void keyboard_post_init_kb(void) {

#ifdef CONSOLE_ENABLE
    // can toggle debug with CTL_LAYR debug keycode, so don't need it enabled here
    //debug_enable = true;
#endif

    eeconfig_confinfo_init();

#ifdef LED_POWER_EN_PIN
    gpio_set_pin_output(LED_POWER_EN_PIN);
    gpio_write_pin_low(LED_POWER_EN_PIN);
#endif

#ifdef USB_POWER_EN_PIN
    gpio_write_pin_low(USB_POWER_EN_PIN);
    gpio_set_pin_output(USB_POWER_EN_PIN);
#endif

#ifdef WIRELESS_ENABLE
    wireless_init();
    wireless_devs_change(!confinfo.devs, confinfo.devs, false);
    post_init_timer = timer_read32();
#endif

    keyboard_post_init_user();
}

#ifdef WIRELESS_ENABLE

void usb_power_connect(void) {

#    ifdef USB_POWER_EN_PIN
    gpio_write_pin_low(USB_POWER_EN_PIN);
#    endif
}

void usb_power_disconnect(void) {

#    ifdef USB_POWER_EN_PIN
    gpio_write_pin_high(USB_POWER_EN_PIN);
#    endif
}

void suspend_power_down_kb(void) {

#    ifdef LED_POWER_EN_PIN
    gpio_write_pin_high(LED_POWER_EN_PIN);
#    endif
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {

#    ifdef LED_POWER_EN_PIN
    gpio_write_pin_low(LED_POWER_EN_PIN);
#    endif

    wireless_devs_change(wireless_get_current_devs(), wireless_get_current_devs(), false);
    suspend_wakeup_init_user();
}

void wireless_post_task(void) {

    // auto switching devs
    if (post_init_timer && timer_elapsed32(post_init_timer) >= 100) {
        md_send_devctrl(MD_SND_CMD_DEVCTRL_FW_VERSION);   // get the module fw version.
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_BT_EN);  // timeout 30min to sleep in bt mode, enable
        md_send_devctrl(MD_SND_CMD_DEVCTRL_SLEEP_2G4_EN); // timeout 30min to sleep in 2.4g mode, enable
        wireless_devs_change(!confinfo.devs, confinfo.devs, false);
        post_init_timer = 0x00;
    }
}

uint32_t wls_process_long_press(uint32_t trigger_time, void *cb_arg) {
    uint16_t keycode = *((uint16_t *)cb_arg);

    switch (keycode) {
        case KC_BT1: {
            wireless_devs_change(wireless_get_current_devs(), DEVS_BT1, true);
        } break;
        case KC_BT2: {
            wireless_devs_change(wireless_get_current_devs(), DEVS_BT2, true);
        } break;
        case KC_BT3: {
            wireless_devs_change(wireless_get_current_devs(), DEVS_BT3, true);
        } break;
        case KC_2G4: {
            wireless_devs_change(wireless_get_current_devs(), DEVS_2G4, true);
        } break;
        default:
            break;
    }

    return 0;
}

// djc: adding this to see more detail about the current battery level
// battery level indicators 
typedef struct {
    uint8_t index;
    uint8_t delay;
} batlevel_indicator;

static batlevel_indicator blevel_indicators[10] = {
    { .index = 15, .delay = 1},
    { .index = 16, .delay = 2},
    { .index = 17, .delay = 3},
    { .index = 18, .delay = 4},
    { .index = 19, .delay = 5},
    { .index = 20, .delay = 6},
    { .index = 21, .delay = 7},
    { .index = 22, .delay = 8},
    { .index = 23, .delay = 9},
    { .index = 24, .delay = 10}
};

void rgb_matrix_wls_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times);
// djc: added for more precise batlevel tracking
void start_batlevel_indicators(uint8_t bat_level); 
 
bool process_record_wls(uint16_t keycode, keyrecord_t *record) {
    static uint16_t keycode_shadow                     = 0x00;
    static deferred_token wls_process_long_press_token = INVALID_DEFERRED_TOKEN;

    keycode_shadow = keycode;

#    ifndef WLS_KEYCODE_PAIR_TIME
#        define WLS_KEYCODE_PAIR_TIME 3000
#    endif

#    define WLS_KEYCODE_EXEC(wls_dev)                                                                                          \
        do {                                                                                                                   \
            if (record->event.pressed) {                                                                                       \
                wireless_devs_change(wireless_get_current_devs(), wls_dev, false);                                             \
                if (wls_process_long_press_token == INVALID_DEFERRED_TOKEN) {                                                  \
                    wls_process_long_press_token = defer_exec(WLS_KEYCODE_PAIR_TIME, wls_process_long_press, &keycode_shadow); \
                }                                                                                                              \
            } else {                                                                                                           \
                cancel_deferred_exec(wls_process_long_press_token);                                                            \
                wls_process_long_press_token = INVALID_DEFERRED_TOKEN;                                                         \
            }                                                                                                                  \
        } while (false)

    switch (keycode) {
        case KC_BT1: {
            WLS_KEYCODE_EXEC(DEVS_BT1);
        } break;
        case KC_BT2: {
            WLS_KEYCODE_EXEC(DEVS_BT2);
        } break;
        case KC_BT3: {
            WLS_KEYCODE_EXEC(DEVS_BT3);
        } break;
        case KC_2G4: {
            WLS_KEYCODE_EXEC(DEVS_2G4);
        } break;
        case KC_USB: {
            if (record->event.pressed) {
                rgb_matrix_wls_indicator_set(14, (RGB){RGB_BLUE}, 500, 2);
                wireless_devs_change(wireless_get_current_devs(), DEVS_USB, false);
            }
        } break;
        // @davex: added this battery check
        case KC_BATQ: {
            if (record->event.pressed) {
                uint8_t bat_level = *md_getp_bat(); 
                dprintf("bat level: %u\n", bat_level);
                start_batlevel_indicators(bat_level);
                if (bat_level > 55) {
                    rgb_matrix_wls_indicator_set(0, (RGB){RGB_GREEN}, 250, 8);
                }
                else if (bat_level > 50) {
                    rgb_matrix_wls_indicator_set(0, (RGB){RGB_YELLOW}, 250, 8);
                }
                else if (bat_level > 45) {
                    rgb_matrix_wls_indicator_set(0, (RGB){RGB_ORANGE}, 250, 8);
                }
                else {
                    rgb_matrix_wls_indicator_set(0, (RGB){RGB_RED}, 250, 8);
                }   
            }
        } break;
        default:
            return true;
    }

    return false;
}
#endif

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {

    if (process_record_user(keycode, record) != true) {
        return false;
    }

#ifdef WIRELESS_ENABLE
    if (process_record_wls(keycode, record) != true) {
        return false;
    }
#endif

    switch (keycode) {
        default:
            return true;
    }

    return false;
}

#ifdef RGB_MATRIX_ENABLE

#    ifdef WIRELESS_ENABLE
bool wls_rgb_indicator_reset        = false;
uint32_t wls_rgb_indicator_timer    = 0x00;
uint32_t wls_rgb_indicator_interval = 0;
uint32_t wls_rgb_indicator_times    = 0;
uint32_t wls_rgb_indicator_index    = 0;
RGB wls_rgb_indicator_rgb           = {0};
//djc added this for the detailed battery level
uint32_t batlevel_indicator_timer   = 0x00;
uint8_t batlevel_indicator_level    = 0;
void flash_batlevel_percentage(int key_index, int start, int end, int restart);

void rgb_matrix_wls_indicator_set(uint8_t index, RGB rgb, uint32_t interval, uint8_t times) {

    wls_rgb_indicator_timer = timer_read32();

    wls_rgb_indicator_index    = index;
    wls_rgb_indicator_interval = interval;
    wls_rgb_indicator_times    = times * 2;
    wls_rgb_indicator_rgb      = rgb;
}

void wireless_devs_change_kb(uint8_t old_devs, uint8_t new_devs, bool reset) {

    wls_rgb_indicator_reset = reset;

    if (confinfo.devs != wireless_get_current_devs()) {
        confinfo.devs = wireless_get_current_devs();
        eeconfig_confinfo_update(confinfo.raw);
    }

    switch (new_devs) {
        case DEVS_BT1: {
            if (reset) {
                rgb_matrix_wls_indicator_set(15, (RGB){RGB_BLUE}, 200, 4);
            } else {
                rgb_matrix_wls_indicator_set(15, (RGB){RGB_BLUE}, 500, 2);
            }
        } break;
        case DEVS_BT2: {
            if (reset) {
                rgb_matrix_wls_indicator_set(16, (RGB){RGB_BLUE}, 200, 4);
            } else {
                rgb_matrix_wls_indicator_set(16, (RGB){RGB_BLUE}, 500, 2);
            }
        } break;
        case DEVS_BT3: {
            if (reset) {
                rgb_matrix_wls_indicator_set(17, (RGB){RGB_BLUE}, 200, 4);
            } else {
                rgb_matrix_wls_indicator_set(17, (RGB){RGB_BLUE}, 500, 2);
            }
        } break;
        case DEVS_2G4: {
            if (reset) {
                rgb_matrix_wls_indicator_set(18, (RGB){RGB_BLUE}, 200, 4);
            } else {
                rgb_matrix_wls_indicator_set(18, (RGB){RGB_BLUE}, 500, 2);
            }
        } break;
        default:
            break;
    }
}

bool rgb_matrix_wls_indicator_cb(void) {

    if (*md_getp_state() != MD_STATE_CONNECTED) {
        wireless_devs_change_kb(wireless_get_current_devs(), wireless_get_current_devs(), wls_rgb_indicator_reset);
        return true;
    }

    // refresh led
    led_wakeup();

    return false;
}

void rgb_matrix_wls_indicator(void) {

    if (wls_rgb_indicator_timer) {

        if (timer_elapsed32(wls_rgb_indicator_timer) >= wls_rgb_indicator_interval) {
            wls_rgb_indicator_timer = timer_read32();

            if (wls_rgb_indicator_times) {
                wls_rgb_indicator_times--;
            }

            if (wls_rgb_indicator_times <= 0) {
                wls_rgb_indicator_timer = 0x00;
                if (rgb_matrix_wls_indicator_cb() != true) {
                    return;
                }
            }
        }

        if (wls_rgb_indicator_times % 2) {
            rgb_matrix_set_color(wls_rgb_indicator_index, wls_rgb_indicator_rgb.r, wls_rgb_indicator_rgb.g, wls_rgb_indicator_rgb.b);
        } else {
            rgb_matrix_set_color(wls_rgb_indicator_index, 0x00, 0x00, 0x00);
        }
        
        // djc: added for detailed battery level
        if (wls_rgb_indicator_index == 0 && batlevel_indicator_timer) {
            // only do the bar lights for the first 2.5 seconds
            if (timer_elapsed(batlevel_indicator_timer) < 2500) {
                // first clear out any rgb that could be on the indicators
                for (int i = 0; i < 10; i++) {
                    rgb_matrix_set_color(blevel_indicators[i].index, 0x00, 0x00, 0x00);
                }
                // now light them up according to the battery level reading
                for (int i = 0; i < 10 && i < batlevel_indicator_level / 10; i++) {
                    if (blevel_indicators[i].delay * 125 < timer_elapsed32(batlevel_indicator_timer)) {
                        rgb_matrix_set_color(blevel_indicators[i].index, wls_rgb_indicator_rgb.r, wls_rgb_indicator_rgb.g, wls_rgb_indicator_rgb.b);
                    }
                }
            }
            // fkeys start at index 1 and numkeys at index 15, but the 0 key is index 24
            uint8_t fkey_index = batlevel_indicator_level / 10;
            uint8_t numkey_index = (batlevel_indicator_level % 10 == 0 ? 24 : batlevel_indicator_level % 10 + 14);
            // now show the actual percentage using F keys and nubmer keys
            // this setup blinks the percentage indicators and makes them more legible
            flash_batlevel_percentage(fkey_index, 1750, 2000, 2250);
            flash_batlevel_percentage(numkey_index, 1800, 2000, 2300);
        }
    }
    // djc added more indicators here for currently connected device
    else {  
        switch (wireless_get_current_devs()) {
        case DEVS_USB: 
            rgb_matrix_set_color(14, 0x77, 0x77, 0x77);
            break;
        case DEVS_BT1: 
            rgb_matrix_set_color(15, 0x77, 0x77, 0x77);
            break;
        case DEVS_BT2: 
            rgb_matrix_set_color(16, 0x77, 0x77, 0x77);
            break;
        case DEVS_BT3: 
            rgb_matrix_set_color(17, 0x77, 0x77, 0x77);
            break;
        case DEVS_2G4: 
            rgb_matrix_set_color(18, 0x77, 0x77, 0x77);
            break;
        }
    }
}
// djc: added this fn to see more detailed battery level
void start_batlevel_indicators(uint8_t bat_level) {
    batlevel_indicator_timer = timer_read32();
    batlevel_indicator_level = bat_level;
}
// moved this to fn so that its simple to do different intervals for fkey and numkey without repeating code
void flash_batlevel_percentage(int key_index, int start, int end, int restart) {
    if (timer_elapsed32(batlevel_indicator_timer) > restart ||
        (timer_elapsed32(batlevel_indicator_timer) < end &&
         timer_elapsed32(batlevel_indicator_timer) > start)) {
        if (batlevel_indicator_level > 45) {
            rgb_matrix_set_color(key_index, 102, 178, 255);  
        }
        else {
            rgb_matrix_set_color(key_index, RGB_MAGENTA);
        }
    }
}
#    endif

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {

    if (rgb_matrix_indicators_advanced_user(led_min, led_max) != true) {
        return false;
    }

    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(44, 0x77, 0x77, 0x77);
    }

#    ifdef WIRELESS_ENABLE
    rgb_matrix_wls_indicator();
#    endif

    return true;
}


void md_devs_change(uint8_t devs, bool reset) {

    switch (devs) {
        case DEVS_USB: {
            md_send_devctrl(MD_SND_CMD_DEVCTRL_USB);
        } break;
        case DEVS_2G4: {
            if (reset) {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_PAIR);
            } else {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_2G4);
            }
        } break;
        case DEVS_BT1: {
            if (reset) {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_PAIR);
            } else {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_BT1);
            }
        } break;
        case DEVS_BT2: {
            if (reset) {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_PAIR);
            } else {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_BT2);
            }
        } break;
        case DEVS_BT3: {
            if (reset) {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_PAIR);
            } else {
                md_send_devctrl(MD_SND_CMD_DEVCTRL_BT3);
            }
        } break;
        default:
            break;
    }
}

#endif

void wireless_send_nkro(report_nkro_t *report) {
    static report_keyboard_t temp_report_keyboard = {0};
    uint8_t wls_report_nkro[MD_SND_CMD_NKRO_LEN]  = {0};

#ifdef NKRO_ENABLE

    if (report != NULL) {
        report_nkro_t temp_report_nkro = *report;
        uint8_t key_count              = 0;

        temp_report_keyboard.mods = temp_report_nkro.mods;
        for (uint8_t i = 0; i < NKRO_REPORT_BITS; i++) {
            key_count += __builtin_popcount(temp_report_nkro.bits[i]);
        }

        /*
         * Use NKRO for sending when more than 6 keys are pressed
         * to solve the issue of the lack of a protocol flag in wireless mode.
         */

        for (uint8_t i = 0; i < key_count; i++) {
            uint8_t usageid;
            uint8_t idx, n = 0;

            for (n = 0; n < NKRO_REPORT_BITS && !temp_report_nkro.bits[n]; n++) {}
            usageid = (n << 3) | biton(temp_report_nkro.bits[n]);
            del_key_bit(&temp_report_nkro, usageid);

            for (idx = 0; idx < WLS_KEYBOARD_REPORT_KEYS; idx++) {
                if (temp_report_keyboard.keys[idx] == usageid) {
                    goto next;
                }
            }

            for (idx = 0; idx < WLS_KEYBOARD_REPORT_KEYS; idx++) {
                if (temp_report_keyboard.keys[idx] == 0x00) {
                    temp_report_keyboard.keys[idx] = usageid;
                    break;
                }
            }
        next:
            if (idx == WLS_KEYBOARD_REPORT_KEYS && (usageid < (MD_SND_CMD_NKRO_LEN * 8))) {
                wls_report_nkro[usageid / 8] |= 0x01 << (usageid % 8);
            }
        }

        temp_report_nkro = *report;

         // find key up and del it.
        uint8_t nkro_keys = key_count;
        for (uint8_t i = 0; i < WLS_KEYBOARD_REPORT_KEYS; i++) {
            report_nkro_t found_report_nkro;
            uint8_t usageid = 0x00;
            uint8_t n;

            found_report_nkro = temp_report_nkro;

            for (uint8_t c = 0; c < nkro_keys; c++) {
                for (n = 0; n < NKRO_REPORT_BITS && !found_report_nkro.bits[n]; n++) {}
                usageid = (n << 3) | biton(found_report_nkro.bits[n]);
                del_key_bit(&found_report_nkro, usageid);
                if (usageid == temp_report_keyboard.keys[i]) {
                    del_key_bit(&temp_report_nkro, usageid);
                    nkro_keys--;
                    break;
                }
            }

            if (usageid != temp_report_keyboard.keys[i]) {
                temp_report_keyboard.keys[i] = 0x00;
            }
        }
    } else {
        memset(&temp_report_keyboard, 0, sizeof(temp_report_keyboard));
    }
#endif
    void wireless_task(void);
    bool smsg_is_busy(void);
    while(smsg_is_busy()) {
        wireless_task();
    }
    extern host_driver_t wireless_driver;
    wireless_driver.send_keyboard(&temp_report_keyboard);
    md_send_nkro(wls_report_nkro);
}
