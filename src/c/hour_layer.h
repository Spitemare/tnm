#pragma once
#include <pebble.h>

typedef Layer HourLayer;

HourLayer *hour_layer_create(GRect frame);
void hour_layer_destroy(HourLayer *this);
