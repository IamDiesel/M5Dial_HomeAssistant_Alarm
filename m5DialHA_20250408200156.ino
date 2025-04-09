/**
 * @file m5DialHA
 * @author D. K.
 * @brief M5Dial Home Assistant Reading and writing HA Entities. Starting alarm when armed and BT-Beacon Entity is over -130 dB
 * @version 0.1
 * @date 2025-04-08
 *
 *
 * @Hardwares: M5Dial
 */

#include "M5Dial.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <string.h>
#include "secrets.h"

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
String sensorReadings;
String lolaBeaconNotifyHAss = "";
String lolaBeaconSignalDBHass = "";
String serverName = "http://192.168.2.43:8123/api/states/input_number.bluecat_received_signal_strength";
enum State {
  OFF,
  ON_GONE,
  ON_HOME
};
enum State curState = OFF;
enum State nextState = OFF;

int i = 0;
int soundCount = 0;

int status = 0;
void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
    M5Dial.Display.setTextSize(1);
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5Dial.Display.drawString(".",M5Dial.Display.width() / 3+(i++*5),M5Dial.Display.height() *1/2);
    }
    i=0;
    status = WiFi.status();
    M5Dial.Display.clear();
    M5Dial.Display.drawString(std::to_string(status).c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() *1/ 4);
    M5Dial.Display.drawString(WiFi.localIP().toString().c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() *2 / 4);
    delay(2000);
    M5Dial.Display.clear();
}

void drawScreenOff(int statusCode){
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify\n[Off]", M5Dial.Display.width() / 2,M5Dial.Display.height() / 2);
    M5Dial.Display.drawString(std::to_string(statusCode).c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);   
}
void drawScreenOff(){
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify\n[Off]", M5Dial.Display.width() / 2,M5Dial.Display.height() / 2);
    M5Dial.Display.setTextSize(0.5);
    M5Dial.Display.drawString("Press BTN to Activate",M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);   
}
void drawScreenOffActivated(){
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify\n[Off]", M5Dial.Display.width() / 2,M5Dial.Display.height() / 2);
    M5Dial.Display.setTextSize(0.5);
    M5Dial.Display.drawString("activating...",M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);   
}
void drawScreenOnGone()
{
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify", M5Dial.Display.width() / 2, M5Dial.Display.height() * 1 / 8);
    M5Dial.Display.drawString("Lola", M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);
                            
    M5Dial.Display.setTextSize(0.8);
    M5Dial.Display.setTextColor(SILVER);
    M5Dial.Display.drawString(lolaBeaconNotifyHAss, M5Dial.Display.width() / 2, M5Dial.Display.height() * 2 / 8);
    M5Dial.Display.drawString(lolaBeaconSignalDBHass, M5Dial.Display.width() / 2,M5Dial.Display.height() * 6 / 8);
}
void drawScreenOnHome()
{
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify", M5Dial.Display.width() / 2, M5Dial.Display.height() * 1 / 8);
    M5Dial.Display.drawString("Lola", M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);
                            
    M5Dial.Display.setTextSize(0.8);
    M5Dial.Display.setTextColor(SILVER);
    M5Dial.Display.drawString(lolaBeaconNotifyHAss, M5Dial.Display.width() / 2, M5Dial.Display.height() * 2 / 8);
    M5Dial.Display.drawString(lolaBeaconSignalDBHass, M5Dial.Display.width() / 2,M5Dial.Display.height() * 6 / 8);
    if(soundCount++ % 200 == 0)
    {
    M5Dial.Speaker.tone(2000, 150);
    delay(150);
    M5Dial.Speaker.tone(2500, 150);
    delay(150);
    M5Dial.Speaker.tone(2000, 150);
    delay(150); 
    }
  
}
void drawScreenWifi(String textline)
{
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString(std::to_string(status).c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() *1/ 4);
    M5Dial.Display.drawString(WiFi.localIP().toString().c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() *2 / 4);
    M5Dial.Display.setTextSize(0.6);
    M5Dial.Display.setTextColor(SILVER);
    M5Dial.Display.drawString(textline.c_str(),M5Dial.Display.width() / 2, M5Dial.Display.height() *3 / 4);
}

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
    payload = http.getString();
  }

  // Free resources
  http.end();
  return payload;
}
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
JSONVar getHAssEntityStateValue(const char* serverName)
{
    sensorReadings = httpGETRequest(serverName);
    JSONVar myObject = JSON.parse(sensorReadings);
    if (JSON.typeof(myObject) == "undefined") 
    {
        return JSONVar("Parsing input failed!");;
    }
    JSONVar value = myObject["state"];
    return value;
}
String getHAssEntityStateValueAsString(const char* serverName)
{
    String value_result = "";
    JSONVar value = getHAssEntityStateValue(serverName);
    value_result = JSON.stringify(value);
    value_result = value_result.substring(1,value_result.length()-1);
    return value_result;
}
void loop(){
    int statusCode = 0;
    M5Dial.update();
    bool buttonPressed = false;
    String wifiLine = "";
    //******************
    //update inputs
    //******************
    if ((millis() - lastTime) > timerDelay) 
    {
        if(WiFi.status()== WL_CONNECTED)
        {
            lolaBeaconNotifyHAss = getHAssEntityStateValueAsString("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active");
            lolaBeaconSignalDBHass = getHAssEntityStateValueAsString("http://192.168.2.43:8123/api/states/input_number.bluecat_received_signal_strength");
            wifiLine = "Connected";
        }
        else 
        {
            wifiLine="WiFi Disconnected";
        }
        lastTime = millis();
        M5Dial.Display.clear();
    }
    buttonPressed = M5Dial.BtnA.wasPressed();

    //******************
    //execute state actions and determine next State
    //******************
    switch(curState)
    {
        case OFF:
                    drawScreenOff();
                    if(buttonPressed)
                    {
                        nextState = ON_GONE;
                        //TODO POST ON TO HA
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","on");
                        M5Dial.Display.clear();
                        drawScreenOffActivated();
                        delay(2000);
                    }
                    else
                        nextState = OFF;
                    break;
        case ON_GONE:
                    drawScreenOnGone();
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconSignalDBHass.toInt() < -60 && lolaBeaconNotifyHAss.equals("on"))
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("on") == false)
                        nextState = OFF;
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))
                        nextState = ON_GONE;
                    if(buttonPressed)
                    {
                        nextState = OFF;
                        //TODO POST OFF to HA
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                    }
                    break;
        case ON_HOME:
                    drawScreenOnHome();
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconNotifyHAss.equals("on"))
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("on") == false)
                        nextState = OFF;
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))
                        nextState = ON_GONE;
                    if(buttonPressed)
                    {
                        nextState = OFF;
                        //TODO POST OFF to HA
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                    }
                    break;
        default:
                    break;
    }
    if(nextState != curState)
        M5Dial.Display.clear();
    curState = nextState;
}