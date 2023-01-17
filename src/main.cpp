#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TaskScheduler.h>
#include "secrets.h"

const char* ssid = SSID;
const char* password = PASSWORD;

Scheduler runner;

int optoPin = 2;
int statePin = 15;
int PCState;

ESP8266WebServer server(80);

// 0 = HIGH // 1 = LOW
void checkPCState() {
  if (digitalRead(statePin) == HIGH) {
    PCState = HIGH;
  } else if (digitalRead(statePin) == LOW)
  {
    PCState = LOW;
  }
}

Task checkPCStateTask(1000, TASK_FOREVER, &checkPCState);

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>ESP8266 Web Server</h1>";
  html += "<p>PC State: <span id='pc-state'>Loading...</span></p>";
  html += "<form method='get' action='/toggle'>";
  html += "<input type='submit' value='Toggle'>";
  html += "</form>";
  html += "<script>";
  html += "var source = new EventSource('/events');";
  html += "source.onmessage = function(event) {";
  html += "document.getElementById('pc-state').innerHTML = event.data;";
  html += "};";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggle() {
  PCState = !PCState;
  Serial.print("Power ");
  Serial.println(PCState);
  digitalWrite(optoPin, HIGH);
  delay(2000);
  digitalWrite(optoPin, LOW);
  server.sendHeader("Location", "/", true);
  server.send(303);
}

void handleEvents() {
  String event = "data: ";
  if (PCState) {
    event += "ON";
  } else {
    event += "OFF";
  }
  event += "\n\n";
  server.send(200, "text/event-stream", event);
}

void setup() {
  Serial.begin(9600);

  pinMode(optoPin, OUTPUT);
  digitalWrite(optoPin, LOW);

  pinMode(statePin, INPUT);

  runner.addTask(checkPCStateTask);
  checkPCStateTask.enable();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("_");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/events", handleEvents);
  server.begin();
  Serial.println("Server started");
}

void loop() {
  runner.execute();
  server.handleClient();
}