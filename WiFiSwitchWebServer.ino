/*
 *  This sketch is running a web server for configuring WiFI if can't connect or for controlling of one GPIO to switch a light/LED
 *  While a WiFi config is not set or can't connect:
 *    http://server_ip will give a config page with 
 *  While a WiFi config is set:
 *    http://server_ip/gpio -> Will display the GIPIO state and a switch form for it
 *    http://server_ip/gpio?state=0 -> Will change the GPIO directly and display the above aswell
 *    http://server_ip/cleareeprom -> Will reset the WiFi setting and rest to configure mode as AP
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 *  For several snippets used the credit goes to:
 *  - https://github.com/esp8266
 *  - https://github.com/chriscook8/esp-arduino-apboot
 *  - And the whole Arduino and ESP8266 comunity
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <EEPROM.h>

extern "C" {
  #include "user_interface.h" //Needed for the reset command
}
const char* ssid = "WiFiSwitch";
const char* hostname ="wifiswitch1";

MDNSResponder mdns;
// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);
//select GPIO's
const int outPin = 13; //out pin
const int inPin = 0;  // the number of the input pin (push button)
int current; //Current state of the button
int rstNeed=0;   // Restart needed to to setup change?
byte previous = HIGH;
unsigned long firstTime =0;   // time when the button was first pressed 
unsigned long secondTime =0;   // time when the button was first released 
String st;

void setup() {
  Serial.begin(115200);
  delay(10);
  // prepare GPIO2
  pinMode(outPin, OUTPUT);
  pinMode(inPin, INPUT_PULLUP);
  attachInterrupt(inPin, btn_handle, CHANGE);
               
  
  // Connect to WiFi network
  initWiFi();
}

void initWiFi(){
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
    {
      esid += char(EEPROM.read(i));
    }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
    {
      epass += char(EEPROM.read(i));
    }
  Serial.print("PASS: ");
  Serial.println(epass);  
  if ( esid.length() > 1 ) {
      // test esid 
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
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.softAPIP());
    if (!mdns.begin(hostname, WiFi.localIP())) {
      Serial.println("Error setting up MDNS responder!");
      while(1) { 
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
    // Start the server
    server.begin();
    Serial.println("Server started");   
    int b = 20;
    int c = 0;
    while(b == 20) { 
       b = mdns1(webtype);
     }
}

int mdns1(int webtype)
{
  // Check for any mDNS queries and send responses
  mdns.update();
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return(20);
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
   }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return(20);
   }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush(); 
  String s;
  if ( webtype == 1 ) { //Setup mode as Wifi Server
      if (req == "/")
      {
        IPAddress ip = WiFi.softAPIP();
        String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
        s += ipStr;
        s += "<p>";
        s += st;
        s += "<form method='get' action='a'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/a?ssid=") ) {
        // /a?ssid=blahhhh&pass=poooo
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        String qsid; 
        qsid = req.substring(8,req.indexOf('&'));
        Serial.println(qsid);
        Serial.println("");
        String qpass;
        qpass = req.substring(req.lastIndexOf('=')+1);
        Serial.println(qpass);
        Serial.println("");
        
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
          {
            EEPROM.write(i, qsid[i]);
            Serial.print("Wrote: ");
            Serial.println(qsid[i]); 
          }
        Serial.println("writing eeprom pass:"); 
        for (int i = 0; i < qpass.length(); ++i)
          {
            EEPROM.write(32+i, qpass[i]);
            Serial.print("Wrote: ");
            Serial.println(qpass[i]); 
          }    
        EEPROM.commit();
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 ";
        s += "Found ";
        s += req;
        s += "<p> saved to eeprom and reset to boot into new wifi</html>\r\n\r\n";
        rstNeed=1;
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }
  }
  else
  { //Normal operating as Wifi client
      if (req == "/")
      {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");
      }
      else if ( req.startsWith("/cleareeprom") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266";
        s += "<p>Clearing the EEPROM and reset to configure new wifi<p>";
        s += "</html>\r\n\r\n";
        Serial.println("Sending 200");  
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
        EEPROM.commit();
        
        rstNeed=1;
      }  
      else if ( req.startsWith("/gpio?state=0") ) {
        // Set GPIO2 according to the request
        digitalWrite(outPin, 0);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLight is now ";
        s += (digitalRead(outPin))?"high":"low";
        s += "<p>Change to <form action='gpio'><input type='radio' name='state' value='1'>On<input type='radio' name='state' value='0' checked>Off <input type='submit' value='Submit'></form></p>";
        s += "</html>\n";
      }
      else if ( req.startsWith("/gpio?state=1") ) {
        // Set GPIO2 according to the request
        digitalWrite(outPin, 1);
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLight is now ";
        s += (digitalRead(outPin))?"on":"off";
        s += "<p>Change to <form action='gpio'><input type='radio' name='state' value='1' checked>On<input type='radio' name='state' value='0'>Off <input type='submit' value='Submit'></form></p>";
        s += "</html>\n";                
      }
      else if ( req.startsWith("/gpio") ) {
        s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nLight is now ";
        s += (digitalRead(outPin))?"on":"off";
        s += "<p>Change to <form action='gpio'><input type='radio' name='state' value='1' checked>On<input type='radio' name='state' value='0'>Off <input type='submit' value='Submit'></form></p>";
        s += "</html>\n";                
      }
      else
      {
        s = "HTTP/1.1 404 Not Found\r\n\r\n";
        Serial.println("Sending 404");
      }       
  }
  client.print(s);
  Serial.println("Done with client");
  if (rstNeed)    
    system_restart();
  return(20);
}

void setupAP(void) {
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
     {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
     }
  }
  Serial.println(""); 
  st = "<ul>";
  for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st +=i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
      st += "</li>";
    }
  st += "</ul>";
  delay(100);
  WiFi.softAP(ssid);
  Serial.println("softap");
  Serial.println("");
  launchWeb(1);
  Serial.println("over");
}

void loop() {
 
}

void btn_handle()
{
  
  current = digitalRead(inPin);
  if (current == LOW && previous == HIGH){  // if the buttons becomes press remember the time 
    firstTime = millis();    
    Serial.print("firstTime set to "); 
    Serial.println(firstTime); 
  }

  if (current == HIGH && previous == LOW){  // if the buttons becomes released remember the time 
    secondTime = millis();        
    Serial.print("secondTime set to "); 
    Serial.println(secondTime); 
  }

  if (current == HIGH && previous == LOW && secondTime - firstTime >=50 && secondTime - firstTime <=1000){ // when the button changed between 50 ms and 1 sec
    Serial.print("Light is ");
    Serial.println(digitalRead(outPin));
    
    Serial.print("Switching light to "); 
    Serial.println(!digitalRead(outPin));
    digitalWrite(outPin, !digitalRead(outPin));     
  }
  Serial.print("button pressed "); 
  Serial.print(secondTime - firstTime); 
  Serial.println(" mSec"); 
  if (current == HIGH && previous == LOW && secondTime - firstTime >=3000 ){ // when the button changed after more than 3 sec
      Serial.print("button pressed "); 
      Serial.print(secondTime - firstTime); 
      Serial.println(" mSec"); 
      //ESP.reset(); 
      system_restart();
  }
  
  previous = current;
}
