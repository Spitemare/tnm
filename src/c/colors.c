#include <pebble.h>
#include "logging.h"
#include "enamel.h"
#include "colors.h"

GColor get_background_color(void) {
    log_func();
#ifdef PBL_COLOR
    return enamel_get_COLOR_BACKGROUND();
#else
    return enamel_get_COLOR_INVERT() ? GColorWhite : GColorBlack;
#endif
}

GColor get_foreground_color(void) {
    log_func();
#ifdef PBL_COLOR
    return gcolor_legible_over(get_background_color());
#else
    return enamel_get_COLOR_INVERT() ? GColorBlack : GColorWhite;
#endif
}
