# WebControlPC (WCP)
A simple web control application to toggle the power state of a computer connected to an ESP8266 module.

# Hardware requirements
1. ESP8266 module
2. [Optocoupler](www.amazon.com/s?k=Optokoppler+PC817)
 use 220 ohms resistance
4. Some wires
5. PC with a power switch

# Installation
1. Clone the repository to your local machine
2. Open the project in your Arduino IDE
3. Install the required libraries:
+ ArduinoJson
+ ESPAsyncWebServer
+ WebSocketsServer
+ LittleFS
4. Modify the secrets.h file with your WiFi credentials
5. Upload the sketch to your ESP8266 module

# Usage
Connect the ESP8266 module to your PC via the optocoupler
Connect to the same WiFi network as your ESP8266 module
Open a web browser and navigate to the IP address of your ESP8266 module
Use the toggle button on the web interface to switch the power state of your PC
The power state of your PC will also be broadcasted to all connected WebSocket clients
Web Interface
The web interface is hosted on port 80 and can be accessed by navigating to the IP address of your ESP8266 module.

WebSocket Interface
The WebSocket interface is hosted on port 81 and broadcasts the power state of your PC to all connected clients.

Credits
This project was created by VoidDeficit and is licensed under the MIT License.
