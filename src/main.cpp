#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <AsyncJson.h>
#include <WebSocketsServer.h>
#include "LittleFS.h"
#include "secrets.h"

const char* ssid = SSID;
const char* password = PASSWORD;
const char* hostname = "home-pc-control"; // Change this to your desired hostname

int optoPin = 13;  // D7
int statePin = 15; // D8
int PCState = false;

unsigned long lastTime = 0;
unsigned long lastchangingTime = 0;

bool demo_mode = false;
bool toggle_state = false;
bool changing = false;
bool powerOnStartup = false;

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
  File cssFile = LittleFS.open("/style.css", "r");

  if (!indexFile) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  String html = indexFile.readString();
  indexFile.close();

  //String css = cssFile.readString();
  //cssFile.close();

  //html.replace("</head>", "<style>" + css + "</style></head>");
  request->send(200, "text/html", html);
}

void handleCss(AsyncWebServerRequest *request) {
  if (!LittleFS.begin()) {
    request->send(500, "text/plain", "LittleFS initialization failed");
    return;
  }

  File cssFile = LittleFS.open("/style.css", "r");
  if (!cssFile) {
    request->send(404, "text/plain", "File not found");
    return;
  }

  String css = cssFile.readString();
  cssFile.close();
  request->send(200, "text/css", css);
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
        IPAddress ip = webSocket.remoteIP(num);
        webSocket.broadcastTXT(("pcstate="+String(PCState)).c_str());
        webSocket.broadcastTXT(("power_on_startup="+String(powerOnStartup)).c_str());
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT: {
      //handle text messages
      Serial.printf("[%u] Text message: %s\n", num, (char*)payload);
      String message = String((char*)payload);
      if (message.startsWith("setting_changed: power_on_startup=")) {
        // Extract the value of the switch from the message
        Serial.println(message.substring(strlen("setting_changed: power_on_startup=")));
        if (message.substring(strlen("setting_changed: power_on_startup=")) == "true") {
          powerOnStartup = 1;
        } else {
          powerOnStartup = 0;
        }
        Serial.print("power_on_startup=");
        Serial.println(powerOnStartup);

        webSocket.broadcastTXT(("power_on_startup="+String(powerOnStartup)).c_str());

        // Save the switch value to the Flash memory
        Preferences preferences;
        preferences.begin("settings", false);
        preferences.putBool("power_on_startup", powerOnStartup);
        preferences.end();
      }
      }
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
      // handle
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize variables and pins

  Preferences preferences;
  preferences.begin("settings", true);
  powerOnStartup = preferences.getBool("power_on_startup", false);
  preferences.end();

  pinMode(optoPin, OUTPUT);
  digitalWrite(optoPin, LOW);

  pinMode(statePin, INPUT);

  if (powerOnStartup) {
    if (digitalRead(statePin) != true) {
      PCState = !PCState;
      Serial.println("Turning on");
      changing = true;
      digitalWrite(optoPin, HIGH);
      delay(2000);
      digitalWrite(optoPin, LOW);
      changing = false;
      Serial.println("Finish");
    }
  }

  WiFi.hostname(hostname); // Set the hostname
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("_");
  }
  Serial.print("\n");
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  

  server.on("/", HTTP_GET, handleRoot);
  server.on("/style.css", HTTP_GET, handleCss);
  server.on("/toggle", HTTP_GET, handleToggle);

  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncJsonResponse *response = new AsyncJsonResponse();
    handleJsonRequest(request, response->getRoot());
    delete response;
  });
  	
  // JSON Api
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
      webSocket.broadcastTXT(("pcstate="+String(PCState)).c_str());
    }

    String response;
    serializeJson(responseJson, response);
    request->send(200, "application/json", response);
    Serial.println(state);
  });

  server.addHandler(handler);

  server.begin();
  Serial.println("Server started");
  Serial.print("power_on_startup=");
  Serial.println((powerOnStartup) ? "on" : "off");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (millis() - lastTime >= 100) { lastTime = millis(); return; }

  if (toggle_state && !changing) {
    PCState = !PCState;
    Serial.println(PCState ? "Turning on" : "Turning off");
    changing = true;
    webSocket.broadcastTXT(("pcstate="+String(PCState)).c_str());
    digitalWrite(optoPin, HIGH);
  } else if (millis() - lastchangingTime >= 2000) {
    lastchangingTime = millis();
    if (toggle_state && changing) {
      digitalWrite(optoPin, LOW);
      toggle_state = false;
      changing = false;
      Serial.println("Finish");
    }
  }

  //digitalRead(statePin) != PCState
  if (digitalRead(statePin) != PCState && demo_mode == false) {
    PCState = !PCState;
    webSocket.broadcastTXT(("pcstate="+String(PCState)).c_str());
  }

  webSocket.loop();
}
