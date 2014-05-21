#include "barcode_layer.h"

#include <pebble.h>

// can't have more bars/spaces than pixels on the screen!
#define MAX_BARS 168

struct BarcodeLayer_ {
    Layer                   *base_layer;
    int16_t                  n_bars;
    uint8_t                  bar_data[MAX_BARS>>3];
};

// Internal functions
void barcode_layer_update_proc(Layer *layer, GContext* ctx);
void barcode_layer_set_bar_state(BarcodeLayer* barcode, int16_t idx, int16_t state);
int16_t barcode_layer_get_bar_state(BarcodeLayer* barcode, int16_t idx);
void barcode_layer_append_value(BarcodeLayer* barcode, int16_t value);

// CODE128 bar/space weights
// FIXME: this could be a more compact representation(!)
static char* bar_weights[] = {
    "212222", "222122", "222221", "121223", "121322", "131222", "122213",
    "122312", "132212", "221213", "221312", "231212", "112232", "122132",
    "122231", "113222", "123122", "123221", "223211", "221132", "221231",
    "213212", "223112", "312131", "311222", "321122", "321221", "312212",
    "322112", "322211", "212123", "212321", "232121", "111323", "131123",
    "131321", "112313", "132113", "132311", "211313", "231113", "231311",
    "112133", "112331", "132131", "113123", "113321", "133121", "313121",
    "211331", "231131", "213113", "213311", "213131", "311123", "311321",
    "331121", "312113", "312311", "332111", "314111", "221411", "431111",
    "111224", "111422", "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211", "241211", "221114",
    "413111", "241112", "134111", "111242", "121142", "121241", "114212",
    "124112", "124211", "411212", "421112", "421211", "212141", "214121",
    "412121", "111143", "111341", "131141", "114113", "114311", "411113",
    "411311", "113141", "114131", "311141", "411131", "211412", "211214",
    "211232", "2331112", "211133",
};

void barcode_layer_append_value(BarcodeLayer* barcode, int16_t value)
{
    // ignore invalid value
    if((value < 0) || (value > 106)) {
        return;
    }

    int16_t state = -1;
    for(char* weights = bar_weights[value]; *weights != '\0'; ++weights)
    {
        int weight = (*weights) - '0';
        for(int i=0; i<weight; ++i) {
            ++barcode->n_bars;
            barcode_layer_set_bar_state(barcode, barcode->n_bars-1, state);
        }
        state = !state;
    }
}

void barcode_layer_update_proc(Layer *layer, GContext* ctx)
{
    // Get our bounds
    GRect bounds = layer_get_bounds(layer);

    // Get barcode struct
    BarcodeLayer* barcode = *((BarcodeLayer**) layer_get_data(layer));

    // Fill background with white
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);

    // Draw barcode bars
    graphics_context_set_stroke_color(ctx, GColorBlack);
    int16_t y = bounds.origin.y + ((bounds.size.h - barcode->n_bars)>>1);
    for(int16_t line_idx=0; (line_idx < barcode->n_bars) && (y < bounds.origin.y + bounds.size.h);
            ++line_idx, ++y)
    {
        if(barcode_layer_get_bar_state(barcode, line_idx)) {
            graphics_draw_line(ctx,
                (GPoint) { .x = bounds.origin.x, .y = y },
                (GPoint) { .x = bounds.origin.x + bounds.size.w, .y = y }
            );
        }
    }
}

void barcode_layer_set_bar_state(BarcodeLayer* barcode, int16_t idx, int16_t value)
{
    // Do nothing if the bar index is invalid
    if((idx < 0) || (idx >= barcode->n_bars)) {
        return;
    }

    int16_t byte_idx = (idx>>3), bit_idx = (idx & 0x7);

    // Read
    uint8_t state = barcode->bar_data[byte_idx];

    // Clear bar
    state &= ~(1<<bit_idx);

    // Set bar if necessary
    if(value) {
        state |= (1<<bit_idx);
    }

    // Update
    barcode->bar_data[byte_idx] = state;
}

int16_t barcode_layer_get_bar_state(BarcodeLayer* barcode, int16_t idx)
{
    // Do nothing if the bar index is invalid
    if((idx < 0) || (idx >= barcode->n_bars)) {
        return 0;
    }

    int16_t byte_idx = (idx>>3), bit_idx = (idx & 0x7);

    // Read
    uint8_t state = barcode->bar_data[byte_idx];

    return (state & (1<<bit_idx)) ? -1 : 0;
}

BarcodeLayer* barcode_layer_create(GRect frame)
{
    // Create BarcodeLayer structure.
    BarcodeLayer* barcode = (BarcodeLayer*) malloc(sizeof(BarcodeLayer));
    if(barcode == NULL)
        return NULL;

    // Initialise struct
    *barcode = (BarcodeLayer) {
        .base_layer         = layer_create_with_data(frame, sizeof(BarcodeLayer*)),
        .n_bars             = 0,
    };

    // Set the layer's data to the barcode struct
    *((BarcodeLayer**) layer_get_data(barcode->base_layer)) = barcode;

    // Set update proc
    layer_set_update_proc(barcode->base_layer, barcode_layer_update_proc);

    return barcode;
}

void barcode_layer_destroy(BarcodeLayer* barcode_layer)
{
    if(NULL == barcode_layer)
        return;

    // destroy the base layer
    layer_destroy(barcode_layer->base_layer);

    // free the state structure
    free(barcode_layer);
}

Layer* barcode_layer_get_layer(const BarcodeLayer* barcode_layer)
{
    return barcode_layer->base_layer;
}

void barcode_layer_set_value(BarcodeLayer* barcode, const char* value)
{
    int16_t checksum = 0;

    // reset barcode and start C
    barcode->n_bars = 0;
    barcode_layer_append_value(barcode, 105); // start code C
    checksum = 105;

    for(int16_t idx=1; *value != '\0'; value+=2, ++idx)
    {
        int16_t code_value = 0;

        // check that both of the values are not \0
        if((value[0] == '\0') || (value[1] == '\0')) {
            barcode->n_bars = 0;
            return;
        }

        // first digit
        if((value[0] >= '0') && (value[0] <= '9')) {
            code_value += 10 * (value[0] - '0');
        } else {
            barcode->n_bars = 0;
            return;
        }

        // second digit
        if((value[1] >= '0') && (value[1] <= '9')) {
            code_value += value[1] - '0';
        } else {
            barcode->n_bars = 0;
            return;
        }

        barcode_layer_append_value(barcode, code_value);
        checksum = (checksum + idx*code_value) % 103;
    }

    // checksum digit
    barcode_layer_append_value(barcode, checksum);

    // stop
    barcode_layer_append_value(barcode, 106);
}
