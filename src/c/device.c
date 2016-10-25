#include <pebble.h>
#include "logging.h"
#include "fonts.h"
#include "minute_layer.h"
#include "hour_layer.h"
#include "battery_layer.h"

static Window *s_window;
static MinuteLayer *s_minute_layer;
static HourLayer *s_hour_layer;
static BatteryLayer *s_battery_layer;

static void window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    s_minute_layer = minute_layer_create(bounds);
    layer_add_child(root_layer, s_minute_layer);

    s_hour_layer = hour_layer_create(bounds);
    layer_add_child(root_layer, s_hour_layer);

    s_battery_layer = battery_layer_create(bounds);
    layer_add_child(root_layer, s_battery_layer);
}

static void window_unload(Window *window) {
    log_func();
    battery_layer_destroy(s_battery_layer);
    hour_layer_destroy(s_hour_layer);
    minute_layer_destroy(s_minute_layer);
}

static void init(void) {
    log_func();
    fonts_init();

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}

static void deinit(void) {
    log_func();
    window_destroy(s_window);

    fonts_deinit();
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
