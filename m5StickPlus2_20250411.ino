/**
 * @file StickCP2HA
 * @author D. K.
 * @brief StickCP2 Home Assistant Reading and writing HA Entities via HA REST API. Starting alarm when armed and BT-Beacon Entity is over -130 dB
 * @version 0.2
 * @date 2025-04-08
 *
 *
 * @Hardwares: StickCP2
 */

#include "M5StickCPlus2.h"
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
int master_volume = 120;
//long encoder_oldPosition = -999;
int battery = 0;

int status = 0;
void setup() {
    auto cfg = M5.config();
    Serial.begin(115200);
    Serial.println("Setup complete");
    StickCP2.begin(cfg);
    StickCP2.Power.begin();
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        StickCP2.Display.drawString(".",StickCP2.Display.width() / 3+(i++*5),StickCP2.Display.height() *1/2);
    }
    i=0;
    status = WiFi.status();
    StickCP2.Display.clear();
    StickCP2.Display.drawString(std::to_string(status).c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *1/ 4);
    StickCP2.Display.drawString(WiFi.localIP().toString().c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *2 / 4);
    delay(2000);
    //StickCP2.Encoder.write(long(master_volume));
    StickCP2.Display.clear();
    Serial.println("Setup complete");
}

void drawScreenOff(int statusCode){
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("Notify\n[Off]", StickCP2.Display.width() / 2,StickCP2.Display.height() / 2);
    StickCP2.Display.drawString(std::to_string(statusCode).c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);   
}
void drawScreenOff(){
    StickCP2.Display.setTextColor(DARKGREY);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("Notify [Off]", StickCP2.Display.width() / 2,StickCP2.Display.height() / 2-20);
    StickCP2.Display.setTextSize(0.7);
    StickCP2.Display.setTextColor(BLUE);
    StickCP2.Display.drawString("Press M5 to activate",StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8-5);
    StickCP2.Display.setTextColor(DARKGREY);
    StickCP2.Display.drawString(lolaBeaconSignalDBHass+" dB", StickCP2.Display.width() / 2,StickCP2.Display.height() * 6 / 8);
}
void drawScreenOffActivated(){
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("Notify [Off]", StickCP2.Display.width() / 2,StickCP2.Display.height() / 2-20);
    StickCP2.Display.setTextSize(0.7);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.drawString("activating...",StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);   
}
void drawScreenOnGone()
{
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("Notify", StickCP2.Display.width() / 2, StickCP2.Display.height() * 3 / 8);
    StickCP2.Display.drawString("Lola", StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);
                            
    StickCP2.Display.setTextSize(0.8);
    StickCP2.Display.setTextColor(SILVER);
    StickCP2.Display.drawString(lolaBeaconNotifyHAss, StickCP2.Display.width() / 2, StickCP2.Display.height() * 4 / 8);
    StickCP2.Display.drawString(lolaBeaconSignalDBHass+" dB", StickCP2.Display.width() / 2,StickCP2.Display.height() * 6 / 8);
}
void drawScreenOnHome()
{
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("Notify", StickCP2.Display.width() / 2, StickCP2.Display.height() * 3 / 8);
    StickCP2.Display.drawString("Lola", StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);
                            
    StickCP2.Display.setTextSize(0.8);
    StickCP2.Display.setTextColor(SILVER);
    StickCP2.Display.drawString(lolaBeaconNotifyHAss, StickCP2.Display.width() / 2, StickCP2.Display.height() * 4 / 8);
    StickCP2.Display.drawString(lolaBeaconSignalDBHass+" dB", StickCP2.Display.width() / 2,StickCP2.Display.height() * 6 / 8);
    
    if(soundCount++ % 200 == 0)
    {
    StickCP2.Power.setLed(255);
    //StickCP2.Display.fillArc(StickCP2.Display.width() / 2,StickCP2.Display.height()/2, 10, 20, 20, 240, TFT_WHITE);
    StickCP2.Display.fillArc(StickCP2.Display.width() / 2,StickCP2.Display.height()/2, 100, 250, 0, 360, TFT_WHITE);
    StickCP2.Speaker.tone(2000, 150);
    delay(150);
    StickCP2.Speaker.tone(2500, 150);
    delay(150);
    StickCP2.Speaker.tone(2000, 150);
    delay(150); 
    }
    else
    {
        StickCP2.Power.setLed(0);
    }
  
}
void drawScreenWifi(String textline)
{
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString(std::to_string(status).c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *1/ 4);
    StickCP2.Display.drawString(WiFi.localIP().toString().c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *2 / 4);
    StickCP2.Display.setTextSize(0.6);
    StickCP2.Display.setTextColor(SILVER);
    StickCP2.Display.drawString(textline.c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *3 / 4);
}
void drawPopupScreenVolume()
{
    StickCP2.Display.clear();
    StickCP2.Speaker.setVolume(master_volume);
    StickCP2.Display.fillArc(StickCP2.Display.width() / 2,StickCP2.Display.height()/2, 45, 55, 0.0, 1.0*float(master_volume)/256.0*360.0, TFT_ORANGE);
    StickCP2.Display.setTextColor(TFT_ORANGE);
    StickCP2.Display.setTextSize(0.4);
    StickCP2.Display.drawString("Volume", StickCP2.Display.width() / 2,StickCP2.Display.height() / 10 *2);
}
void drawBattery()
{
    StickCP2.Display.setTextColor(YELLOW);
    StickCP2.Display.setTextSize(0.7);
    int lvl = StickCP2.Power.getBatteryLevel();

    StickCP2.Display.setCursor(5, 12);
    StickCP2.Display.printf("BAT: %d", (lvl+battery)/2);
    battery = lvl;
    //StickCP2.Display.printf("%");
}
void empty_battery_notification()
{
    static unsigned long last_battery_beep = 0;
    if(battery <= 5 && curState != ON_HOME)
    {
        StickCP2.Power.setLed(125);
        if (battery < 2 && (millis() - last_battery_beep) > 10000)
        {
            last_battery_beep = millis();
            StickCP2.Speaker.tone(1000, 500);
        }
    }
    else
        StickCP2.Power.setLed(0);

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
    StickCP2.update();
    int statusCode = 0;
    bool buttonAPressed = false;
    bool buttonBPressed = false;
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
        StickCP2.Display.clear();
        drawBattery();
    }
    buttonAPressed = StickCP2.BtnA.wasPressed();
    buttonBPressed = StickCP2.BtnB.wasPressed();

    //******************
    //execute state actions, determine next State and execute transition actions
    //******************
    switch(curState)
    {
        case OFF:
                    drawScreenOff(); //state action
                    if(buttonAPressed)                                                                                                     //If user presses button                      --> ON_GONE
                    {
                        nextState = ON_GONE;
                        //Transition actions:
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","on");
                        StickCP2.Display.clear();
                        //drawBattery();
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
                    if(buttonAPressed)                                                                                                      //if user presses button                         --> OFF
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
                    {
                        nextState = OFF;
                        StickCP2.Power.setLed(0);
                    }
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))                                    //if beacon is not in range and notify is on     --> ON_GONE
                    {
                        nextState = ON_GONE;
                        StickCP2.Power.setLed(0);
                    }
                    if(buttonAPressed)                                                                                                       //if user presses button                         --> OFF
                    {
                        nextState = OFF;
                        httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                        StickCP2.Power.setLed(0);
                    }
                    break;
        default:
                    break;
    }
    if(nextState != curState)
        StickCP2.Display.clear();
        //drawBattery();
    curState = nextState;
    //elay(100);


    //******************
    //2nd Task: Create Popup for alarm volume on user interaction via dial
    //******************
    //long encoder_newPosition = StickCP2.Encoder.read();
    if(buttonBPressed)  //button B has been pressed
    {    
        master_volume += 20;
        master_volume = master_volume % 161;
        drawPopupScreenVolume();
    }
    //check if battery is nearly empty and activate power LED / beep, if thats the case
    empty_battery_notification();
    
}