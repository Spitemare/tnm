#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"
#include "enamel.h"
#include "colors.h"

static bool s_connected;
static EventHandle s_connection_event_handle;

static void connection_handler(bool connected) {
    log_func();
    s_connected = connected;
}

void colors_init(void) {
    log_func();

    s_connected = connection_service_peek_pebble_app_connection();
    s_connection_event_handle = events_connection_service_subscribe((ConnectionHandlers) {
        .pebble_app_connection_handler = connection_handler
    });
}

void colors_deinit(void) {
    log_func();
    events_connection_service_unsubscribe(s_connection_event_handle);
}

GColor get_background_color(void) {
    log_func();
    if (!s_connected) return GColorDarkGray;
#ifdef PBL_COLOR
    return enamel_get_COLOR_BACKGROUND();
#else
    return enamel_get_COLOR_INVERT() ? GColorWhite : GColorBlack;
#endif
}

GColor get_foreground_color(void) {
    log_func();
    if (!s_connected) return GColorDarkGray;
#ifdef PBL_COLOR
    return gcolor_legible_over(get_background_color());
#else
    return enamel_get_COLOR_INVERT() ? GColorBlack : GColorWhite;
#endif
}
