bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }

  int otaFlagC = json["otaFlag"];
  String esidC = json["esid"];
  String epassC = json["epass"];
  int iotModeC = json["iotMode"];
  String pubTopicC = json["pubTopic"];
  String subTopicC = json["subTopic"];
  String mqttServerC = json["mqttServer"];

  // Real world application would store these values in some variables for
  // later use.
  otaFlag = otaFlagC;
  esid = esidC;
  epass = epassC;
  iotMode = iotModeC;
  pubTopic = pubTopicC;
  subTopic = subTopicC;
  mqttServer = mqttServerC;
  Serial.print("otaFlag: "); 
  Serial.println(otaFlag);
  Serial.print("esid: ");
  Serial.println(esid);
  Serial.print("epass: ");
  Serial.println(epass);
  Serial.print("iotMode: ");
  Serial.println(iotMode);
  Serial.print("pubTopic: ");
  Serial.println(pubTopic);
  Serial.print("subTopic: ");
  Serial.println(subTopic);
  Serial.print("mqttServer: ");
  Serial.println(mqttServer);
  Serial.print("esid: ");
  Serial.println(esid);
  return true;
}

bool saveConfig() {
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["otaFlag"] = otaFlag;
  json["esid"] = esid;
  json["epass"] = epass;
  json["iotMode"] = iotMode;
  json["pubTopic"] = pubTopic;
  json["subTopic"] = subTopic;
  json["mqttServer"] = mqttServer;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}


void setOtaFlag(int intOta){
  otaFlag=intOta;
  saveConfig();
  yield();
}

bool clearConfig(){
    Debugln("DEBUG: In config clear!");
    return SPIFFS.format();  
}

