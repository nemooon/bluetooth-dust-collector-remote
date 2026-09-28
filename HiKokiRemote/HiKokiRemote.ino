#include <Arduino.h>
#include <bluefruit.h>
#include <FreeRTOS.h>
#include <task.h>
#include "Config.h"
#include "HardwareProfile.h"
#include "Protocol.h"

enum class Mode { Standby, Advertising, Connected, Disconnecting };
static Mode mode = Mode::Standby;
static TaskHandle_t uiTask;
static bool rawPressed;
static bool stablePressed;
static bool longHandled;
static bool running;
static bool lowBattery;
static uint32_t rawChangedAt;
static uint32_t pressedAt;
static uint32_t advertisingAt;
static uint32_t nextNotifyAt;
static uint32_t disconnectAt;
static uint32_t nextBatteryAt;
static uint32_t lowFlashUntil;

static void wakeUi() { if (uiTask) xTaskNotifyGive(uiTask); }
static void onButtonEdge()
{
    BaseType_t woken = pdFALSE;
    if (uiTask) vTaskNotifyGiveFromISR(uiTask, &woken);
    portYIELD_FROM_ISR(woken);
}

static void setLed(bool on)
{
    hardware::setLed(on);
}

static void updateLed(uint32_t now)
{
    bool on = false;
    if (mode == Mode::Advertising) on = (now / 500) % 2 == 0;
    if (mode == Mode::Connected || mode == Mode::Disconnecting) on = true;
    if (mode == Mode::Standby && (int32_t)(lowFlashUntil - now) > 0)
        on = (now / 150) % 2 == 0;
    if (lowBattery && mode != Mode::Standby && now % 10000 < 250) on = false;
    setLed(on);
}

static void checkBattery(uint32_t now, bool showWarning)
{
    int32_t mv = hardware::batteryMillivolts();
    lowBattery = hardware::kHasBattery && kLowBatteryMv > 0 && mv <= kLowBatteryMv;
#if HIKOKI_DEBUG
    Serial.print("Battery mV: ");
    Serial.println(mv);
#endif
    if (lowBattery && showWarning) lowFlashUntil = now + 900;
    nextBatteryAt = now + kBatteryCheckMs;
}

static void standby()
{
    mode = Mode::Standby;
    running = false;
    protocolSetRunning(false);
}

static void startSearch(uint32_t now)
{
    if (protocolStartAdvertising()) {
        advertisingAt = now;
        mode = Mode::Advertising;
    } else {
        standby();
    }
}

static void stopSearch()
{
    protocolStopAdvertising();
    standby();
}

static void shortPress(uint32_t now)
{
    checkBattery(now, true);
    if (mode != Mode::Connected) return;
    running = !running;
    protocolSetRunning(running);
    protocolNotify();
    nextNotifyAt = now + kNotifyMs;
}

static void longPress(uint32_t now)
{
    checkBattery(now, true);
    switch (mode) {
    case Mode::Standby: startSearch(now); break;
    case Mode::Advertising: stopSearch(); break;
    case Mode::Connected:
        if (running) {
            running = false;
            protocolSetRunning(false);
            protocolNotify();
            mode = Mode::Disconnecting;
            disconnectAt = now + kOffBeforeDisconnectMs;
            nextNotifyAt = now + kNotifyMs;
        } else {
            mode = Mode::Disconnecting;
            protocolDisconnect();
            disconnectAt = now + 1000;
        }
        break;
    case Mode::Disconnecting: break;
    }
}

static void pollButton(uint32_t now)
{
    bool pressed = hardware::buttonPressed();
    if (pressed != rawPressed) {
        rawPressed = pressed;
        rawChangedAt = now;
    }
    if (rawPressed != stablePressed && now - rawChangedAt >= kDebounceMs) {
        stablePressed = rawPressed;
        if (stablePressed) {
            pressedAt = now;
            longHandled = false;
        } else if (!longHandled) {
            shortPress(now);
        }
    }
    if (stablePressed && !longHandled && now - pressedAt >= kLongPressMs) {
        longHandled = true;
        longPress(now);
    }
}

void setup()
{
    uiTask = xTaskGetCurrentTaskHandle();
#if HIKOKI_DEBUG
    Serial.begin(115200);
#endif
    hardware::begin(onButtonEdge);
    if (hardware::kStartupBlink) {
        for (unsigned i = 0; i < 2; ++i) {
            setLed(true);
            delay(120);
            setLed(false);
            delay(120);
        }
    }
    rawPressed = stablePressed = hardware::buttonPressed();
    uint32_t now = millis();
    rawChangedAt = pressedAt = now;
    checkBattery(now, false);
    if (!protocolBegin(wakeUi)) {
        // Bluetooth setup failed: remain visible instead of silently sleeping.
        while (true) { setLed(true); delay(100); setLed(false); delay(100); }
    }
    standby();
}

void loop()
{
    uint32_t now = millis();
    pollButton(now);
    bool connected = protocolIsConnected();
    if (mode == Mode::Advertising) {
        if (connected) {
            mode = Mode::Connected;
            running = false;
            protocolSetRunning(false);
            nextNotifyAt = now;
        } else if (!protocolIsAdvertising() || now - advertisingAt >= kAdvertisingMs) {
            stopSearch();
        }
    } else if ((mode == Mode::Connected || mode == Mode::Disconnecting) && !connected) {
        standby();
    } else if (mode == Mode::Standby && connected) {
        mode = Mode::Disconnecting; // Connection raced with advertising cancellation.
        protocolDisconnect();
        disconnectAt = now + 1000;
    }
    if ((mode == Mode::Connected || mode == Mode::Disconnecting) && connected &&
        (int32_t)(now - nextNotifyAt) >= 0) {
        protocolNotify();
        nextNotifyAt = now + kNotifyMs;
    }
    if (mode == Mode::Disconnecting && connected && (int32_t)(now - disconnectAt) >= 0) {
        protocolDisconnect();
        disconnectAt = now + 1000;
    }
    if (mode != Mode::Standby && (int32_t)(now - nextBatteryAt) >= 0)
        checkBattery(now, false);
    updateLed(now);

    bool active = mode != Mode::Standby || rawPressed || stablePressed ||
                  (int32_t)(lowFlashUntil - now) > 0;
    ulTaskNotifyTake(pdTRUE, (active || !hardware::kButtonInterrupt) ?
                     pdMS_TO_TICKS(10) : portMAX_DELAY);
}
