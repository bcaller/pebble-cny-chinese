#pragma once
#include <pebble.h>

typedef struct OutlinedTextLayer OutlinedTextLayer;

OutlinedTextLayer *outlined_text_layer_create(GRect frame);
void outlined_text_layer_destroy(OutlinedTextLayer *layer);
Layer* outlined_text_layer_get_layer(OutlinedTextLayer* layer);

void outlined_text_layer_set_text(OutlinedTextLayer* layer, char* text);
void outlined_text_layer_set_font(OutlinedTextLayer* layer, GFont font);

void outlined_text_layer_set_colors(OutlinedTextLayer * layer, GColor background, GColor foreground);

void outlined_text_layer_set_text_alignment(OutlinedTextLayer * text_layer, GTextAlignment text_alignment);
void outlined_text_layer_set_text_overflow_mode(OutlinedTextLayer * text_layer, GTextOverflowMode overflow);

void outlined_text_layer_set_offset_x(OutlinedTextLayer * layer, uint8_t offset_x);