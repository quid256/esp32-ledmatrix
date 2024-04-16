// import order is important!

#include <Arduino.h>
#include <WiFi.h>
// #include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"

const char* hostname = "gumby";

// Instantiate an async webserver object
AsyncWebServer server(80);

// HTML of the webpage
const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
<head>
  <title>Webserver Example</title>
</head>
<body>
  <p> Hello, world! %MESSAGE% </p>

  <p> Current state: %STATE% </p>

  <a href="?state=T"><button>set state to true</button></a>
  <a href="?state=F"><button>set state to false</button></a>
</body>
</html>
)rawliteral";

String localIP;
boolean state = false;

void setup() {
    Serial.begin(115200);
    while (!Serial)
        ;

    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    WiFi.begin(WIFI_SSID, WIFI_PWD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("connecting to wifi...");
    }
    Serial.println(WiFi.localIP());

    // Set up mDNS (allows for nice *.local address on LAN)
    if (!MDNS.begin(hostname)) {
        Serial.println("!! ERROR setting mDNS responder");
    } else {
        Serial.printf("mDNS responder set: %s\n", hostname);
    }

    // Set up web server endpoints
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        int n_params = request->params();
        for (int i = 0; i < n_params; i++) {
            AsyncWebParameter* p = request->getParam(i);

            if (p->name() == "state") {
                state = (p->value() == "T");
                Serial.printf("Set state: %s\n", state ? "TRUE" : "FALSE");
            }
        }
        // Respond with the contents of the index_html string
        request->send_P(200, "text/html", index_html, [](const String& var) {
            // Called for each of the %TEMPLATE% strings in the above --
            // can be used to specialize what the webpage shows to different
            // situations
            if (var == "MESSAGE") {
                return localIP;
            } else if (var == "STATE") {
                return String(state ? "TRUE" : "FALSE");
            }

            return String();
        });
    });
    server.begin();
}

void loop() {}
