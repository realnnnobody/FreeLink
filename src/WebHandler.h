#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

class LoRaHandler;

class WebHandler {
public:
    using WebMsgCb = std::function<void(String&)>;
    WebHandler(const char* apName, const char* apPass);
    void beggin();
    void loop();

    void broadcastIncoming(String &msg);
    void onWebMessage(WebMsgCb cb) { webMsgCb = cb; }

private:
    const char* ssid;
    const char* password;
    WebServer server;
    WebSocketsServer ws;

    WebMsgCb webMsgCb = nullptr;
    static char indexHtml[] PROGMEM;

    void handleRoot();
    static void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
    void handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    static WebHandler* instance;
};
