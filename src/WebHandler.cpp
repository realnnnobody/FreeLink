#include "WebHandler.h"
#include "LoRaHandler.h"
#include <WiFi.h>
#include <LittleFS.h>

WebHandler* WebHandler::instance = nullptr;
char WebHandler::indexHtml[] PROGMEM = R"rawliteral(
html,body{height:100%;margin:0;font-family:system-ui,Segoe UI,Roboto,Helvetica,Arial}
body{background:linear-gradient(180deg,#071024 0%, #071a2a 100%);display:flex;align-items:center;justify-content:center;padding:24px}
.app{width:100%;max-width:720px;background:var(--card);border-radius:12px;box-shadow:0 10px 30px rgba(2,6,23,.6);overflow:hidden;color:white}
header{padding:16px 20px;border-bottom:1px solid rgba(255,255,255,0.03);font-weight:600}
.messages{height:56vh;overflow:auto;padding:18px;display:flex;flex-direction:column;gap:10px}
.msg{max-width:75%;padding:10px 14px;border-radius:12px;line-height:1.2}
.me{align-self:flex-end;background:linear-gradient(90deg,var(--me),#0891b2);border-bottom-right-radius:4px}
.other{align-self:flex-start;background:rgba(255,255,255,0.03);border-bottom-left-radius:4px;color:var(--muted)}
.meta{font-size:12px;opacity:.8;margin-top:6px;text-align:right}
footer{display:flex;border-top:1px solid rgba(255,255,255,0.03);padding:12px}
input[type=text]{flex:1;padding:10px;border-radius:10px;border:0;background:rgba(255,255,255,0.02);color:#dbeafe;outline:none}
button{margin-left:8px;padding:10px 14px;border-radius:10px;border:0;background:var(--me);color:#022;cursor:pointer}
.hint{padding:8px 16px;color:var(--muted);font-size:13px}
@media(max-width:420px){.messages{height:60vh}}
</style>
</head>
<body>
<div class="app">
<header>LoRa Chat • Device: <span id="devip">...</span></header>
<div id="msgs" class="messages"></div>
<div class="hint">Your messages appear on the right; remote on the left.</div>
<footer>
<input id="input" type="text" placeholder="Type message and press Send" autocomplete="off" />
<button id="send">Send</button>
</footer>
</div>
<script>
(function(){
const msgs = document.getElementById('msgs');
const input = document.getElementById('input');
const btn = document.getElementById('send');
const devip = document.getElementById('devip');
devip.textContent = location.hostname;


const ws = new WebSocket('ws://' + location.hostname + ':81/');
ws.onopen = ()=> console.log('ws open');
ws.onclose = ()=> console.log('ws close');
ws.onerror = e => console.warn('ws err', e);


function addMessage(text, who){
const el = document.createElement('div');
el.className = 'msg ' + (who==='me' ? 'me' : 'other');
el.textContent = text;
msgs.appendChild(el);
msgs.scrollTop = msgs.scrollHeight;
}


ws.onmessage = (ev)=>{
// messages coming from device are treated as remote
addMessage(ev.data, 'other');
}


function send(){
const v = input.value.trim();
if(!v) return;
addMessage(v, 'me');
ws.send(v);
input.value='';
input.focus();
}


btn.addEventListener('click', send);
input.addEventListener('keydown', e=>{ if(e.key==='Enter') send(); });
})();
</script>
</body>
</html>
)rawliteral";

WebHandler::WebHandler(const char* apName, const char* apPass)
    : ssid(apName)
    , password(apPass)
    , server(80)
    , ws(81)
{
    instance = this;
}

void WebHandler::beggin() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS failed");
    } else {
        Serial.println("LittleFS mounted");
    }

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    IPAddress ip = WiFi.softAPIP();
    //Serial.print("AP started: "); Serial.println(WiFi.softAPSSID);
    //Serial.print("Open: http://"); Serial.println(WiFi.ip);

    server.on("/", HTTP_GET, [this]() {
        if (LittleFS.exists("/index.html")) {
            File f = LittleFS.open("/index.html", "r");
            server.streamFile(f, "text/html");
            f.close();
        } else {
            server.send_P(200, "text/html", indexHtml);
        }
    });

    server.on("/style.css", HTTP_GET, [this]() {
        if (LittleFS.exists("/style.css")) {
            File f = LittleFS.open("/style.css", "r");
            server.streamFile(f, "text/css");
            f.close();
        } else {
            server.send_P(404, "text/plain", "not found");
        }
    });

    server.on("/app.js", HTTP_GET, [this]() {
        if (LittleFS.exists("/app.js")) {
            File f = LittleFS.open("/app.js", "r");
            server.streamFile(f, "application/javascript");
            f.close();
        } else {
            server.send_P(404, "text/plain", "not found");
        }
    });

    server.begin();
    ws.begin();
    ws.onEvent([](uint8_t num, WStype_t type, uint8_t *payload, size_t length){
        if (WebHandler::instance) WebHandler::instance->handleWebSocketMessage(num, type, payload, length);
    });

    Serial.println("Web server started (HTTP 80, WS 81)");
}

void WebHandler::handleRoot() {
    server.send_P(200, "text/html", indexHtml);
}


void WebHandler::loop() {
    server.handleClient();
    ws.loop();
}

void WebHandler::handleWebSocketMessage(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        Serial.print("WS recv: "); Serial.println(msg);
        if (webMsgCb) webMsgCb(msg);
    } else if (type == WStype_DISCONNECTED) {
        Serial.printf("Client %u disconnected\n", num);
    } else if (type == WStype_CONNECTED) {
        Serial.printf("Client %u connected\n", num);
    }
}

void WebHandler::broadcastIncoming(String &msg) {
    ws.broadcastTXT(msg);
}