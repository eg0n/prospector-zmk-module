#include "battery_circles.h"

#include <zmk/display.h>
#include <zmk/battery.h>
#include <zmk/ble.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/events/split_central_status_changed.h>
#include <zmk/prospector/peripheral_status.h>
#include <zmk/split/central.h>
#include <zmk/event_manager.h>

#include <fonts.h>
#include "display_colors.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

#ifndef PERIPHERAL_COUNT
#define PERIPHERAL_COUNT ZMK_SPLIT_BLE_PERIPHERAL_COUNT
#endif

static lv_obj_t *peripheral_arcs[PERIPHERAL_COUNT];
static lv_obj_t *peripheral_label;

struct peripherals_state {
    uint8_t level[PERIPHERAL_COUNT];
    bool connected[PERIPHERAL_COUNT];
};

static bool widgets_ready;

static void update_peripheral_display(uint8_t source, uint8_t level, bool connected) {
    int max_displayed = PERIPHERAL_COUNT > 3 ? 3 : PERIPHERAL_COUNT;
    if (source >= max_displayed) {
        return;
    }

    lv_obj_t *arc = peripheral_arcs[source];
    if (!arc) {
        return;
    }

    lv_arc_set_value(arc, connected ? level : 0);
    lv_obj_set_style_arc_color(arc,
        lv_color_hex(connected ? DISPLAY_COLOR_ARC_INDICATOR : DISPLAY_COLOR_ARC_BG),
        LV_PART_INDICATOR);

    if (PERIPHERAL_COUNT == 1 && peripheral_label) {
        if (connected && level > 0) {
            lv_label_set_text_fmt(peripheral_label, "%d", level);
        } else {
            lv_label_set_text(peripheral_label, "");
        }
    }
}

static uint8_t peripheral_battery[PERIPHERAL_COUNT];
static bool peripheral_connected[PERIPHERAL_COUNT];

static void peripherals_update_cb(struct peripherals_state state) {
    int max_displayed = PERIPHERAL_COUNT > 3 ? 3 : PERIPHERAL_COUNT;

    if (!widgets_ready) {
        return;
    }

    for (int i = 0; i < max_displayed; i++) {
        peripheral_battery[i] = state.level[i];
        peripheral_connected[i] = state.connected[i];
        update_peripheral_display(i, state.level[i], state.connected[i]);
    }
}

/* Snapshot every peripheral: the display listener keeps one pending state,
 * so of two events close together only the last would be drawn. */
static struct peripherals_state get_peripherals_state(const zmk_event_t *eh) {
    struct peripherals_state state = {0};

    for (uint8_t i = 0; i < PERIPHERAL_COUNT; i++) {
        zmk_split_central_get_peripheral_battery_level(i, &state.level[i]);
        state.connected[i] = prospector_peripheral_connected(i);
    }

    return state;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_circles_peripherals, struct peripherals_state,
                            peripherals_update_cb, get_peripherals_state);
ZMK_SUBSCRIPTION(widget_battery_circles_peripherals, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_circles_peripherals, zmk_split_central_status_changed);

static lv_obj_t *create_arc(lv_obj_t *parent, int size, int x, int y, int width) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, x, y);

    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_rotation(arc, 270);

    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(DISPLAY_COLOR_ARC_BG), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);

    lv_obj_set_style_arc_width(arc, width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, lv_color_hex(DISPLAY_COLOR_ARC_BG), LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);

    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    return arc;
}

int zmk_widget_battery_circles_init(struct zmk_widget_battery_circles *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 108, 62);
    lv_obj_set_style_bg_color(widget->obj, lv_color_hex(DISPLAY_COLOR_BATTERY_PANEL_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(widget->obj, 255, LV_PART_MAIN);
    lv_obj_set_style_radius(widget->obj, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);

    if (PERIPHERAL_COUNT == 1) {
        int arc_size = 32;
        peripheral_arcs[0] = create_arc(widget->obj, arc_size, 17, (62 - arc_size) / 2, 6);

        peripheral_label = lv_label_create(widget->obj);
        lv_label_set_text(peripheral_label, "");
        lv_obj_set_style_text_font(peripheral_label, &Symbols_Medium_28, LV_PART_MAIN);
        lv_obj_set_style_text_color(peripheral_label, lv_color_hex(DISPLAY_COLOR_ARC_INDICATOR), LV_PART_MAIN);
        lv_obj_set_style_text_align(peripheral_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
        lv_obj_align(peripheral_label, LV_ALIGN_RIGHT_MID, -19, -1);

    } else if (PERIPHERAL_COUNT == 2) {
        int arc_size = 30;
        int left_pad = 19;
        int arc_gap = 10;
        int y_center = (62 - arc_size) / 2;

        for (int i = 0; i < 2; i++) {
            peripheral_arcs[i] = create_arc(widget->obj, arc_size, left_pad + i * (arc_size + arc_gap), y_center, 6);
        }

    } else {
        int arc_size = 24;
        int left_pad = 12;
        int arc_gap = 5;
        int y_center = (62 - arc_size) / 2;
        int max_displayed = PERIPHERAL_COUNT > 3 ? 3 : PERIPHERAL_COUNT;

        for (int i = 0; i < max_displayed; i++) {
            peripheral_arcs[i] = create_arc(widget->obj, arc_size, left_pad + i * (arc_size + arc_gap), y_center, 4);
        }
    }

    widget->initialized = true;
    sys_slist_append(&widgets, &widget->node);

    /* Draw what's already known, including links that predate the display. */
    widgets_ready = true;
    widget_battery_circles_peripherals_init();

    return 0;
}

lv_obj_t *zmk_widget_battery_circles_obj(struct zmk_widget_battery_circles *widget) {
    return widget->obj;
}
