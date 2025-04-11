/**
 * @file m5DialHA
 * @author D. K.
 * @brief M5Dial Home Assistant Reading and writing HA Entities via HA REST API. Starting alarm when armed and BT-Beacon Entity is over -130 dB
 * @version 0.2
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
/*Definition of states
OFF->Notify is off
ON_GONE: Notify is on, kippy beacon not in range
On_HOME: Notify is on, kippy beacon is in range
*/
enum State {
  OFF,
  ON_GONE,
  ON_HOME
};
enum State curState = OFF;
enum State nextState = OFF;

int i = 0;
int soundCount = 0;
int master_volume = 128;
long encoder_oldPosition = -999;

int status = 0;
void setup() {
    auto cfg = M5.config();
    Serial.begin(115200);
    Serial.println("Setup complete");
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
    M5Dial.Encoder.write(long(master_volume));
    M5Dial.Display.clear();
    Serial.println("Setup complete");
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
    M5Dial.Display.drawString("Notify", M5Dial.Display.width() / 2, M5Dial.Display.height() * 3 / 8);
    M5Dial.Display.drawString("Lola", M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);
                            
    M5Dial.Display.setTextSize(0.8);
    M5Dial.Display.setTextColor(SILVER);
    M5Dial.Display.drawString(lolaBeaconNotifyHAss, M5Dial.Display.width() / 2, M5Dial.Display.height() * 4 / 8);
    M5Dial.Display.drawString(lolaBeaconSignalDBHass, M5Dial.Display.width() / 2,M5Dial.Display.height() * 6 / 8);
}
void drawScreenOnHome()
{
    M5Dial.Display.setTextColor(GREEN);
    M5Dial.Display.setTextSize(1);
    M5Dial.Display.drawString("Notify", M5Dial.Display.width() / 2, M5Dial.Display.height() * 3 / 8);
    M5Dial.Display.drawString("Lola", M5Dial.Display.width() / 2, M5Dial.Display.height() * 5 / 8);
                            
    M5Dial.Display.setTextSize(0.8);
    M5Dial.Display.setTextColor(SILVER);
    M5Dial.Display.drawString(lolaBeaconNotifyHAss, M5Dial.Display.width() / 2, M5Dial.Display.height() * 4 / 8);
    M5Dial.Display.drawString(lolaBeaconSignalDBHass, M5Dial.Display.width() / 2,M5Dial.Display.height() * 6 / 8);
    if(soundCount++ % 200 == 0)
    {
    //M5Dial.Display.fillArc(M5Dial.Display.width() / 2,M5Dial.Display.height()/2, 10, 20, 20, 240, TFT_WHITE);
    M5Dial.Display.fillArc(M5Dial.Display.width() / 2,M5Dial.Display.height()/2, 100, 250, 0, 360, TFT_WHITE);
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
void drawPopupScreenVolume()
{
    M5Dial.Display.clear();
    M5Dial.Speaker.setVolume(master_volume);
    M5Dial.Display.fillArc(M5Dial.Display.width() / 2,M5Dial.Display.height()/2, 90, 100, 0.0, 1.0*float(master_volume)/256.0*360.0, TFT_ORANGE);
    M5Dial.Display.setTextColor(TFT_ORANGE);
    M5Dial.Display.setTextSize(0.4);
    M5Dial.Display.drawString("Volume", M5Dial.Display.width() / 2,M5Dial.Display.height() / 10 *2);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  String payload = "{}"; 
  //Serial.println("httpGETRequest Start");
  if(http.begin(client, serverName)==-1)
  {
    //Serial.println("httpGETRequest failed.");
    //http.end();
    return payload;
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
int httpPostState(const char* serverName, String value) {
  WiFiClient client;
  HTTPClient http;
  if(http.begin(client, serverName) == -1)
    {
        //http.end();
        //Serial.println("httpPost failed.");
        return -1;
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
String getHAssEntityStateValue(const char* serverName)
{
    sensorReadings = httpGETRequest(serverName);
    JSONVar getResponseJSON = JSON.parse(sensorReadings);
    if (JSON.typeof(getResponseJSON) == "undefined" or getResponseJSON.hasOwnProperty("state") == false) 
    {
        return "";
    }
    JSONVar value = getResponseJSON["state"];
    String value_s = JSON.stringify(value);
    return value_s;
}
String getHAssEntityStateValueAsString(const char* serverName)
{
    String value_result = "";
    value_result = getHAssEntityStateValue(serverName);

    if(value_result.length()>0)
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
    //execute state actions, determine next State and execute transition actions
    //******************
    switch(curState)
    {
        case OFF:
                    drawScreenOff(); //state action
                    if(buttonPressed)                                                                                                     //If user presses button                      --> ON_GONE
                    {
                        nextState = ON_GONE;
                        //Transition actions:
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","on");
                        M5Dial.Display.clear();
                        drawScreenOffActivated();
                        delay(2000);
                    }
                    else
                        nextState = OFF;
                    break;
        case ON_GONE:
                    drawScreenOnGone(); //state action
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconSignalDBHass.toInt() < -60 && lolaBeaconNotifyHAss.equals("on")) //If beacon is in range and notify is on         --> ON_HOME
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("on") == false)                                                                    //if notify is off                               --> OFF              
                        nextState = OFF;
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))                                   //if beacon is not in range and notify is on     --> ON_GONE  (stay)                            
                        nextState = ON_GONE;
                    if(buttonPressed)                                                                                                      //if user presses button                         --> OFF
                    {
                        nextState = OFF;
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                    }
                    break;
        case ON_HOME:
                    drawScreenOnHome(); //state action
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconNotifyHAss.equals("on"))                                          //If beacon is in range and notify is on         --> ON_HOME (stay)                                       
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("on") == false)                                                                     //if notify is off                               --> OFF   
                        nextState = OFF;
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))                                    //if beacon is not in range and notify is on     --> ON_GONE
                        nextState = ON_GONE;
                    if(buttonPressed)                                                                                                       //if user presses button                         --> OFF
                    {
                        nextState = OFF;
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                    }
                    break;
        default:
                    break;
    }
    if(nextState != curState)
        M5Dial.Display.clear();
    curState = nextState;


    //******************
    //2nd Task: Create Popup for alarm volume on user interaction via dial
    //******************
    long encoder_newPosition = M5Dial.Encoder.read();
    if(encoder_newPosition != encoder_oldPosition)  //dial has been turned
    {    
        if(encoder_newPosition < 0) //dial left turn
        {
            M5Dial.Encoder.write(0);
            encoder_newPosition = 0;
        }
        if(encoder_newPosition > 128) //dial right turn
        {
            M5Dial.Encoder.write(128);
            encoder_newPosition = 128;
        }
        master_volume = int(encoder_newPosition) % 129;
        encoder_oldPosition = encoder_newPosition;
        drawPopupScreenVolume();
    }
}