/*
 *  This sketch is running a web server for configuring WiFI if can't connect or for controlling of one GPIO to switch a light/LED
 *  Also it supports to change the state of the light via MQTT message and gives back the state after change.
 *  The push button has to switch to ground. It has following functions: Normal press less than 1 sec but more than 50ms-> Switch light. Restart press: 3 sec -> Restart the module. Reset press: 20 sec -> Clear the settings in EEPROM
 *  While a WiFi config is not set or can't connect:
 *    http://server_ip will give a config page with 
 *  While a WiFi config is set:
 *    http://server_ip/gpio -> Will display the GIPIO state and a switch form for it
 *    http://server_ip/gpio?state=0 -> Will change the GPIO directly and display the above aswell
 *    http://server_ip/cleareeprom -> Will reset the WiFi setting and rest to configure mode as AP
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected. (most likly it will be 192.168.4.1)
 * To force AP config mode, press button 20 Secs!
 *  For several snippets used, the credit goes to:
 *  - https://github.com/esp8266
 *  - https://github.com/chriscook8/esp-arduino-apboot
 *  - https://github.com/knolleary/pubsubclient
 *  - https://github.com/vicatcu/pubsubclient <- Currently this needs to be used instead of the origin
 *  - https://gist.github.com/igrr/7f7e7973366fc01d6393
 *  - http://www.esp8266.com/viewforum.php?f=25
 *  - http://www.esp8266.com/viewtopic.php?f=29&t=2745
 *  - And the whole Arduino and ESP8266 comunity
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <PubSubClient.h>

extern "C" {
  #include "user_interface.h" //Needed for the reset command
}

//***** Settings declare ********************************************************************************************************* 
String ssid = "WiFiSwitch"; //The ssid when in AP mode
String clientName ="WiFiSwitch"; //The MQTT ID -> MAC adress will be added to make it kind of unique
String FQDN ="WiFiSwitch"; //The DNS hostname - Does not work yet?
//select GPIO's
const int outPin = 13; //output pin
const int inPin = 0;  // input pin (push button)

const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec
const int setupTimeOutLimit=600; //Limit for setupTimeOut to reset within setup mode after some time. usefull after power break and WiFi not ready.
const int debug = 0; //Set to 1 to get more log to serial
//##### Object instances #####
ESP8266WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient;
Ticker btn_timer;
Ticker setupTimeOut_timer;


//##### Flags ##### They are needed because the loop needs to continue and cant wait for long tasks!
int rstNeed=0;   // Restart needed to apply new settings
int toPub=0; // determine if state should be published.
int eepromToClear=0; // determine if EEPROM should be cleared.

//##### Global vars ##### 
int iotMode=0; //IOT mode: 0 = Web control, 1 = MQTT (No const since it can change during runtime)
int webtypeGlob; // Are we in normal(0) or AP web(1)
int current; //Current state of the button
unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
char buf[40]; //For MQTT data recieve
int setupTimeOut=0; //Counter to reset within setup mode after some time. usefull after power break and WiFi not ready.

//To be read from EEPROM Config
String esid;
String epass = "";
String pubTopic;
String subTopic;
String mqttServer = "";

  
//-------------- void's -------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(10);
  // prepare GPIO2
  pinMode(outPin, OUTPUT);
  pinMode(inPin, INPUT_PULLUP);
  btn_timer.attach(0.05, btn_handle);
  loadConfig();
  if(debug==1) Serial.println("DEBUG: loadConfig() passed");
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  FQDN += "-";
  FQDN += macToStr(mac);
  
  // Connect to WiFi network
  initWiFi();
  if(debug==1) Serial.println("DEBUG: initWiFi() passed");
  if(debug==1) Serial.println("DEBUG: Starting the main loop");
}



void launchWeb(int webtype) {
    Serial.println("");
    Serial.println("WiFi connected");    
    //Start the web server or MQTT
     if (webtype==1 || iotMode==0){ //in config mode or WebControle
        if (webtype==1) {           
          webtypeGlob == 1;
          Serial.println(WiFi.softAPIP());
          server.on("/", webHandleConfig);
          server.on("/a", webHandleConfigSave);          
        } else {
          //setup DNS since we are a client in WiFi net
          if (!MDNS.begin((char*) FQDN.c_str())) {
            Serial.println("Error setting up MDNS responder!");
            while(1) { 
              delay(1000);
            }
          } else {
            Serial.println("MDNS responder started:" + FQDN);
          }          
          Serial.println(WiFi.localIP());
          server.on("/", webHandleRoot);  
          server.on("/cleareeprom", webHandleClearRom);
          server.on("/gpio", webHandleGpio);
        }
        //server.onNotFound(webHandleRoot);
        server.begin();
        Serial.println("Web server started");   
        webtypeGlob=webtype; //Store global to use in loop()
      } else if(webtype!=1 && iotMode==1){ // in MQTT and not in config mode     
        mqttClient.setBrokerDomain((char*)mqttServer.c_str());
        mqttClient.setPort(1883);
        mqttClient.setCallback(mqtt_arrived);
        mqttClient.setClient(wifiClient);
        if (WiFi.status() == WL_CONNECTED){
          if (!connectMQTT()){
              delay(2000);
              if (!connectMQTT()){                            
                Serial.println("Could not connect MQTT.");
                Serial.println("Starting web server instead.");
                iotMode=0;
                launchWeb(0);
                webtypeGlob=webtype;
              }
            }                    
          }
    }
}


void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0){
    Serial.println("no networks found");
    st ="<b>No networks found:</b>";
  } else {
    Serial.print(n);
    Serial.println(" Networks found");
    st = "<ul>";
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" (OPEN)":"*");
      
      // Print to web SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" (OPEN)":"*";
      st += "</li>";
      delay(10);
     }
    st += "</ul>";
  }
  Serial.println(""); 
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  //Build SSID
  //uint8_t mac[6];
  //WiFi.macAddress(mac);
  //ssid += "-";
  //ssid += macToStr(mac);
  
  WiFi.softAP((char*) ssid.c_str());
  WiFi.begin((char*) ssid.c_str());
  Serial.print("Access point started with name ");
  Serial.println(ssid);
  setupTimeOut_timer.attach(1,setupTimeOutCount);
  launchWeb(1);
}

void setupTimeOutCount(){
  if(setupTimeOut>=setupTimeOutLimit){
    system_restart();
  } else {
    setupTimeOut++;
  }
}
void btn_handle()
{
  if(!digitalRead(inPin)){
    ++count; // one count is 50ms
  } else {
    if (count > 1 && count < humanpressDelay/5) { //push between 50 ms and 1 sec      
      Serial.print("button pressed "); 
      Serial.print(count*0.05); 
      Serial.println(" Sec."); 
    
      Serial.print("Light is ");
      Serial.println(digitalRead(outPin));
      
      Serial.print("Switching light to "); 
      Serial.println(!digitalRead(outPin));
      digitalWrite(outPin, !digitalRead(outPin)); 
      if(iotMode==1 && mqttClient.connected()) toPub=1;
    } else if (count > (restartDelay/0.05) && count <= (resetDelay/0.05)){ //pressed 3 secs (60*0.05s)
      Serial.print("button pressed "); 
      Serial.print(count*0.05); 
      Serial.println(" Sec. Restarting!"); 
      system_restart();
    } else if (count > (resetDelay/0.05)){ //pressed 20 secs
      Serial.print("button pressed "); 
      Serial.print(count*0.05); 
      Serial.println(" Sec."); 
      Serial.println(" Clearing EEPROM and resetting!");       
      eepromToClear=1;
      }
    count=0; //reset since we are at high
  }
}

//-------------------------------- Help functions ---------------------------

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
//-------------------------------- Main loop ---------------------------
void loop() {
  if(debug==1) Serial.println("DEBUG: loop() begin");
  if(eepromToClear==1){
    if(debug==1) Serial.println("DEBUG: loop() clear EEPROM flag set!");
    clearEEPROM();
    delay(1000);
    system_restart();
  }
  if(debug==1) Serial.println("DEBUG: eeprom reset check passed");  
  if (WiFi.status() == WL_CONNECTED || webtypeGlob == 1){
    if(debug==1) Serial.println("DEBUG: loop() wifi connected & webServer ");
    if (iotMode==0 || webtypeGlob == 1){
      if(debug==1) Serial.println("DEBUG: loop() Web mode requesthandling ");
      server.handleClient();
      //mdns.update(); we get problems with this.
      delay(10);
    } else if (iotMode==1 && webtypeGlob != 1){
          if(debug==1) Serial.println("DEBUG: loop() MQTT mode requesthandling ");
          if (!connectMQTT()){
              delay(200);
          }                    
          if (mqttClient.connected()){
              mqtt_handler();
          }
    }
  } else{
    if(debug==1) Serial.println("DEBUG: loop - WiFi not connected");  
    delay(1000);
    initWiFi(); //Try to connect again
  }
    if(debug==1) Serial.println("DEBUG: loop() end");
}