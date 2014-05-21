#include <pebble.h>

#include "barcode_layer.h"

#define CLUBCARD_NUMBER_LENGTH 17 // including '\0' character
#define CLUBCARD_NUMBER_KEY 1

#define ACCOUNT_NUMBER_KEY 0

static Window *window;
static BarcodeLayer *barcode_layer;

static AppSync sync_ctx;
static uint8_t sync_buffer[32];

char clubcard_number[CLUBCARD_NUMBER_LENGTH];

static void set_clubcard_number(const char* number)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting clubcard account number to '%s'", number);
    strncpy(clubcard_number, number, CLUBCARD_NUMBER_LENGTH);
    persist_write_string(CLUBCARD_NUMBER_KEY, clubcard_number);
    barcode_layer_set_value(barcode_layer, clubcard_number);
}

static void sync_error_callback(DictionaryResult dict_error,
        AppMessageResult app_message_error, void *context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple,
        const Tuple* old_tuple, void* context)
{
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple changed for key %lu", key);
    switch(key) {
        case ACCOUNT_NUMBER_KEY:
            set_clubcard_number(new_tuple->value->cstring);
            break;
    }
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    barcode_layer = barcode_layer_create((GRect) {
        .origin = { 0, 0 }, .size = { bounds.size.w, bounds.size.h }
    });

    layer_add_child(window_layer, barcode_layer_get_layer(barcode_layer));

    // attempt to read code from storage initialising to "" otherwise
    if(0 >= persist_read_string(CLUBCARD_NUMBER_KEY, clubcard_number, CLUBCARD_NUMBER_LENGTH)) {
        strncpy(clubcard_number, "", CLUBCARD_NUMBER_LENGTH);
    }

    // Can't use TupleCString here because of http://forums.getpebble.com/discussion/10690/
    Tuplet initial_values[] = {
        ((const Tuplet) {
            .type = TUPLE_CSTRING,
            .key = ACCOUNT_NUMBER_KEY,
            .cstring = { .data = clubcard_number, .length = strlen(clubcard_number) + 1 }}),
    };

    app_sync_init(&sync_ctx, sync_buffer, sizeof(sync_buffer),
        initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_callback, sync_error_callback, NULL);
}

static void window_unload(Window *window) {
    app_sync_deinit(&sync_ctx);

    barcode_layer_destroy(barcode_layer);
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });

    const int inbound_size = 64;
    const int outbound_size = 16;
    app_message_open(inbound_size, outbound_size);

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
