
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
  s += "<a href=\"/gpio\">Controle GPIO</a><br />";
  s += "<a href=\"/cleareeprom\">Clear settings an boot into Config mode</a><br />";
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
      Serial.print("Light switched via web request to  ");      
      Serial.println(state);      
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
