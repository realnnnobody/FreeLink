#include "LoRaHandler.h"

#define LORA_SPI_SPEED 2000000

LoRaHandler* LoRaHandler::instance = nullptr;

LoRaHandler::LoRaHandler(int cs, int dio0, int rst, int sck, int miso, int mosi)
    : pinNss(cs)
    , pinDio0(dio0)
    , pinRst(rst)
    , pinSck(sck)
    , pinMiso(miso)
    , pinMosi(mosi)
    , spiInst(), spiSettings(LORA_SPI_SPEED, MSBFIRST, SPI_MODE0)
    , radio(new Module(cs, dio0, rst, RADIOLIB_NC, spiInst, spiSettings))
{
    instance = this;
}

bool LoRaHandler::begin() {
    spiInst.begin(pinSck, pinMiso, pinMosi, pinNss);

    int state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("radio.begin failed, code: "); Serial.println(state);
        return false;
    }

    if (radio.setFrequency(433.5) == RADIOLIB_ERR_INVALID_FREQUENCY) {
        Serial.println(F("Selected frequency is invalid for this module!"));
        while (true) { delay(10); }
    }

    // від 6 до 12 чим більше тим далі летить але повільно(модуляція) 10
    if (radio.setSpreadingFactor(12) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        Serial.println("Selected spreading factor is invalid for this module!");
        while (true) { delay(10); }
    }

    // корекція помилки 5 - 8
    if (radio.setCodingRate(6) == RADIOLIB_ERR_INVALID_CODING_RATE) {
        Serial.println(F("Selected coding rate is invalid for this module!"));
        while (true) { delay(10); }
    }

    //треба
    if (radio.setSyncWord(0x14) != RADIOLIB_ERR_NONE) {
        Serial.println(F("Unable to set sync word!"));
        while (true) { delay(10); }
    }

    //потужнысть менше 1мВт
    if (radio.setOutputPower(7) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        Serial.println(F("Selected output power is invalid for this module!"));
        while (true) { delay(10); }
    }

    //обмеження по току
    if (radio.setCurrentLimit(80) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
        Serial.println(F("Selected current limit is invalid for this module!"));
        while (true) { delay(10); }
    }

    //данні на прокидання
    if (radio.setPreambleLength(15) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
        Serial.println(F("Selected preamble length is invalid for this module!"));
        while (true) { delay(10); }
    }

    //підсилання вхідного сигналу
    if (radio.setGain(1) == RADIOLIB_ERR_INVALID_GAIN) {
        Serial.println(F("Selected gain is invalid for this module!"));
        while (true) { delay(10); }
    }


    radio.setPacketReceivedAction(isrWrapper);

    radio.startReceive();
    Serial.println("LoRa: listening (interrupt mode)");
    return true;
}

void LoRaHandler::isrWrapper() {
    if (instance) {
        instance->packetFlag = true;
    }
}

void LoRaHandler::handlePacket() {
    String data;
    int err = radio.readData(data);
    if (err == RADIOLIB_ERR_NONE) {
        rxBuffer = data;
        if (rxCb) rxCb(rxBuffer);
    } else {
        Serial.print("readData error: "); Serial.println(err);
    }
    radio.startReceive();
}

void LoRaHandler::loop() {
    if (packetFlag) {
        packetFlag = false;
        handlePacket();
    }
}

bool LoRaHandler::send(const String &payload) {
    if (payload.length() == 0) return false;
    if (payload.length() > 240) {
        Serial.println("payload too long");
        return false;
    }

    int err = radio.transmit((uint8_t*)payload.c_str(), payload.length());
    if (err != RADIOLIB_ERR_NONE) {
        Serial.print("transmit err: "); Serial.println(err);
        return false;
    }
    radio.startReceive();
    return true;
}
