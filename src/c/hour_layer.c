#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"
#include "fonts.h"
#include "hour_layer.h"

typedef struct {
    FFont *font;
    uint8_t hour;
    EventHandle tick_timer_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);
    uint8_t hour = data->hour;

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    int16_t font_size = bounds.size.w / 10;
    fctx_set_text_em_height(&fctx, data->font, font_size);
    fctx_set_color_bias(&fctx, 0);
    fctx_set_fill_color(&fctx, GColorBlack);

    GRect rect = grect_crop(bounds, font_size * 1.4);
    for (int i = hour; i < 12 + hour; i ++) {
        fctx_begin_fill(&fctx);

        int32_t rot_angle = (i - hour) * TRIG_MAX_ANGLE / 12;
        fctx_set_rotation(&fctx, rot_angle);

        int32_t point_angle = (i + 15 - hour) * TRIG_MAX_ANGLE / 12;
        GPoint p = gpoint_from_polar(rect, GOvalScaleModeFitCircle, point_angle);
        fctx_set_offset(&fctx, g2fpoint(p));

        char s[3];
        snprintf(s, sizeof(s), "%02d", i > 12 ? i - 12 : i);
        fctx_draw_string(&fctx, s, data->font, GTextAlignmentRight, FTextAnchorMiddle);

        fctx_end_fill(&fctx);
        fctx_set_fill_color(&fctx, GColorDarkGray);
    }

    fctx_deinit_context(&fctx);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    data->hour = tick_time->tm_hour;
    if (data->hour > 12) data->hour -= 12;
    layer_mark_dirty(this);
}

HourLayer *hour_layer_create(GRect frame) {
    log_func();
    HourLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = fonts_get(RESOURCE_ID_LECO_FFONT);

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    tick_handler(tick_time, HOUR_UNIT, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(HOUR_UNIT, tick_handler, this);

    return this;
}

void hour_layer_destroy(HourLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    layer_destroy(this);
}
