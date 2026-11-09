#include "battery_bar.h"

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

struct battery_update_state {
    uint8_t source;
    uint8_t level;
};

struct connection_update_state {
    uint8_t source;
    bool connected;
};

static void set_battery_bar_value(lv_obj_t *widget_obj, struct battery_update_state state, bool is_initialized) {
    if (!is_initialized || state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return;
    }

    lv_obj_t *info_container = lv_obj_get_child(widget_obj, state.source);
    if (!info_container) {
        return;
    }

    lv_obj_t *bar = lv_obj_get_child(info_container, 0);
    lv_obj_t *num = lv_obj_get_child(info_container, 1);
    if (!bar || !num) {
        return;
    }

    lv_bar_set_value(bar, state.level, LV_ANIM_ON);
    lv_label_set_text_fmt(num, "%d", state.level);

    if (state.level < 20) {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0xD3900F), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xE8AC11), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x6E4E07), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xFFB802), 0);
    } else {
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_text_color(num, lv_color_hex(0xFFFFFF), 0);
    }
}

static void set_battery_bar_connected(lv_obj_t *widget_obj, struct connection_update_state state, bool is_initialized) {
    if (!is_initialized || state.source >= ZMK_SPLIT_BLE_PERIPHERAL_COUNT) {
        return;
    }

    lv_obj_t *info_container = lv_obj_get_child(widget_obj, state.source);
    if (!info_container) {
        return;
    }

    lv_obj_t *bar = lv_obj_get_child(info_container, 0);
    lv_obj_t *num = lv_obj_get_child(info_container, 1);
    lv_obj_t *nc_bar = lv_obj_get_child(info_container, 2);
    lv_obj_t *nc_num = lv_obj_get_child(info_container, 3);
    if (!bar || !num || !nc_bar || !nc_num) {
        return;
    }

    // Prevent animation stacking on rapid connection changes
    lv_anim_del(bar, NULL);
    lv_anim_del(num, NULL);
    lv_anim_del(nc_bar, NULL);
    lv_anim_del(nc_num, NULL);

    if (state.connected) {
        lv_obj_fade_out(nc_bar, 150, 0);
        lv_obj_fade_out(nc_num, 150, 0);
        lv_obj_fade_in(bar, 150, 250);
        lv_obj_fade_in(num, 150, 250);
    } else {
        lv_obj_fade_out(bar, 150, 0);
        lv_obj_fade_out(num, 150, 0);
        lv_obj_fade_in(nc_bar, 150, 250);
        lv_obj_fade_in(nc_num, 150, 250);
    }
}

struct peripherals_state {
    uint8_t level[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
    bool connected[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
};

/* What's on screen, so unchanged peripherals don't restart their fades. */
static int16_t drawn_level[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];
static int8_t drawn_connected[ZMK_SPLIT_BLE_PERIPHERAL_COUNT];

static void peripherals_update_cb(struct peripherals_state state) {
    for (uint8_t i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        bool level_changed = drawn_level[i] != state.level[i];
        bool connected_changed = drawn_connected[i] != state.connected[i];
        struct zmk_widget_battery_bar *widget;

        SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
            if (level_changed) {
                set_battery_bar_value(widget->obj, (struct battery_update_state){i, state.level[i]},
                                      widget->initialized);
            }
            if (connected_changed) {
                set_battery_bar_connected(
                    widget->obj, (struct connection_update_state){i, state.connected[i]},
                    widget->initialized);
            }
        }

        drawn_level[i] = state.level[i];
        drawn_connected[i] = state.connected[i];
    }
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

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_bar_peripherals, struct peripherals_state,
                            peripherals_update_cb, get_peripherals_state);
ZMK_SUBSCRIPTION(widget_battery_bar_peripherals, zmk_peripheral_battery_state_changed);
ZMK_SUBSCRIPTION(widget_battery_bar_peripherals, zmk_split_central_status_changed);

int zmk_widget_battery_bar_init(struct zmk_widget_battery_bar *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_width(widget->obj, lv_pct(100));
    lv_obj_set_flex_flow(widget->obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(widget->obj, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(widget->obj, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(widget->obj, 16, LV_PART_MAIN);

    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        lv_obj_t *info_container = lv_obj_create(widget->obj);
        lv_obj_center(info_container);
        lv_obj_set_height(info_container, lv_pct(100));
        lv_obj_set_flex_grow(info_container, 1);

        lv_obj_t *bar = lv_bar_create(info_container);
        lv_obj_set_size(bar, lv_pct(100), 4);
        lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x202020), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(0x909090), LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, 255, LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_color(bar, lv_color_hex(0xf0f0f0), LV_PART_INDICATOR);
        lv_obj_set_style_bg_grad_dir(bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 1, LV_PART_INDICATOR);
        lv_obj_set_style_anim_time(bar, 250, 0);

        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        lv_obj_set_style_opa(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_opa(bar, 255, LV_PART_INDICATOR);

        lv_obj_t *num = lv_label_create(info_container);
        lv_obj_set_style_text_font(num, &FG_Medium_20, 0);
        lv_obj_set_style_text_color(num, lv_color_white(), 0);
        lv_obj_set_style_opa(num, 255, 0);
        lv_obj_align(num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(num, "N/A");

        lv_obj_set_style_opa(num, 0, 0);

        lv_obj_t *nc_bar = lv_obj_create(info_container);
        lv_obj_set_size(nc_bar, lv_pct(100), 4);
        lv_obj_align(nc_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_bg_color(nc_bar, lv_color_hex(0x9e2121), LV_PART_MAIN);
        lv_obj_set_style_radius(nc_bar, 1, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(nc_bar, 255, 0);

        lv_obj_t *nc_num = lv_label_create(info_container);
        lv_obj_set_style_text_color(nc_num, lv_color_hex(0xe63030), 0);
        lv_obj_align(nc_num, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(nc_num, LV_SYMBOL_CLOSE);
        lv_obj_set_style_opa(nc_num, 255, 0);
    }

    sys_slist_append(&widgets, &widget->node);
    widget->initialized = true;
    for (int i = 0; i < ZMK_SPLIT_BLE_PERIPHERAL_COUNT; i++) {
        drawn_level[i] = -1;
        drawn_connected[i] = -1;
    }
    widget_battery_bar_peripherals_init();

    return 0;
}

lv_obj_t *zmk_widget_battery_bar_obj(struct zmk_widget_battery_bar *widget) { return widget->obj; }

static void set_battery_bar_width(void *obj, int32_t width) {
    lv_obj_set_width(obj, width);
}

void zmk_widget_battery_bar_set_compact(bool compact) {
    struct zmk_widget_battery_bar *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, widget->obj);
        lv_anim_set_exec_cb(&a, set_battery_bar_width);
        lv_anim_set_time(&a, 200);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_out);

        int32_t current_width = lv_obj_get_width(widget->obj);
        int32_t target_width = compact ? 227 : 280;

        lv_anim_set_values(&a, current_width, target_width);
        lv_anim_start(&a);
    }
}