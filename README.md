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
* How to use twingate on a mobile phone as proxy to connect the M5 Dial/StickC to Home Assistant from outside the home network
1) Install twingate e.g. via docker on a device in the home network (where home assistant is running) and setup a twingate connector. More information see https://www.twingate.com/
2) Install "Twingate" app on android phone
3) Install  "Every Proxy" app on android phone
4) Create Hotspot on android phone: In this example "LolaNet" with "yourWifiPassword"
5) Start twingate on the android phone, sign in and connect to home network
6) Start "Every Proxy" app. Inside the app start the HTTP-Server. In this example the Proxy is hosted on: 192.168.192.3, port 8080
7) Code Snippet on how to GET and POST remotly from M5Dial/M5StickC via android hotspot to your home network Home Assistant using the Home Assistant REST API:
~~~
#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "LolaNet";
const char* password = "yourWifiPassword";
bool useProxy = true;
WiFi.begin(ssid, password);
while(WiFi.status() != WL_CONNECTED) 
{
    delay(500);
    M5Dial.Display.drawString(".",M5Dial.Display.width() / 3+(i++*5),M5Dial.Display.height() *1/2);
}
//GET Request
//serverName could be: serverName = http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active;
//Where 192.168.2.43 is the Home Assistant server
String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
  String payload = "{}"; 
  proxyServer = "192.168.192.3";
  int proxyPort = 8080;

  if(useProxy)
  {
        http.setRedirectLimit(5);

        //if(http.begin(client, proxyServerName+newServerName)==-1)
        if(http.begin(client, proxyServer, port, serverName, true)==-1)
            return payload;
        //http.setURL(serverName);
  }
  else
  {
      if(http.begin(client, serverName)==-1)
        {
            //Serial.println("httpGETRequest failed.");
            //http.end();
            return payload;
        }
  }
   
  //add headers
  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/json");
  // Send HTTP POST request
  int httpResponseCode = http.GET();

  
  if (httpResponseCode>0) {
    payload = http.getString();
  }

  // Free resources
  http.end();
  return payload;
}

//POST Request
int httpPostState(String serverName, String value) {
  WiFiClient client;
  HTTPClient http;
  String proxyServer = "192.168.192.3";
  int proxyPort = 8080;

    if(useProxy)
    {
        if(http.begin(client, proxyServer, proxyPort, serverName, true)==-1)
        {
            //http.end();
            //Serial.println("httpPost failed.");
            return -1;
        }
        //http.setURL(serverName);
    }
    else
    {
        if(http.begin(client, serverName) == -1)
        {
            //http.end();
            //Serial.println("httpPost failed.");
            return -1;
        }
    }

  //add headers
  http.addHeader("Authorization", token);
  http.addHeader("Content-Type", "application/json");
  // Send HTTP POST request
  String data = "{\"state\": \"" + value + "\"}";
  int httpResponseCode = http.POST(data);
  http.end();
  return httpResponseCode;
}
~~~
  
