#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"
#include "fonts.h"
#include "colors.h"
#include "minute_layer.h"

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
    int8_t min = data->value;

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    int16_t font_size = bounds.size.w / 10;
    fctx_set_color_bias(&fctx, 0);
    fctx_set_fill_color(&fctx, get_foreground_color());

    FPoint offset = FPointI(PBL_IF_RECT_ELSE(-1.5 * font_size, 0), 0);
    for (int i = min; i > min - 60; i--) {
        fctx_begin_fill(&fctx);

        int32_t rot_angle = (i - min) * TRIG_MAX_ANGLE / 60;
        fctx_set_rotation(&fctx, rot_angle);

        int32_t point_angle = (i + 15 - min) * TRIG_MAX_ANGLE / 60;
        GPoint p = gpoint_from_polar(bounds, PBL_IF_RECT_ELSE(GOvalScaleModeFillCircle, GOvalScaleModeFitCircle), point_angle);
        fctx_set_offset(&fctx, fpoint_add(offset, g2fpoint(p)));

        if (i % 5 == 0) {
            char s[3];
            fctx_set_text_em_height(&fctx, data->font, font_size);
            snprintf(s, sizeof(s), "%02d", i < 0 ? i + 60 : i);
            fctx_draw_string(&fctx, s, data->font, GTextAlignmentRight, FTextAnchorMiddle);
        } else {
            fctx_set_text_em_height(&fctx, data->font, font_size - 4);
            fctx_draw_string(&fctx, "-", data->font, GTextAlignmentRight, FTextAnchorMiddle);
        }
        fctx_end_fill(&fctx);
#ifdef PBL_COLOR
        fctx_set_color_bias(&fctx, -3);
#else
        fctx_set_fill_color(&fctx, GColorDarkGray);
#endif
    }

    fctx_deinit_context(&fctx);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed, void *this) {
    log_func();
    Data *data = layer_get_data(this);
    if (!data->animated) {
        data->value = tick_time->tm_min;
#ifdef DEMO
        data->value = 30;
#endif
        layer_mark_dirty(this);
    }
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
    memcpy(&to, &tick_time->tm_min, sizeof(int));
#ifdef DEMO
    to = 30;
#endif

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
        static int16_t mday;
        memcpy(&mday, &tick_time->tm_mday, sizeof(int16_t));
        PropertyAnimation *a2 = property_animation_clone(a1);
        property_animation_set_to_int16(a2, &mday);
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

MinuteLayer *minute_layer_create(GRect frame) {
    log_func();
    MinuteLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = fonts_get(RESOURCE_ID_LECO_FFONT);

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);
    tick_handler(tick_time, MINUTE_UNIT, this);
    data->tick_timer_event_handle = events_tick_timer_service_subscribe_context(MINUTE_UNIT, tick_handler, this);

    data->tap_event_handle = events_accel_tap_service_subscribe_context(accel_tap_handler, this);

    return this;
}

void minute_layer_destroy(MinuteLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_accel_tap_service_unsubscribe(data->tap_event_handle);
    events_tick_timer_service_unsubscribe(data->tick_timer_event_handle);
    layer_destroy(this);
}
