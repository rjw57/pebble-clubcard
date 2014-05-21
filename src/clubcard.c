#include <pebble.h>

#include "barcode_layer.h"

#define CLUBCARD_CODE_LENGTH 17 // including '\0' character
#define CLUBCARD_CODE_KEY 1

static Window *window;
static BarcodeLayer *barcode_layer;

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    barcode_layer = barcode_layer_create((GRect) {
        .origin = { 0, 0 }, .size = { bounds.size.w, bounds.size.h }
    });

    char clubcard_code[CLUBCARD_CODE_LENGTH];
    if(persist_read_string(CLUBCARD_CODE_KEY, clubcard_code, CLUBCARD_CODE_LENGTH) > 0) {
        barcode_layer_set_value(barcode_layer, clubcard_code);
    }

    layer_add_child(window_layer, barcode_layer_get_layer(barcode_layer));
}

static void window_unload(Window *window) {
    barcode_layer_destroy(barcode_layer);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
    window_stack_push(window, /* animated = */ true);
}

static void deinit(void) {
    window_destroy(window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}