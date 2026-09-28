#pragma once

#include <Arduino.h>
#include <nrf_gpio.h>

// Development dongle MDBT50Q-CX-40 has a DFU button on P1.06 and D1 LED on
// P0.06 (active LOW). D2 on P0.08 is not populated on the standard dongle.
// No battery circuit is assumed. Polling keeps this profile
// independent of an Arduino variant's logical pin numbering.
namespace hardware {
constexpr uint32_t kButtonPin = NRF_GPIO_PIN_MAP(1, 6);
constexpr uint32_t kLedPin = NRF_GPIO_PIN_MAP(0, 6);
constexpr bool kHasBattery = false;
constexpr bool kButtonInterrupt = false;
constexpr bool kStartupBlink = true;

inline void begin(void (*)())
{
    nrf_gpio_cfg_output(kLedPin);
    nrf_gpio_pin_set(kLedPin);
    nrf_gpio_cfg_input(kButtonPin, NRF_GPIO_PIN_PULLUP);
}

inline bool buttonPressed() { return nrf_gpio_pin_read(kButtonPin) == 0; }
inline void setLed(bool on)
{
    if (on) nrf_gpio_pin_clear(kLedPin);
    else nrf_gpio_pin_set(kLedPin);
}
inline int32_t batteryMillivolts() { return -1; }
} // namespace hardware
