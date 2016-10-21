#pragma once
#include <pebble.h>

typedef Layer MinuteLayer;

MinuteLayer *minute_layer_create(GRect frame);
void minute_layer_destroy(MinuteLayer *this);
