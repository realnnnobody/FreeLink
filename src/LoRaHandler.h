#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include <functional>

class LoRaHandler {
public:
    using RxCallback =  std::function<void(String&)>;
    LoRaHandler(int cs, int dio0, int rst, int sck, int miso, int mosi);
    bool begin();
    void loop();
    bool send(const String &payload);
    void onReceive(RxCallback cb) { rxCb = cb; }

private:
    int pinNss, pinDio0, pinRst;
    int pinSck, pinMiso, pinMosi;

    SPIClass spiInst;
    SPISettings spiSettings;
    SX1278 radio;

    volatile bool packetFlag = false;
    String rxBuffer;
    RxCallback rxCb = nullptr;

    static LoRaHandler* instance;
    static IRAM_ATTR void isrWrapper();
    void handlePacket();
};
