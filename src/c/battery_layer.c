#include <pebble.h>
#include <pebble-fctx/fctx.h>
#include <pebble-fctx/ffont.h>
#include <pebble-events/pebble-events.h>
#include "logging.h"
#include "fonts.h"
#include "colors.h"
#include "battery_layer.h"

static const uint32_t TAP_TIMEOUT = 3000; // 3 seconds

typedef struct {
    FFont *font;
    int8_t value;
    bool animated;
    EventHandle battery_state_event_handle;
} Data;

static void update_proc(Layer *this, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(this);
    Data *data = layer_get_data(this);
    int8_t bat = data->value;

    FContext fctx;
    fctx_init_context(&fctx, ctx);

    int16_t font_size = bounds.size.w / PBL_IF_ROUND_ELSE(16, 14);
    fctx_set_color_bias(&fctx, 0);
    fctx_set_fill_color(&fctx, get_foreground_color());

    FPoint offset = FPointI(PBL_IF_RECT_ELSE(-1.9 * font_size, 0), 0);
    GRect rect = grect_crop(bounds, font_size * PBL_IF_ROUND_ELSE(4.6, 3.8));
    for (int i = bat; i < 100 + bat; i++) {
        fctx_begin_fill(&fctx);

        int32_t rot_angle = (i - bat) * TRIG_MAX_ANGLE / 100;
        fctx_set_rotation(&fctx, rot_angle);

        int32_t point_angle = (i - 25 - bat) * TRIG_MAX_ANGLE / 100;
        GPoint p = gpoint_from_polar(rect, PBL_IF_RECT_ELSE(GOvalScaleModeFillCircle, GOvalScaleModeFitCircle), point_angle);
        fctx_set_offset(&fctx, fpoint_add(offset, g2fpoint(p)));

        if (i % 10 == 0) {
            char s[4];
            fctx_set_text_em_height(&fctx, data->font, font_size);
            snprintf(s, sizeof(s), "%d", i > 100 ? i - 100 : i);
            fctx_draw_string(&fctx, s, data->font, GTextAlignmentLeft, FTextAnchorMiddle);
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

static void battery_state_handler(BatteryChargeState state, void *context) {
    log_func();
    Data *data = layer_get_data(context);
    if (!data->animated) {
        static int16_t from;
        memcpy(&from, &data->value, sizeof(int8_t));
        static int16_t to;
        memcpy(&to, &state.charge_percent, sizeof(uint8_t));
        to = to < 10 ? 10 : to;

        PropertyAnimation *animation = property_animation_create(&animation_impl, context, NULL, NULL);
        property_animation_set_from_int16(animation, &from);
        property_animation_set_to_int16(animation, &to);
        animation_schedule(property_animation_get_animation(animation));
    }
}

BatteryLayer *battery_layer_create(GRect frame) {
    log_func();
    BatteryLayer *this = layer_create_with_data(frame, sizeof(Data));
    layer_set_update_proc(this, update_proc);
    Data *data = layer_get_data(this);

    data->font = fonts_get(RESOURCE_ID_LECO_FFONT);

    BatteryChargeState charge_state = battery_state_service_peek();
    data->value = charge_state.charge_percent;
    data->battery_state_event_handle = events_battery_state_service_subscribe_context(battery_state_handler, this);

    return this;
}

void battery_layer_destroy(BatteryLayer *this) {
    log_func();
    Data *data = layer_get_data(this);
    events_battery_state_service_unsubscribe(data->battery_state_event_handle);
    layer_destroy(this);
}

