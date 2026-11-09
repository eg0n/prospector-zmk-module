/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

/* With zmk-split-esb as the split transport there are no BLE connections
 * to watch, so map its per-peripheral link events onto the same
 * split_central_status_changed events the layouts already use. */

#include <zephyr/kernel.h>

#include <zmk/event_manager.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk_split_esb/events/peripheral_link_changed.h>

static int esb_link_listener(const zmk_event_t *eh) {
    const struct zmk_split_esb_peripheral_link_changed *ev =
        as_zmk_split_esb_peripheral_link_changed(eh);

    if (ev) {
        raise_zmk_split_central_status_changed((struct zmk_split_central_status_changed){
            .slot = ev->source,
            .connected = ev->connected,
        });
    }

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(prospector_esb_link, esb_link_listener);
ZMK_SUBSCRIPTION(prospector_esb_link, zmk_split_esb_peripheral_link_changed);
