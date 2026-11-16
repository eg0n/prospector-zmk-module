#pragma once

/* Caps Lock as the host has it: its LED, from ZMK's HID indicators for the
 * current output, so it is right whichever keyboard or host toggled it. The
 * layouts refresh on zmk_hid_indicators_changed, raised on LED reports and on
 * output changes. Red, as in the KeyBeacon Mac app. */

#include <stdbool.h>

#define PROSPECTOR_CAPS_LOCK_COLOR 0xE5484D

#if IS_ENABLED(CONFIG_ZMK_HID_INDICATORS)
#include <dt-bindings/zmk/hid_indicators.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/hid_indicators.h>

static inline bool prospector_caps_lock_active(void) {
    return (zmk_hid_indicators_get_current_profile() & HID_INDICATOR_CAPS_LOCK) != 0;
}
#else
static inline bool prospector_caps_lock_active(void) { return false; }
#endif
