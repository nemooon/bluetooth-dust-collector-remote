#pragma once

#include <Arduino.h>

namespace hardware {
constexpr uint8_t kButtonPin = D1;    // NO button between D1 and GND
constexpr uint8_t kLedPin = D2;       // D2 -> 1 kOhm -> blue LED -> GND
constexpr bool kHasBattery = true;
constexpr bool kButtonInterrupt = true;
constexpr bool kStartupBlink = false;

inline void begin(void (*onButtonEdge)())
{
    pinMode(kLedPin, OUTPUT);
    digitalWrite(kLedPin, LOW);
    pinMode(kButtonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(kButtonPin), onButtonEdge, CHANGE);

    // The on-board divider is enabled with P0.14 LOW. P0.13 input selects
    // the lower charge-current setting on the XIAO.
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    pinMode(PIN_CHARGING_CURRENT, INPUT);
    analogReference(AR_INTERNAL);
    analogReadResolution(12);
    analogSampleTime(40);
}

inline bool buttonPressed() { return digitalRead(kButtonPin) == LOW; }
inline void setLed(bool on) { digitalWrite(kLedPin, on ? HIGH : LOW); }

inline int32_t batteryMillivolts()
{
    analogRead(PIN_VBAT);
    uint32_t sum = 0;
    for (unsigned i = 0; i < 4; ++i) sum += analogRead(PIN_VBAT);
    return (int32_t)(((uint64_t)sum * 3600 * 1510) / (4ULL * 4095 * 510));
}
} // namespace hardware
