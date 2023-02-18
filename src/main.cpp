#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <AsyncJson.h>
#include <WebSocketsServer.h>
#include "LittleFS.h"
#include "secrets.h"

const char* ssid = SSID;
const char* password = PASSWORD;

int optoPin = 13;  // D7
int statePin = 15; // D8
int PCState = false;

unsigned long lastTime = 0;

bool toggle_state = false;
String event = "OFF";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void handleToggle(AsyncWebServerRequest *request) {
  request->redirect("/");
  toggle_state = true;
}

void handleRoot(AsyncWebServerRequest *request) {
  if (!LittleFS.begin()) {
    request->send(500, "text/plain", "LittleFS initialization failed");
    return;
  }

  File indexFile = LittleFS.open("/index.html", "r");
  File cssFile = LittleFS.open("/style.css", "r"); // Neue Zeile

  if (!indexFile) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  String html = indexFile.readString();
  indexFile.close();

  String css = cssFile.readString(); // Neue Zeile
  cssFile.close(); // Neue Zeile

  html.replace("</head>", "<style>" + css + "</style></head>"); // Neue Zeile

  request->send(200, "text/html", html);
}

void handleJsonRequest(AsyncWebServerRequest *request, ArduinoJson::JsonVariant &json) {
  String jsonString;
  json["state"] = PCState;
  serializeJson(json, jsonString);
  request->send(200, "application/json", jsonString);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
        //Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        //IPAddress ip = webSocket.remoteIP(num);
        webSocket.broadcastTXT(event);
        //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:
      // handle text messages
      break;
    case WStype_BIN:
      // handle binary messages
      break;
    case WStype_ERROR:
      // handle errors
      break;
    case WStype_FRAGMENT_TEXT_START:
      // handle text fragments
      break;
    case WStype_FRAGMENT_BIN_START:
      // handle binary fragments
      break;
    case WStype_FRAGMENT:
      // handle fragments
      break;
    case WStype_FRAGMENT_FIN:
      // handle end of fragments
      break;
    case WStype_PING:
      // handle ping
      break;
    case WStype_PONG:
      // handle pong
      break;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(optoPin, OUTPUT);
  digitalWrite(optoPin, LOW);

  pinMode(statePin, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("_");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/toggle", HTTP_GET, handleToggle);

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    handleJsonRequest(request, response->getRoot());
    delete response;
  });

  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/state", [](AsyncWebServerRequest *request, JsonVariant &json) {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }

    // Get Inf-State data from the JSON object or array
    bool state = bool(data["state"]);

    // Create a new JSON object to hold the Inf-State data
    StaticJsonDocument<200> responseJson;
    responseJson["State"] = state;

    if (PCState != state) {
      toggle_state = true;
      event = (state) ? "ON" : "OFF";
      webSocket.broadcastTXT(event);
    }

    String response;
    serializeJson(responseJson, response);
    request->send(200, "application/json", response);
    Serial.println(state);
  });

  server.addHandler(handler);

  server.begin();
  Serial.println("Server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (millis() - lastTime >= 100) { lastTime = millis(); return; }

  if (digitalRead(statePin) != PCState) {
    PCState = !PCState;
    event = (PCState) ? "ON" : "OFF";
    webSocket.broadcastTXT(event);
  }

  if (toggle_state) {
    PCState = !PCState;
    Serial.println(PCState ? "Turning on" : "Turning off");
    event = (PCState) ? "ON" : "OFF";
    webSocket.broadcastTXT(event);
    digitalWrite(optoPin, HIGH);
    delay(2000);
    digitalWrite(optoPin, LOW);
    toggle_state = false;
  }

  webSocket.loop();
}