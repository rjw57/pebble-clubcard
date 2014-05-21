/* A CODE-128C barcode which can display an even number of numerical digits. */

#ifndef BARCODE_H__
#define BARCODE_H__

#include <pebble.h>

/* An opaque type representing a BarcodeLayer */
typedef struct BarcodeLayer_ BarcodeLayer;

BarcodeLayer* barcode_layer_create(GRect frame);

void barcode_layer_destroy(BarcodeLayer* barcode_layer);

/* Gets the "root" Layer of the bitmap layer, which is the parent for the sub-
 * layers used for its implementation. */
Layer* barcode_layer_get_layer(const BarcodeLayer* barcode_layer);

/* Set the barcode value to the string passed in. The string must have even length
 * and consist only of the digits 0-9. If the string does not correspond to these
 * requirements, the barcode is cleared.
 */
void barcode_layer_set_value(BarcodeLayer* barcode_layer, const char* value);

#endif /* BARCODE_H__ */
