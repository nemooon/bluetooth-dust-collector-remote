#ifndef HIKOKI_RAYTAC_CX40_VARIANT_H
#define HIKOKI_RAYTAC_CX40_VARIANT_H

#define VARIANT_MCK 64000000ul
#define USE_LFXO

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PINS_COUNT 48
#define NUM_DIGITAL_PINS 48
#define NUM_ANALOG_INPUTS 8
#define NUM_ANALOG_OUTPUTS 0

// Raw Arduino pin numbers map directly to nRF P0.00..P0.31, P1.00..P1.15.
#define PIN_LED PINS_COUNT
#define LED_BUILTIN PIN_LED
#define LED_BLUE PINS_COUNT
#define LED_STATE_ON 1
#define PIN_BUTTON1 38
#define PIN_SERIAL1_RX PINS_COUNT
#define PIN_SERIAL1_TX PINS_COUNT
#define SPI_INTERFACES_COUNT 0
#define WIRE_INTERFACES_COUNT 0
#define PIN_NEOPIXEL PINS_COUNT
#define NEOPIXEL_NUM 0
#define EXTERNAL_FLASH_DEVICES

#ifdef __cplusplus
}
#endif

#endif
