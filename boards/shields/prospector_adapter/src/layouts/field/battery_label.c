#include "battery_label.h"

#include <stdio.h>
#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/prospector/peripheral_status.h>
#include <zmk/split/central.h>
#include <zmk/event_manager.h>

#include <fonts.h>

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static uint8_t battery_levels[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
static bool connected[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static void update_label_text(lv_obj_t *label) {
    static char buf[32];
    char *ptr = buf;

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        if (i > 0) {
            *ptr++ = '/';
        }
        if (connected[i]) {
            ptr += snprintf(ptr, sizeof(buf) - (ptr - buf), "%d", battery_levels[i]);
        } else {
            *ptr++ = '-';
        }
    }
    *ptr = '\0';

    lv_label_set_text(label, buf);
}

static void refresh_all_widgets(void) {
    struct zmk_widget_battery_label *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        update_label_text(widget->obj);
    }
}

struct peripherals_state {
    uint8_t level[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
    bool connected[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
};

static void peripherals_update_cb(struct peripherals_state state) {
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        battery_levels[i] = state.level[i];
        connected[i] = state.connected[i];
    }
    refresh_all_widgets();
}

/* Snapshot every peripheral: the display listener keeps one pending state,
 * so of two events close together only the last would be drawn. */
static struct peripherals_state get_peripherals_state(const zmk_event_t *eh) {
    struct peripherals_state state = {0};

    for (uint8_t i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        zmk_split_central_get_peripheral_battery_level(i, &state.level[i]);
        state.connected[i] = prospector_peripheral_connected(i);
    }

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_label_peripherals, struct peripherals_state,
                            peripherals_update_cb, get_peripherals_state);
ZMK_SUBSCRIPTION(widget_battery_label_peripherals, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_label_peripherals, zmk_split_central_status_changed);

int zmk_widget_battery_label_init(struct zmk_widget_battery_label *widget, lv_obj_t *parent) {
    widget->obj = lv_label_create(parent);

    lv_obj_set_style_text_font(widget->obj, &FR_Regular_30, 0);
    lv_obj_set_style_text_letter_space(widget->obj, -1, 0);
    lv_obj_set_style_text_color(widget->obj, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(widget->obj, LV_OPA_TRANSP, 0);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        battery_levels[i] = 0;
        connected[i] = false;
    }

    sys_slist_append(&widgets, &widget->node);

    widget_battery_label_peripherals_init();

    update_label_text(widget->obj);

    return 0;
}

lv_obj_t *zmk_widget_battery_label_obj(struct zmk_widget_battery_label *widget) {
    return widget->obj;
}
