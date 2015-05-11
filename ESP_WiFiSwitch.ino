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
 *  printed to Serial when the module is connected.
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
String FQDN ="WiFiSwitch.local"; //The DNS hostname - Does not work yet?
int iotMode=0; //IOT mode: 0 = Web control, 1 = MQTT (No const since it can change during runtime)
//select GPIO's
const int outPin = 13; //output pin
const int inPin = 0;  // input pin (push button)

const int restartDelay = 3; //minimal time for button press to reset in sec
const int humanpressDelay = 50; // the delay in ms untill the press should be handled as a normal push by human. Button debouce. !!! Needs to be less than restartDelay & resetDelay!!!
const int resetDelay = 20; //Minimal time for button press to reset all settings and boot to config mode in sec

//##### Object instances ##### 
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient;
Ticker btn_timer;
Ticker iot_loop;


//##### Flags ##### They are needed because the loop needs to continue and cant wait for long tasks!
int rstNeed=0;   // Restart needed to apply settings change?
int toPub=0; // determine if state should be published.
int eepromToClear=0; // determine if EEPROM should be cleared.

//##### Global vars ##### 
int webtypeGlob;
int current; //Current state of the button
unsigned long count = 0; //Button press time counter
String st; //WiFi Stations HTML list
char buf[40]; //FOr MQTT data recieve

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
  // Connect to WiFi network
  initWiFi();
}

void loadConfig(){
  EEPROM.begin(512);
  delay(10);
  String temp="";
  
  //len: 32
  //addr += EEPROM.get(addr, esid);
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  
  //len: 64+32=96
  //addr += EEPROM.get(addr, epass);
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  
  
  //len: 1+96=97
  String eiot = "";
  //addr += EEPROM.get(addr, iotMode);
  for (int i = 96; i < 97; ++i)
    {
      eiot += char(EEPROM.read(i));
    }
  Serial.print("IOT Mode: ");
  if(eiot=="1"){
    iotMode=1;
  }else{
    iotMode=0;
  }
  Serial.println(iotMode); 
    
  //len: 64+97=161
  //addr += EEPROM.get(addr, subTopic);
  for (int i = 97; i < 161; ++i)
    {
      subTopic += char(EEPROM.read(i));
    }
  Serial.print("MQTT subscribe topic: ");
  Serial.println(subTopic); 
  
  //len: 64+161=225
  //addr += EEPROM.get(addr, pubTopic);
  for (int i = 161; i < 225; ++i)
    {
      pubTopic += char(EEPROM.read(i));
    }
  Serial.print("MQTT publish topic: ");
  Serial.println(pubTopic); 
  
  //len: 15+225=240
  //addr += EEPROM.get(addr, mqttServer);
  for (int i = 225; i < 240; ++i)
    {
      temp=EEPROM.read(i);
      if(temp!="0") mqttServer += char(EEPROM.read(i)); // Ignore spaces
    }
  Serial.print("MQTT Broker IP: ");
  Serial.println(mqttServer);     
  EEPROM.end();
}

void initWiFi(){
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  esid.trim();
  if ( esid.length() > 1 ) {
      // test esid 
      WiFi.disconnect();
      delay(100);
      WiFi.mode(WIFI_STA);
      Serial.print("Connecting to WiFi ");
      Serial.println(esid);
      WiFi.begin(esid.c_str(), epass.c_str());
      if ( testWifi() == 20 ) { 
          launchWeb(0);
          return;
      }
  }
  setupAP();   
}

int testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");  
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED) { return(20); } 
    delay(500);
    Serial.print(".");    
    c++;
  }
  Serial.println("Connect timed out, opening AP");
  return(10);
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
          if (!mdns.begin((char*) FQDN.c_str(), WiFi.localIP())) {
            Serial.println("Error setting up MDNS responder!");
            while(1) { 
              delay(1000);
            }
          } else {
            Serial.println("mDNS responder started");
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

void webHandleConfig(){
  IPAddress ip = WiFi.softAPIP();
  String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
  String s;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += "-";
  clientName += macToStr(mac);
  
  s = "Configuration of " + clientName + " at ";
  s += ipStr;
  s += "<p>";
  s += st;
  s += "<form method='get' action='a'>";
  s += "<label>SSID: </label><input name='ssid' length=32><label> Pass: </label><input name='pass' type='password' length=64></br>";
  s += "The following is not ready yet!</br>";
  s += "<label>IOT mode: </label><input type='radio' name='iot' value='0'> HTTP<input type='radio' name='iot' value='1' checked> MQTT</br>";
  s += "<label>MQTT Broker IP/DNS: </label><input name='host' length=15></br>";
  s += "<label>MQTT Publish topic: </label><input name='pubtop' length=64></br>";
  s += "<label>MQTT Subscribe topic: </label><input name='subtop' length=64></br>";
  s += "<input type='submit'></form></p>";
  s += "\r\n\r\n";
  Serial.println("Sending 200");  
  server.send(200, "text/html", s); 
}

void webHandleConfigSave(){
  // /a?ssid=blahhhh&pass=poooo
  String s;
  s = "<p>Settings saved to eeprom and reset to boot into new settings</p>\r\n\r\n";
  server.send(200, "text/html", s); 
  Serial.println("clearing EEPROM.");
  clearEEPROM();
  String qsid; 
  qsid = server.arg("ssid");
  qsid.replace("%2F","/");
  Serial.println(qsid);
  Serial.println("");

  String qpass;
  qpass = server.arg("pass");
  qpass.replace("%2F","/");
  Serial.println(qpass);
  Serial.println("");

  String qiot;
  qiot = server.arg("iot");
  Serial.println(qiot);
  Serial.println("");
  
  String qsubTop;
  qsubTop = server.arg("subtop");
  qsubTop.replace("%2F","/");
  Serial.println(qsubTop);
  Serial.println("");
  
  String qpubTop;
  qpubTop = server.arg("pubtop");
  qpubTop.replace("%2F","/");
  Serial.println(qpubTop);
  Serial.println("");
  
  String qmqttip;
  qmqttip = server.arg("host");
  Serial.println(qmqttip);
  Serial.println("");
  
  //int addr=0;
  EEPROM.begin(512);
  delay(10);
  Serial.println("writing eeprom ssid.");
  //addr += EEPROM.put(addr, qsid);
  for (int i = 0; i < qsid.length(); ++i)
    {
      EEPROM.write(i, qsid[i]);
      Serial.print(qsid[i]); 
    }
  Serial.println("");
    
  Serial.println("writing eeprom pass."); 
  //addr += EEPROM.put(addr, qpass);
  for (int i = 0; i < qpass.length(); ++i)
    {
      EEPROM.write(32+i, qpass[i]);
      Serial.print(qpass[i]); 
    }  
  Serial.println("");
    
  Serial.println("writing eeprom iot."); 
  //addr += EEPROM.put(addr, qiot);
  for (int i = 0; i < qiot.length(); ++i)
    {
      EEPROM.write(96+i, qiot[i]);
      Serial.print(qiot[i]); 
    } 
  Serial.println("");
    
  Serial.println("writing eeprom subTop."); 
  //addr += EEPROM.put(addr, qsubTop);
  for (int i = 0; i < qsubTop.length(); ++i)
    {
      EEPROM.write(97+i, qsubTop[i]);
      Serial.print(qsubTop[i]); 
    } 
  Serial.println("");
    
  Serial.println("writing eeprom pubTop."); 
  //addr += EEPROM.put(addr, qpubTop);
  for (int i = 0; i < qpubTop.length(); ++i)
    {
      EEPROM.write(161+i, qpubTop[i]);
      Serial.print(qpubTop[i]); 
    } 
  Serial.println("");
    
  Serial.println("writing eeprom MQTT IP."); 
  //addr += EEPROM.put(addr, qmqttip);
  for (int i = 0; i < qmqttip.length(); ++i)
    {
      EEPROM.write(225+i, qmqttip[i]);
      Serial.print(qmqttip[i]); 
    } 
  Serial.println("");  
  
  EEPROM.commit();
  delay(1000);
  EEPROM.end();
  Serial.println("Settings written, restarting!"); 
  system_restart();
}

void webHandleRoot(){
  String s;
  s = "<p>Hello from ESP8266";
  s += "</p>";
  s += "\r\n\r\n";
  Serial.println("Sending 200");  
  server.send(200, "text/html", s); 
}

void webHandleClearRom(){
  String s;
  s = "<p>Clearing the EEPROM and reset to configure new wifi<p>";
  s += "</html>\r\n\r\n";
  Serial.println("Sending 200"); 
  server.send(200, "text/html", s); 
  Serial.println("clearing eeprom");
  clearEEPROM();
  delay(10);
  Serial.println("Done, restarting!");
  system_restart();
}

void webHandleGpio(){
  String s;
   // Set GPIO according to the request
    if (server.arg("state")=="1" || server.arg("state")=="0" ) {
      int state = server.arg("state").toInt();
      digitalWrite(outPin, state);
    }
    s = "Light is now ";
    s += (digitalRead(outPin))?"on":"off";
    s += "<p>Change to <form action='gpio'><input type='radio' name='state' value='1' ";
    s += (digitalRead(outPin))?"checked":"";
    s += ">On<input type='radio' name='state' value='0' ";
    s += (digitalRead(outPin))?"":"checked";
    s += ">Off <input type='submit' value='Submit'></form></p>";   
    server.send(200, "text/html", s);    
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
  WiFi.begin((char*) ssid.c_str()); // not sure if need but works
  Serial.print("Access point started with name ");
  Serial.println(ssid);
  launchWeb(1);
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
void clearEEPROM(){
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++){
    EEPROM.write(i, 0);    
  }
  delay(200);
  EEPROM.commit(); 
  EEPROM.end(); 
}

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

//-------------------------------- MQTT functions ---------------------------
boolean connectMQTT(){
  if (mqttClient.connected()){
    return true;
  }  
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += "-";
  clientName += macToStr(mac);
  
  Serial.print("Connecting to MQTT server ");
  Serial.print(mqttServer);
  Serial.print(" as ");
  Serial.println(clientName);
  
  if (mqttClient.connect((char*) clientName.c_str())) {
    Serial.println("Connected to MQTT broker");
    if(mqttClient.subscribe((char*)subTopic.c_str())){
      Serial.println("Subsribed to topic.");
    } else {
      Serial.println("NOT subsribed to topic!");      
    }
    return true;
  }
  else {
    Serial.println("MQTT connect failed! ");
    return false;
  }
}

void disconnectMQTT(){
  mqttClient.disconnect();
}

void mqtt_handler(){
  if (toPub==1){
    if(pubState()){
     toPub=0; 
    }
  }
  mqttClient.loop();
  delay(100); //let things happen in background
}

void mqtt_arrived(char* subTopic, byte* payload, unsigned int length) { // handle messages arrived 
  int i = 0;
  Serial.print("MQTT message arrived:  topic: " + String(subTopic));
    // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {    
    buf[i] = payload[i];
  }
  buf[i] = '\0';
  String msgString = String(buf);
  Serial.println(" message: " + msgString);
  if (msgString == "1"){
      Serial.print("Light is ");
      Serial.println(digitalRead(outPin));      
      Serial.print("Switching light to "); 
      Serial.println("high");
      digitalWrite(outPin, 1); 
  } else if (msgString == "0"){
      Serial.print("Light is ");
      Serial.println(digitalRead(outPin));    
      Serial.print("Switching light to "); 
      Serial.println("low");
      digitalWrite(outPin, 0); 
  }    
}

boolean pubState(){ //Publish the current state of the light    
  if (!connectMQTT()){
      delay(100);
      if (!connectMQTT){                            
        Serial.println("Could not connect MQTT.");
        Serial.println("Publish state NOK");
        return false;
      }
    }
    if (mqttClient.connected()){      
      String state = (digitalRead(outPin))?"1":"0";
        Serial.println("To publish state " + state );  
      if (mqttClient.publish((char*) pubTopic.c_str(), (char*) state.c_str())) {
        Serial.println("Publish state OK");        
        return true;
      } else {
        Serial.println("Publish state NOK");        
        return false;
      }
     } else {
         Serial.println("Publish state NOK");
         Serial.println("No MQTT connection.");        
     }    
}
//-------------------------------- Main loop ---------------------------
void loop() {
  if(eepromToClear==1){
    clearEEPROM();
    delay(1000);
    system_restart();
  }
  
  if (WiFi.status() == WL_CONNECTED || webtypeGlob == 1){
    if (iotMode==0 || webtypeGlob == 1){
      server.handleClient();
      mdns.update();
      //Serial.print("*");
      delay(10);
    } else if (iotMode==1 && webtypeGlob != 1){
          if (!connectMQTT()){
              delay(200);
          }                    
          if (mqttClient.connected()){
              mqtt_handler();
          }
    }
  } else{
    delay(1000);
    initWiFi();
  }
}
