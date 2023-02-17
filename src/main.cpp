#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <TaskScheduler.h>
#include "LittleFS.h"
#include "secrets.h"

const char* ssid = SSID;
const char* password = PASSWORD;

int optoPin = 13;  // D7
int statePin = 15; // D8
int PCState = false;

bool PCStateCheck = false;
bool toggle_state = false;
String event = "OFF";

Scheduler checkPCStaterunner;

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

void checkPCState() {
  PCStateCheck = true;
}

Task checkPCStateTask(1000, TASK_FOREVER, &checkPCState);

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

  checkPCStaterunner.addTask(checkPCStateTask);
  checkPCStateTask.enable();

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

  server.begin();
  Serial.println("Server started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  checkPCStaterunner.execute();
  if (PCStateCheck) {
    int currentState = digitalRead(statePin);
    if (currentState != PCState) {
      PCState = currentState;
      if (PCState) {
        event = "ON";
      } else {
        event = "OFF";
      }
      webSocket.broadcastTXT(event);
    }
    PCStateCheck = false;
  }

  if (toggle_state) {
    PCState = !PCState;
    if (PCState) {
      Serial.println("Turning on");
      event = "ON";
      digitalWrite(optoPin, HIGH);
      delay(2000);
      digitalWrite(optoPin, LOW);
    } else {
      Serial.println("Turning off");
      event = "OFF";
      digitalWrite(optoPin, HIGH);
      delay(2000); 
      digitalWrite(optoPin, LOW);
    }
    toggle_state = false;
    webSocket.broadcastTXT(event);
  }

  webSocket.loop();
}