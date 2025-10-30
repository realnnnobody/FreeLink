#include <Arduino.h>
#include "LoRaHandler.h"
#include "WebHandler.h"

#if defined(BOARD_ESP32)
  #define LORA_SCK  14
  #define LORA_MISO 12
  #define LORA_MOSI 13
  #define LORA_NSS  15
  #define LORA_DIO0 33
  #define LORA_RST  17
#elif defined(BOARD_XIAO_ESP32S3)
  #define LORA_SCK  7
  #define LORA_MISO 8
  #define LORA_MOSI 9
  #define LORA_NSS  1
  #define LORA_DIO0 2
  #define LORA_RST  4
#endif

const char* AP_SSID = "ESP32-LoRa-Chat(ESP)";
const char* AP_PASS = "12345678";

LoRaHandler lora(LORA_NSS, LORA_DIO0, LORA_RST, LORA_SCK, LORA_MISO, LORA_MOSI);
WebHandler web(AP_SSID, AP_PASS);

void onLoRaIncoming(String &msg) {
  web.broadcastIncoming(msg);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== ESP32 LoRa Chat - starting ===");


  if (!lora.begin()) {
    Serial.println("LoRa init failed - halt");
    while (true) { delay(1000); }
  }
  lora.onReceive(onLoRaIncoming);

  web.beggin();

  web.onWebMessage([&](String &txt){
    Serial.print("Web -> LoRa: "); Serial.println(txt);
    bool ok = lora.send(txt);
    if (!ok) Serial.println("LoRa TX failed");
  });

  Serial.println("Setup complete. AP running. Connect to the web UI to chat.");
}


void loop() {
  lora.loop();
  web.loop();
  delay(5);
}




/*#include <RadioLib.h>
#include <SPI.h>

#define My_transiver


#if defined(BOARD_ESP32)
  #define LORA_SCK  14
  #define LORA_MISO 12
  #define LORA_MOSI 13
  #define LORA_NSS  15
  #define LORA_DIO0 33
  #define LORA_RST  17
#elif defined(BOARD_XIAO_ESP32S3)
  #define LORA_SCK  7
  #define LORA_MISO 8
  #define LORA_MOSI 9
  #define LORA_NSS  1
  #define LORA_DIO0 2
  #define LORA_RST  4
#endif


SPIClass LoRaSPI;
SPISettings LoRaSPISettings(2000000, MSBFIRST, SPI_MODE0);
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, RADIOLIB_NC, LoRaSPI, LoRaSPISettings);

#if defined(My_transiver)
  int transmissionState = RADIOLIB_ERR_NONE;
  volatile bool transmittedFlag = false;
  bool busy = false;
  ICACHE_RAM_ATTR void setFlag(void) {
    transmittedFlag = true;
  }
#else
  volatile bool receivedFlag = false;
  ICACHE_RAM_ATTR void setFlag(void) {
    receivedFlag = true;
  }
#endif

void setup() {
  Serial.begin(9600);
  delay(200);
  LoRaSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);

  Serial.print(F("[SX1278] Initializing ... "));
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true) { delay(10); }
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

  #if defined(My_transiver)
    radio.setPacketReceivedAction(setFlag);

    Serial.println("Type text");
  #else
    radio.setPacketReceivedAction(setFlag);
    Serial.println(F("All settings successfully changed!"));

    Serial.print(F("[SX1278] Starting to listen ... "));
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println(F("success!"));
    } else {
      Serial.print("failed, code ");
      Serial.println(state);
      while (true) { delay(10); }
    }
  #endif
}

void loop() {
  #if defined(My_transiver)
    if (transmittedFlag) {
      transmittedFlag = false;
      busy = false;

      if (transmissionState == RADIOLIB_ERR_NONE) {
        Serial.println("[SX1278] transmission finished!");
      } else {
        Serial.print("[SX1278] transmission failed, code ");
        Serial.println(transmissionState);
      }
      radio.finishTransmit();
    }

    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();

      if (line.length() == 0) { return; }
  
      if (line.length() > 255) {
        Serial.print("[SX1278] Input too long (");
        Serial.print(line.length());
        Serial.print(" chars). Truncating to 255 chars.");
        line = line.substring(0, 255);
      }

      if (busy) {
        Serial.println(F("[SX1278] Currently transmitting. Please wait until current transmission finishes."));
      } else {
        Serial.print(F("[SX1278] Sending: "));
        Serial.println(line);
        transmissionState = radio.startTransmit(line);
        if (transmissionState == RADIOLIB_ERR_NONE) {
          busy = true;
        } else {
          Serial.print(F("[SX1278] startTransmit failed, code "));
          Serial.println(transmissionState);
          busy = false;
        }
    }
  }
  #else
    if (receivedFlag) {
      receivedFlag = false;

      String str;
      int state = radio.readData(str);

      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[SX1278] Received packet!");

        Serial.print("[SX1278] Data:\t\t");
        Serial.println(str);

        Serial.print("[SX1278] RSSI\t\t");
        Serial.print(radio.getRSSI());
        Serial.println(" dBm");

        Serial.print(F("[SX1278] SNR:\t\t"));
        Serial.print(radio.getSNR());
        Serial.println(F(" dB"));

        Serial.print(F("[SX1278] Frequency error:\t"));
        Serial.print(radio.getFrequencyError());
        Serial.println(F(" Hz"));
      } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
        Serial.println("[SX1278] CRC error!");
      } else {
        Serial.print("[SX1278] Failed, code ");
        Serial.println(state);
      }
    }
  #endif
}*/






