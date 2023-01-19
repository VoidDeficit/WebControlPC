#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <TaskScheduler.h>
#include "secrets.h"

const char* ssid = SSID;
const char* password = PASSWORD;

int optoPin = 13;
int statePin = 15;
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
  String html = "<html><body>";
  html += "<h1>ESP8266 Web Server</h1>";
  html += "<p>PC State: <span id='pc-state'>Loading...</span></p>";
  html += "<form method='get' action='/toggle'>";
  html += "<input type='submit' value='Toggle'>";
  html += "</form>";
  html += "<script>";
  html += "var socket = new WebSocket('ws://' + window.location.hostname + ':81');";
  html += "socket.onmessage = function(event) {";
  html += "document.getElementById('pc-state').innerHTML = event.data;";
  html += "};";
  html += "</script>";
  html += "</body></html>";
  request->send(200, "text/html", html);
}

void checkPCState() {
  int currentState = digitalRead(statePin);
  if (currentState != PCState) {
    Serial.println("changed");
    PCState = currentState;
  }
  PCStateCheck = true;
}

Task checkPCStateTask(100, TASK_FOREVER, &checkPCState);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        //Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
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
    if (PCState) {
      event = "ON";
    } else {
      event = "OFF";
    }
    webSocket.broadcastTXT(event);
    PCStateCheck = false;
  }

  if (toggle_state) {
    PCState = !PCState;
    if (PCState) {
      Serial.println("Turning on");
      digitalWrite(optoPin, HIGH);
      delay(2000);
      digitalWrite(optoPin, LOW);
    } else {
      Serial.println("Turning off");
      digitalWrite(optoPin, HIGH);
      delay(2000); 
      digitalWrite(optoPin, LOW);
    }
    toggle_state = false;
  }

  webSocket.loop();
}