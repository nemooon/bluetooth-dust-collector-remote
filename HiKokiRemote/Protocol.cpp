#include <bluefruit.h>
#include "Protocol.h"

namespace {
constexpr char kName[] = "HiKOKI BSL36A18BX";
constexpr uint8_t kManufacturer[] = {0x95, 0x06, 0x00, 0x07, 0x00, 0x00, 0x02};
constexpr uint8_t kProperties = CHR_PROPS_READ | CHR_PROPS_WRITE_WO_RESP |
                                CHR_PROPS_WRITE | CHR_PROPS_NOTIFY | CHR_PROPS_INDICATE;
BLEService service("ffff1948-ffff-ffff-efcd-ab8967452301");
BLECharacteristic char1("ffff1029-ffff-ffff-efcd-ab8967452301");
BLECharacteristic char2("ffff30ef-ffff-ffff-efcd-ab8967452301");
BLECharacteristic char3("ffffb178-ffff-ffff-efcd-ab8967452301");
BLECharacteristic stateChar("ffff11f1-ffff-ffff-efcd-ab8967452301");
void (*wakeUi)();
uint8_t currentState = 0x02;

void onConnect(uint16_t) { if (wakeUi) wakeUi(); }
void onDisconnect(uint16_t, uint8_t) { if (wakeUi) wakeUi(); }

void onStateWrite(uint16_t, BLECharacteristic*, uint8_t*, uint16_t)
{
    // Restore the physical-button state if the central writes this value.
    stateChar.write8(currentState);
}

bool beginByteCharacteristic(BLECharacteristic& characteristic, uint8_t initial)
{
    characteristic.setProperties(kProperties);
    characteristic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    characteristic.setFixedLen(1);
    if (characteristic.begin() != NRF_SUCCESS) return false;
    return characteristic.write8(initial) == 1;
}
} // namespace

bool protocolBegin(void (*wake)())
{
    wakeUi = wake;
    Bluefruit.configUuid128Count(5);
    Bluefruit.autoConnLed(false);
    if (!Bluefruit.begin(1, 0)) return false;
    Bluefruit.setName(kName);

    // Keep the SoftDevice's factory-generated random static address. It is
    // unique to this nRF52840 and remains stable across resets.

    Bluefruit.Periph.setConnectCallback(onConnect);
    Bluefruit.Periph.setDisconnectCallback(onDisconnect);
    Bluefruit.Advertising.restartOnDisconnect(false);
    Bluefruit.Advertising.setInterval(0x0120, 0x0120); // 180 ms
    Bluefruit.Advertising.setFastTimeout(0);          // UI owns the timeout
    if (service.begin() != NRF_SUCCESS) return false;
    if (!beginByteCharacteristic(char1, 0x00) ||
        !beginByteCharacteristic(char2, 0x00) ||
        !beginByteCharacteristic(char3, 0x00) ||
        !beginByteCharacteristic(stateChar, currentState)) return false;
    stateChar.setWriteCallback(onStateWrite);

    // Flags (3) + complete name (19) + manufacturer data (9) = 31 bytes.
    Bluefruit.Advertising.clearData();
    if (!Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE) ||
        !Bluefruit.Advertising.addName() ||
        !Bluefruit.Advertising.addManufacturerData(kManufacturer, sizeof(kManufacturer))) {
        return false;
    }
    return Bluefruit.Advertising.count() == 31;
}

bool protocolStartAdvertising()
{
    return Bluefruit.Advertising.start(0);
}

void protocolStopAdvertising()
{
    if (Bluefruit.Advertising.isRunning()) Bluefruit.Advertising.stop();
}

bool protocolIsAdvertising() { return Bluefruit.Advertising.isRunning(); }
bool protocolIsConnected() { return Bluefruit.Periph.connected() != 0; }

void protocolSetRunning(bool running)
{
    currentState = running ? 0x01 : 0x02;
    stateChar.write8(currentState);
}

void protocolNotify()
{
    if (protocolIsConnected()) stateChar.notify8(currentState);
}

void protocolDisconnect()
{
    if (protocolIsConnected()) Bluefruit.disconnect(Bluefruit.connHandle());
}

uint16_t protocolStateHandle() { return stateChar.handles().value_handle; }
