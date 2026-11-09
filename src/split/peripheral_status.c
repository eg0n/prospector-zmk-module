/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>

#include <zmk/events/split_central_status_changed.h>
#include <zmk/prospector/peripheral_status.h>

static bool connected[8];

void prospector_peripheral_status_set(uint8_t slot, bool is_connected) {
    if (slot < ARRAY_SIZE(connected)) {
        connected[slot] = is_connected;
    }

    raise_zmk_split_central_status_changed(
        (struct zmk_split_central_status_changed){.slot = slot, .connected = is_connected});
}

bool prospector_peripheral_connected(uint8_t slot) {
    return slot < ARRAY_SIZE(connected) && connected[slot];
}
