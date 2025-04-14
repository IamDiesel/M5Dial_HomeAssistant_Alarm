# M5Dial / M5StickCPlus2 HomeAssistant Alarm
From M5Dial M5Stick set button value in Home Assistant and poll sensor value that can trigger an alarm on the M5Dial
![image](https://github.com/user-attachments/assets/246e2db2-0dcd-4b51-96d4-f6f72a8777fc)

![WhatsApp Bild 2025-04-09 um 15 16 25_37731e9d](https://github.com/user-attachments/assets/abb95c94-26bb-4f0d-86e1-3fab8fa2de3c)
![WhatsApp Bild 2025-04-09 um 15 16 25_30c7c64c](https://github.com/user-attachments/assets/b2aedcb1-fc76-4a0d-afb6-08681e1fb32b)

# Commissioning
* Follow steps from M5 Arduino tutorial https://docs.m5stack.com/en/arduino/arduino_ide

# Additional Information
* Home Assistant REST API: https://developers.home-assistant.io/docs/api/rest/
* Code snippet http-POST to Home Assistant REST API:
~~~
#include "M5Dial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <string.h>
const char* ssid = "yourWIFISSID";
const char* password = "yourWIFIPassword";
const char * token = "Bearer ...yourverylonghomeassistanttoken...";
WiFi.begin(ssid, password);
int httpPostState(const char* serverName, String value) {
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);
  //add headers
  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/json");
  // Send HTTP POST request
  String data = "{\"state\": \"" + value + "\"}";
  int httpResponseCode = http.POST(data);
  http.end();
  return httpResponseCode;
}
httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","on");
~~~
* Code snippet http-GET from Home Assistant REST API:
~~~
#include "M5Dial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <string.h>

String serverName = "http://192.168.2.43:8123/api/states/input_number.bluecat_received_signal_strength";
const char* ssid = "yourWIFISSID";
const char* password = "yourWIFIPassword";
const char * token = "Bearer ...yourverylonghomeassistanttoken...";

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  //add headers
  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/json");
  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}"; 
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}
sensorReadings = httpGETRequest(serverName.c_str());
JSONVar myObject = JSON.parse(sensorReadings);
JSONVar value = myObject["state"];
int lolaBeaconSignalDB = int(value);
~~~

