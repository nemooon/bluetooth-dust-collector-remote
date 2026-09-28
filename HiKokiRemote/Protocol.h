#pragma once
#include <Arduino.h>

// BLE profile observed with the proven dongle. Callbacks only wake the UI task.
bool protocolBegin(void (*wake)());
bool protocolStartAdvertising();
void protocolStopAdvertising();
bool protocolIsAdvertising();
bool protocolIsConnected();
void protocolSetRunning(bool running);
void protocolNotify();
void protocolDisconnect();
uint16_t protocolStateHandle();
