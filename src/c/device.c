#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include "logging.h"
#include "fonts.h"
#include "enamel.h"
#include "colors.h"
#include "minute_layer.h"
#include "hour_layer.h"
#include "battery_layer.h"

static Window *s_window;
static MinuteLayer *s_minute_layer;
static HourLayer *s_hour_layer;
static BatteryLayer *s_battery_layer;

static EventHandle s_settings_event_handle;
static EventHandle s_connection_event_handle;

static void settings_handler(void *context) {
    log_func();
    window_set_background_color(s_window, get_background_color());
    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());
#ifdef PBL_HEALTH
    connection_vibes_enable_health(enamel_get_ENABLE_HEALTH());
    hourly_vibes_enable_health(enamel_get_ENABLE_HEALTH());
#endif
}

static void connection_handler(bool connected, void *context) {
    log_func();
    layer_mark_dirty(context);
}

static void window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);
#ifdef PBL_RECT
    int16_t diff = bounds.size.h - bounds.size.w;
    bounds = GRect(bounds.origin.x - diff, bounds.origin.y, bounds.size.w + diff, bounds.size.h);
#endif

    s_minute_layer = minute_layer_create(bounds);
    layer_add_child(root_layer, s_minute_layer);

    s_hour_layer = hour_layer_create(bounds);
    layer_add_child(root_layer, s_hour_layer);

    s_battery_layer = battery_layer_create(bounds);
    layer_add_child(root_layer, s_battery_layer);

    settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(settings_handler, NULL);

    s_connection_event_handle = events_connection_service_subscribe_context((EventConnectionHandlers) {
        .pebble_app_connection_handler = connection_handler
    }, root_layer);
}

static void window_unload(Window *window) {
    log_func();
    events_connection_service_unsubscribe(s_connection_event_handle);
    enamel_settings_received_unsubscribe(s_settings_event_handle);

    battery_layer_destroy(s_battery_layer);
    hour_layer_destroy(s_hour_layer);
    minute_layer_destroy(s_minute_layer);
}

static void init(void) {
    log_func();
    enamel_init();
    fonts_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });
    colors_init();

    events_app_message_open();

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

    colors_deinit();
    hourly_vibes_deinit();
    connection_vibes_deinit();
    fonts_deinit();
    enamel_deinit();
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
