#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"
#include "fonts.h"
#include "colors.h"
#include "hour_layer.h"

static const uint32_t TAP_TIMEOUT = 3000; // 3 seconds

typedef struct {
    FFont *font;
    int8_t value;
    bool animated;
    EventHandle tick_timer_event_handle;
    EventHandle tap_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);
    int8_t hour = data->value;

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    int16_t font_size = bounds.size.w / 10;
    fctx_set_text_em_height(&fctx, data->font, font_size);
    fctx_set_color_bias(&fctx, 0);
    fctx_set_fill_color(&fctx, get_foreground_color());

    FPoint offset = FPointI(PBL_IF_RECT_ELSE(-1.5 * font_size, 0), 0);
    GRect rect = grect_crop(bounds, font_size * 1.4);
    for (int i = hour; i > hour - 60; i--) {
        if (i % 5 == 0) {
            fctx_begin_fill(&fctx);

            int32_t rot_angle = (i - hour) * TRIG_MAX_ANGLE / 60;
            fctx_set_rotation(&fctx, rot_angle);

            int32_t point_angle = (i + 15 - hour) * TRIG_MAX_ANGLE / 60;
            GPoint p = gpoint_from_polar(rect, PBL_IF_RECT_ELSE(GOvalScaleModeFillCircle, GOvalScaleModeFitCircle), point_angle);
            fctx_set_offset(&fctx, fpoint_add(offset, g2fpoint(p)));

            char s[3];
            int j = i <= 0 ? i + 60 : i;
            snprintf(s, sizeof(s), "%02d", j / 5);
            fctx_draw_string(&fctx, s, data->font, GTextAlignmentRight, FTextAnchorMiddle);

            fctx_end_fill(&fctx);
        }
#ifdef PBL_COLOR
        fctx_set_color_bias(&fctx, -3);
#else
        fctx_set_fill_color(&fctx, GColorDarkGray);
#endif
    }

    fctx_deinit_context(&fctx);
}

static void value_setter(void *subject, int16_t value) {
    log_func();
    ((Data *) layer_get_data(subject))->value = value;
    layer_mark_dirty(subject);
}

static int16_t value_getter(void *subject) {
    log_func();
    return ((Data *) layer_get_data(subject))->value;
}


static const PropertyAnimationImplementation animation_impl = {
    .base = {
        .update = (AnimationUpdateImplementation)  property_animation_update_int16
    },
    .accessors = {
        .setter = { .int16 = value_setter },
        .getter = { .int16 = value_getter }
    }
};

static void timer_callback(void *context) {
    log_func();
    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    static int16_t to;
    memcpy(&to, &tick_time->tm_hour, sizeof(int));
#ifdef DEMO
        to = 12;
#endif
    to = to > 12 ? to - 12 : to;
    to *= 5;

    PropertyAnimation *animation = property_animation_create(&animation_impl, context, NULL, NULL);
    property_animation_set_to_int16(animation, &to);
    animation_schedule(property_animation_get_animation(animation));

    Data *data = layer_get_data(context);
    data->animated = false;
}

static void animation_stopped_handler(Animation *animation, bool finished, void *context) {
    log_func();
    app_timer_register(TAP_TIMEOUT, timer_callback, context);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    if (!data->animated) {
        static int16_t to = 60;
        PropertyAnimation *a1 = property_animation_create(&animation_impl, context, NULL, NULL);
        property_animation_set_to_int16(a1, &to);

        time_t now = time(NULL);
        struct tm *tick_time = localtime(&now);
        static int16_t mon;
        memcpy(&mon, &tick_time->tm_mon, sizeof(int));
        mon += 1;
        mon *= 5;
        PropertyAnimation *a2 = property_animation_clone(a1);
        property_animation_set_to_int16(a2, &mon);
        animation_set_handlers(property_animation_get_animation(a2), (AnimationHandlers) {
            .stopped = animation_stopped_handler
        }, context);

        Animation *animation = animation_sequence_create(
            property_animation_get_animation(a1),
            property_animation_get_animation(a2),
            NULL);
        animation_schedule(animation);
        data->animated = true;
    }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    if (!data->animated) {
        static int16_t from;
        memcpy(&from, &data->value, sizeof(int8_t));
        static int16_t to;
        memcpy(&to, &tick_time->tm_hour, sizeof(int));
#ifdef DEMO
        to = 12;
#endif
        to = to > 12 ? to - 12 : to;
        to *= 5;

        PropertyAnimation *animation = property_animation_create(&animation_impl, context, NULL, NULL);
        property_animation_set_from_int16(animation, &from);
        property_animation_set_to_int16(animation, &to);
        animation_schedule(property_animation_get_animation(animation));
    }
}

HourLayer *hour_layer_create(GRect frame) {
    log_func();
    HourLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = fonts_get(RESOURCE_ID_LECO_FFONT);

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    int hour = tick_time->tm_hour;
#ifdef DEMO
    hour = 12;
#endif
    hour = hour > 12 ? hour - 12 : hour;
    data->value = hour * 5;
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(HOUR_UNIT, tick_handler, this);

    data->tap_event_handle = events_accel_tap_service_subscribe_context(accel_tap_handler, this);

    return this;
}

void hour_layer_destroy(HourLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_accel_tap_service_unsubscribe(data->tap_event_handle);
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    layer_destroy(this);
}
