# WebControlPC (WCP)
A simple web control application to toggle the power state of a computer connected to an ESP8266 module.

# Hardware requirements
1. [ESP8266 module](https://www.amazon.com/s?k=esp8266)
2. [Optocoupler](https://www.amazon.com/s?k=Optokoppler+PC817)
 use 220 ohms resistance
3. Some wires
4. PC with a power switch

# Installation
1. Clone the repository to your local machine
2. Open the project in [vscode](https://code.visualstudio.com/download) 
. [installation](https://www.youtube.com/watch?v=ft89u3hcb3c)
3. Install [platformio](https://www.youtube.com/watch?v=sm6QxJkWcSc)
4. Install the required libraries:
* ArduinoJson
* ESPAsyncWebServer
* WebSocketsServer
* LittleFS
5. Modify the secrets.h file with your WiFi credentials
### secrets.h
```
#define SSID "SSID"
#define PASSWORD "PASSWORD"
```
5. Upload the sketch to your ESP8266 module

# Usage
1. Connect the ESP8266 module to your PC via the optocoupler
2. Connect to the same WiFi network as your ESP8266 module
3. Open a web browser and navigate to the IP address of your ESP8266 module
4. Use the toggle button on the web interface to switch the power state of your PC
5. The power state of your PC will be broadcasted to all connected WebSocket clients

### JSON API
* The API is accessible through the "/state" endpoint, and clients can send a GET request to retrieve the current state of the server.
* You also can sent data with a Json POST request to set the PC state

Python Exmaple:
```
import requests

url = 'http://your-ip-here/state'
headers = {'Content-Type': 'application/json'}
data = {'state': True}

response = requests.post(url, headers=headers, json=data)

print(response.status_code) # prints the HTTP status code of the response

```

# Credits
This project was created by VoidDeficit and is licensed under the MIT License.
