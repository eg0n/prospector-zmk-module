/*
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

/* Crystal layout, ported from the zmk-dongle-screen custom status screen: WPM
 * top-left, connection top-right, modifiers left, the animated crystal right,
 * peripheral batteries bottom-left and the active layer bottom-right. */

#include <lvgl.h>

#include "animation.h"
#include "battery.h"
#include "connection.h"
#include "layer.h"
#include "modifiers.h"
#include "wpm.h"

#if CONFIG_PROSPECTOR_CRYSTAL_WPM_ACTIVE
static struct zmk_widget_wpm wpm_widget;
#endif
#if CONFIG_PROSPECTOR_CRYSTAL_CONNECTION_ACTIVE
static struct zmk_widget_connection connection_widget;
#endif
#if CONFIG_PROSPECTOR_CRYSTAL_MODIFIER_ACTIVE
static struct zmk_widget_modifiers modifiers_widget;
#endif
#if CONFIG_PROSPECTOR_CRYSTAL_ANIMATION_ACTIVE
static struct zmk_widget_animation animation_widget;
#endif
#if CONFIG_PROSPECTOR_CRYSTAL_BATTERY_ACTIVE
static struct zmk_widget_battery battery_widget;
#endif
#if CONFIG_PROSPECTOR_CRYSTAL_LAYER_ACTIVE
static struct zmk_widget_layer layer_widget;
#endif

static lv_style_t global_style;

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, 255, LV_PART_MAIN);

    lv_style_init(&global_style);
    lv_style_set_text_color(&global_style, lv_color_white());
    lv_style_set_text_letter_space(&global_style, 1);
    lv_style_set_text_line_space(&global_style, 1);
    lv_obj_add_style(screen, &global_style, LV_PART_MAIN);

#if CONFIG_PROSPECTOR_CRYSTAL_WPM_ACTIVE
    zmk_widget_wpm_init(&wpm_widget, screen);
    lv_obj_align(zmk_widget_wpm_obj(&wpm_widget), LV_ALIGN_TOP_LEFT, 25, 20);
#endif

#if CONFIG_PROSPECTOR_CRYSTAL_CONNECTION_ACTIVE
    zmk_widget_connection_init(&connection_widget, screen);
    lv_obj_align(zmk_widget_connection_obj(&connection_widget), LV_ALIGN_TOP_RIGHT, -25, 20);
#endif

#if CONFIG_PROSPECTOR_CRYSTAL_MODIFIER_ACTIVE
    zmk_widget_modifiers_init(&modifiers_widget, screen);
    lv_obj_align(zmk_widget_modifiers_obj(&modifiers_widget), LV_ALIGN_LEFT_MID, 20, -10);
#endif

#if CONFIG_PROSPECTOR_CRYSTAL_ANIMATION_ACTIVE
    zmk_widget_animation_init(&animation_widget, screen);
    lv_obj_align(zmk_widget_animation_obj(&animation_widget), LV_ALIGN_RIGHT_MID, -35, 0);
#endif

#if CONFIG_PROSPECTOR_CRYSTAL_BATTERY_ACTIVE
    zmk_widget_battery_init(&battery_widget, screen);
    lv_obj_align(zmk_widget_battery_obj(&battery_widget), LV_ALIGN_BOTTOM_LEFT, 20, -20);
#endif

#if CONFIG_PROSPECTOR_CRYSTAL_LAYER_ACTIVE
    zmk_widget_layer_init(&layer_widget, screen);
    lv_obj_align(zmk_widget_layer_obj(&layer_widget), LV_ALIGN_BOTTOM_RIGHT, -5, -35);
#endif

    return screen;
}
