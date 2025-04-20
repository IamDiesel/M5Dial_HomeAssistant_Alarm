/**
 * @file StickCP2HA
 * @author D. K.
 * @brief StickCP2 Home Assistant Reading and writing HA Entities via HA REST API. Starting alarm when armed and BT-Beacon Entity is over -130 dB
 * @version 0.3
 * @date 2025-04-08
 *
 *
 * @Hardwares: StickCP2
 TODO: Refactor Code :D
 */

#include "M5StickCPlus2.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <string.h>
#include "secrets.h"

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;
String sensorReadings ="";
String lolaBeaconNotifyHAss = "";
String lolaBeaconSignalDBHass = "";
String wifiLine = "";
//String serverName = "http://192.168.2.43:8123/api/states/input_number.bluecat_received_signal_strength";
bool useProxy = true;
/*Definition of states
OFF_NO_ALARM->Notify is off
ON_GONE: Notify is on, kippy beacon not in range
On_HOME: Notify is on, kippy beacon is in range
*/
enum State {
  OFF_NO_ALARM,
  ON_GONE,
  ON_HOME,
  OFF_DISPLAY,
  ERROR,
  ERROR_NO_BEEP
};
enum State curState = OFF_NO_ALARM;
enum State nextState = OFF_NO_ALARM;
enum State restoreState = OFF_NO_ALARM;

int i = 0;
int soundCount = 0;
int master_volume = 160;
int battery = 0;
bool settingDisplayIsBright = true;
int status = 0;

int connectWifi()
{
    lastTime = millis();    
    WiFi.begin(proxySSID, proxyPassword);
    StickCP2.Display.drawString("CON. WIFI PROXY",StickCP2.Display.width() / 2,StickCP2.Display.height() *1/2);
    while(WiFi.status() != WL_CONNECTED && (millis() - lastTime) < (timerDelay*4)) 
    {
        delay(500);
        StickCP2.Display.drawString(".",StickCP2.Display.width() / 3+(i++*5),StickCP2.Display.height() *2/3);
    }
    i=0;
    status = WiFi.status();
    if(status != WL_CONNECTED)
    {
        StickCP2.Display.clear();
        lastTime = millis();    
        WiFi.begin(ssid, password);
        StickCP2.Display.drawString("CON. WIFI HOME",StickCP2.Display.width() / 2,StickCP2.Display.height() *1/2);
        while(WiFi.status() != WL_CONNECTED && (millis() - lastTime) < (timerDelay*4)) 
        {
            delay(500);
            StickCP2.Display.drawString(".",StickCP2.Display.width() / 3+(i++*5),StickCP2.Display.height() *2/3);
        }
        i=0;
        status = WiFi.status();
        useProxy = false;
    }
    StickCP2.Display.clear();
    if(status != WL_CONNECTED)
    {
        StickCP2.Display.drawString("CON FAILED",StickCP2.Display.width() / 3,StickCP2.Display.height() *1/2);
        delay(2000);
        StickCP2.Display.clear();
    }
        
    return status;
}

void setup() {
    auto cfg = M5.config();
    Serial.begin(115200);
    StickCP2.begin(cfg);
    StickCP2.Power.begin();
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);
    status = connectWifi();
    StickCP2.Display.clear();
    StickCP2.Display.drawString(std::to_string(status).c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *1/ 4);
    StickCP2.Display.drawString(WiFi.localIP().toString().c_str(),StickCP2.Display.width() / 2, StickCP2.Display.height() *2 / 4);
    delay(2000);
    //StickCP2.Encoder.write(long(master_volume));
    StickCP2.Display.clear();
    Serial.println("Setup complete");
}
//beepInterval+toneDuration in ms
void beepAndBlink(int beepInterval, int toneFrequency, int toneDuration, bool blink)
{
    static unsigned long last_beep = 0;
    static bool toggleLED = true;
    if ((millis() - last_beep) > beepInterval)
    {
            last_beep = millis();
            StickCP2.Speaker.tone(toneFrequency, toneDuration);
            if(blink)
            {
                if(toggleLED)
                    StickCP2.Power.setLed(255);
                else
                    StickCP2.Power.setLed(0);
                toggleLED = !toggleLED;
            }
    }
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
    StickCP2.Display.drawString("Lola Beacon:", StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);
                            
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
    StickCP2.Display.drawString("Lola Beacon", StickCP2.Display.width() / 2, StickCP2.Display.height() * 5 / 8);
                            
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
void drawErrorScreen(bool beep)
{
    
    StickCP2.Display.setBrightness(255);
    StickCP2.Display.setTextColor(RED);
    StickCP2.Display.setTextSize(1);
    StickCP2.Display.drawString("ERROR",StickCP2.Display.width() / 2, StickCP2.Display.height() *2/ 8);
    StickCP2.Display.setTextSize(0.6);
    StickCP2.Display.drawString("Home Assistant unreachable",StickCP2.Display.width() / 2, StickCP2.Display.height() *3/ 8);
    //StickCP2.Display.setTextSize(0.4);
    StickCP2.Display.setTextSize(0.6);
    StickCP2.Display.drawString("If error persists:",StickCP2.Display.width() / 2, StickCP2.Display.height() *5 / 8);
    StickCP2.Display.drawString("1) Check Proxy / WiFi AP",StickCP2.Display.width() / 2, StickCP2.Display.height() *6 / 8);
    StickCP2.Display.drawString("2) Reboot Home Assistant",StickCP2.Display.width() / 2, StickCP2.Display.height() *7 / 8);
    if(beep)
    {
        beepAndBlink(1000,2000,100,true);
    }
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
void drawWIFIStatus()
{
    StickCP2.Display.setTextColor(YELLOW);
    StickCP2.Display.setTextSize(0.7);
    StickCP2.Display.drawString(wifiLine.c_str(), StickCP2.Display.width() *5 / 6,StickCP2.Display.height() / 10);

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
void drawInfoBar()
{
    drawWIFIStatus();
    drawBattery();
}

String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;
  String payload = "{\"state\":\"502\"}"; 


  if(useProxy)
  {
        http.setRedirectLimit(5);

        //if(http.begin(client, proxyServerName+newServerName)==-1)
        if(http.begin(client, proxyServer, proxyPort, serverName, true)==-1)
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
  // GET request
  int httpResponseCode = http.GET();

  
  if (httpResponseCode>0) {
    Serial.println(httpResponseCode);
    payload = http.getString();
  }

  // Free resources
  http.end();
  return payload;
}
int httpPostState(String serverName, String value) {
  WiFiClient client;
  HTTPClient http;

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
  Serial.print("POST:"+String(httpResponseCode));
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
    bool buttonPPressed = false;
    bool buttonAHold = false;
    bool buttonBHold = false;
    bool buttonAReleased = false;
    bool buttonBReleased = false;
    bool buttonPReleased = false;
    static bool isOddWifiCycle = true;
    static int httpStatusCode = 200;
    //******************
    //update inputs
    //******************
    if ((millis() - lastTime) > timerDelay) 
    {
        if(WiFi.status()== WL_CONNECTED)
        {
            lolaBeaconNotifyHAss = getHAssEntityStateValueAsString("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active");
            lolaBeaconSignalDBHass = getHAssEntityStateValueAsString("http://192.168.2.43:8123/api/states/input_number.bluecat_received_signal_strength");
            if(useProxy && isOddWifiCycle)
                wifiLine = "PRXY\\";
            else if(useProxy && !isOddWifiCycle)
                wifiLine = "PRXY/";
            else if(!useProxy && isOddWifiCycle)
                wifiLine = "HOME\\";
            else if(!useProxy && !isOddWifiCycle)
                wifiLine = "HOME/";           
        }
        else 
        {
            wifiLine="NO WIFI";
        }
        isOddWifiCycle = !isOddWifiCycle;
        lastTime = millis();
        StickCP2.Display.clear();
        drawInfoBar();
    }
    buttonAPressed = StickCP2.BtnA.wasClicked();
    buttonBPressed = StickCP2.BtnB.wasClicked();
    //buttonPPressed = StickCP2.BtnPWR.wasSingleClicked();
    
    buttonAHold = StickCP2.BtnA.wasHold();
    buttonBHold = StickCP2.BtnB.wasHold();
    //buttonAReleased = StickCP2.BtnA.was

    //******************
    //execute state actions, determine next State and execute transition actions
    //******************
    switch(curState)
    {
        case OFF_NO_ALARM:
                    restoreState = OFF_NO_ALARM;
                    drawScreenOff(); //state action
                    if(buttonAPressed)                                                                                                     //If user presses button                      --> ON_GONE
                    {
                        nextState = ON_GONE;
                        //Transition actions:
                        httpStatusCode = httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","on");
                        StickCP2.Display.clear();
                        drawScreenOffActivated();
                        delay(2000);
                    }
                    else
                        nextState = OFF_NO_ALARM;
                    if(buttonBHold == true)
                    {
                        nextState = OFF_DISPLAY;
                        StickCP2.Display.setBrightness(0);
                    }
                        
                    break;
        case ON_GONE:
                    restoreState = ON_GONE;
                    drawScreenOnGone(); //state action
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconSignalDBHass.toInt() < -60 && lolaBeaconNotifyHAss.equals("on")) //If beacon is in range and notify is on         --> ON_HOME
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("off"))                                                                              //if notify is off                               --> OFF_NO_ALARM              
                        nextState = OFF_NO_ALARM;
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))                                   //if beacon is not in range and notify is on     --> ON_GONE  (stay)                            
                        nextState = ON_GONE;
                    if(buttonAPressed)                                                                                                      //if user presses button                         --> OFF_NO_ALARM
                    {
                        nextState = OFF_NO_ALARM;
                        httpStatusCode = httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                    }
                    if(buttonBHold == true)
                    {
                        nextState = OFF_DISPLAY;  
                        StickCP2.Display.setBrightness(0);
                    }
                    break;
        case ON_HOME:
                    restoreState = ON_HOME;
                    drawScreenOnHome(); //state action
                    if(lolaBeaconSignalDBHass.toInt() > -130 && lolaBeaconNotifyHAss.equals("on"))                                          //If beacon is in range and notify is on         --> ON_HOME (stay)                                       
                        nextState = ON_HOME;
                    else if(lolaBeaconNotifyHAss.equals("off"))                                                                     //if notify is off                               --> OFF_NO_ALARM
                    {
                        nextState = OFF_NO_ALARM;
                        StickCP2.Power.setLed(0);
                    }
                    else if(lolaBeaconSignalDBHass.toInt() <= -130 && lolaBeaconNotifyHAss.equals("on"))                                    //if beacon is not in range and notify is on     --> ON_GONE
                    {
                        nextState = ON_GONE;
                        StickCP2.Power.setLed(0);
                    }
                    if(buttonAPressed)                                                                                                       //if user presses button                         --> OFF_NO_ALARM
                    {
                        nextState = OFF_NO_ALARM;
                        httpStatusCode = httpPostState("http://192.168.2.43:8123/api/states/input_boolean.bluecat_kippy_gps_active","off");
                        StickCP2.Power.setLed(0);
                    }
                    if(buttonBHold == true)
                    {
                        nextState = OFF_DISPLAY;
                        StickCP2.Display.setBrightness(0);
                    }
                    break;
        case OFF_DISPLAY:
                    nextState = OFF_DISPLAY;
                    if(buttonBHold || buttonBPressed || buttonAPressed)
                    {
                        nextState = restoreState;
                        StickCP2.Display.setBrightness(255);
                    }
                        
                    if((restoreState == ON_GONE || restoreState == ON_HOME) && lolaBeaconSignalDBHass.toInt() > -130)
                    {
                        nextState = ON_HOME;
                        StickCP2.Display.setBrightness(255);
                    }
                    break;
        case ERROR:
                    nextState = ERROR;

                    httpStatusCode = httpPostState("http://192.168.2.43:8123/api/states/input_boolean.poll_me","off");
                    if(lolaBeaconNotifyHAss != "502" && httpStatusCode == 200)
                    {
                        StickCP2.Display.setBrightness(255);
                        StickCP2.Power.setLed(0);
                        nextState = restoreState;
                    }
                    Serial.println("Error: "+String(httpStatusCode)+" " +lolaBeaconNotifyHAss);
                    drawErrorScreen(true);
                    if(buttonBHold || buttonBPressed || buttonAPressed)
                        nextState = ERROR_NO_BEEP;
                    break;
        case ERROR_NO_BEEP:
                    nextState = ERROR_NO_BEEP;
                    httpStatusCode = httpPostState("http://192.168.2.43:8123/api/states/input_boolean.poll_me","off");
                    if(lolaBeaconNotifyHAss != "502" && httpStatusCode == 200)
                    {
                        StickCP2.Display.setBrightness(255);
                        StickCP2.Power.setLed(0);
                        nextState = restoreState;
                    }
                    Serial.println("Error: "+String(httpStatusCode)+" " +lolaBeaconNotifyHAss);
                    drawErrorScreen(false);
                    break;
        default:
                    break;
    }
    if(lolaBeaconNotifyHAss == "502" || lolaBeaconNotifyHAss == "" || httpStatusCode!= 200)
    {
        if(curState == ERROR_NO_BEEP || nextState == ERROR_NO_BEEP)
            nextState = ERROR_NO_BEEP;
        else
            nextState = ERROR;
    }
    if(nextState != curState)
        StickCP2.Display.clear();
    curState = nextState;


    //******************
    //2nd Task: Create Popup for alarm volume on user interaction via dial
    //******************
    //long encoder_newPosition = StickCP2.Encoder.read();
    if(buttonBPressed && buttonBHold == false)  //button B has been pressed
    {
        if(StickCP2.BtnB.getClickCount() > 1)
        {
            master_volume += 20;
            master_volume = master_volume % 161;
        }  
        drawPopupScreenVolume();
    }
    /*if(buttonBHold == true)
    {
        if(settingDisplayIsBright)
            StickCP2.Display.setBrightness(0);
        else
            StickCP2.Display.setBrightness(255);
        settingDisplayIsBright = !settingDisplayIsBright;
    }
    if(buttonAPressed || buttonBPressed || curState == ON_HOME)
    {
        StickCP2.Display.setBrightness(255);
        settingDisplayIsBright = true;
    }*/
    //check if battery is nearly empty and activate power LED / beep, if thats the case
    empty_battery_notification();
    
}