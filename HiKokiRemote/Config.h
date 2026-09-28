#pragma once
#include <Arduino.h>

// Select the wiring profile here. The development dongle board sets this to 2.
#define HIKOKI_BOARD_XIAO 1
#define HIKOKI_BOARD_RAYTAC_DEV_DONGLE 2
#ifndef HIKOKI_BOARD
#define HIKOKI_BOARD HIKOKI_BOARD_XIAO
#endif
#if HIKOKI_BOARD != HIKOKI_BOARD_XIAO && HIKOKI_BOARD != HIKOKI_BOARD_RAYTAC_DEV_DONGLE
#error "Unknown HIKOKI_BOARD"
#endif

constexpr uint32_t kDebounceMs = 25;
constexpr uint32_t kLongPressMs = 800;
constexpr uint32_t kAdvertisingMs = 30000;
constexpr uint32_t kNotifyMs = 100;
constexpr uint32_t kOffBeforeDisconnectMs = 350;
constexpr uint32_t kBatteryCheckMs = 60000;

// Set only after measuring the chosen cell on this board. Zero disables warning.
constexpr int32_t kLowBatteryMv = 0;
#ifndef HIKOKI_DEBUG
#define HIKOKI_DEBUG 0  // Change to 1 while measuring voltage over USB serial.
#endif
