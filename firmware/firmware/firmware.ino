#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <SPI.h>
#include <ESPmDNS.h>
#include <WiFiClient.h> 
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiAP.h>
#include <LittleFS.h>
#include "SD.h"

// Config SSID, password and hostname
String defaultSSID = "Evil Crow RF v2";  // Enter your SSID here
String defaultPassword = "123456789ECRFv2";  // Enter your Password here
String hostname = "evilcrow-rf"; // Enter your hostname here

// MicroSD slot pins
#define SD_SCLK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_SS   22
SPIClass sdspi(VSPI);

// SPI Pins
int sck_pin = 14;
int miso_pin = 12;
int mosi_pin = 13;

// CC1101 Module 1
int rx_pin1 = 4;
int tx_pin1 = 2;
int cs_pin1 = 5;

// CC1101 Module 2
int rx_pin2 = 26;
int tx_pin2 = 25;
int cs_pin2 = 27;

// RF variables
#define RECEIVE_ATTR IRAM_ATTR
#define samplesize 2000
int error_toleranz = 200;
const int minsample = 30;
unsigned long sample[samplesize];
unsigned long samplesmooth[samplesize];
String lastSampleSmooth;
int  lastIndex;
int samplecount;
static unsigned long lastTime = 0;
int mod;
float deviation;
int datarate;
float frequency;
float setrxbw;
int power_jammer;
byte jammer[11] = {0xff,0xff,};
long data_to_send[2000];

// Other variables
const bool formatOnFail = true;
String OutputLog;

// File
File logs;

// Web Server
String tmp_module;
String tmp_frequency;
String tmp_setrxbw;
String tmp_mod;
String tmp_deviation;
String tmp_datarate;
String raw_rx = "0";
String jammer_tx = "0";
String transmit;
AsyncWebServer controlserver(80);

void connectToWiFi() {
  String wifiSSID = defaultSSID;
  String wifiPassword = defaultPassword;

  if (readWiFiConfig(wifiSSID, wifiPassword)) {
    //Serial.println("Using saved Wi-Fi credentials.");
  } else {
    //Serial.println("Using default Wi-Fi credentials.");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  //Serial.print("Connecting to Wi-Fi: "); Serial.println(wifiSSID);
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    if (millis() - start > 15000) { 
      //Serial.println("\nFailed to connect to Wi-Fi.");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    //Serial.print("\nConnected! IP: "); Serial.println(WiFi.localIP());
  }
}

bool readWiFiConfig(String &ssid, String &password) {
  if (!SD.exists("/wifi_config.txt")) {
    //Serial.println("Wi-Fi config file not found");
    return false;
  }

  File file = SD.open("/wifi_config.txt", FILE_READ);
  if (!file) {
    //Serial.println("Failed to open Wi-Fi config file");
    return false;
  }

  ssid = file.readStringUntil('\n');
  ssid.trim();

  password = file.readStringUntil('\n');
  password.trim();

  file.close();

  if (ssid.length() == 0 || password.length() == 0) {
    //Serial.println("Invalid Wi-Fi config (empty fields)");
    return false;
  }

  //Serial.println("Wi-Fi config read successfully:");
  //Serial.print("SSID: "); Serial.println(ssid);
  //Serial.print("Password: "); Serial.println(password);
  return true;
}

void handleUpdateWiFi(AsyncWebServerRequest *request) {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String newSSID = request->getParam("ssid", true)->value();
    String newPassword = request->getParam("password", true)->value();

    if (SD.exists("/wifi_config.txt")) {
      SD.remove("/wifi_config.txt");
      //Serial.println("Existing Wi-Fi config file deleted.");
    }

    appendFile(SD, "/wifi_config.txt", newSSID.c_str(), "\n");
    appendFile(SD, "/wifi_config.txt", newPassword.c_str(), "\n");
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Wi-Fi config applied successfully! Device will restart.\"}");
    delay(500);
    ESP.restart();
  } else {
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing SSID or password\"}");
  }
}

void handleDeleteWiFiConfig(AsyncWebServerRequest *request) {
  if (SD.exists("/wifi_config.txt")) {
    if (SD.remove("/wifi_config.txt")) {
      request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Wi-Fi config deleted successfully\"}");
      ESP.restart();
    } else {
      request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Failed to delete the file\"}");
    }
  } else {
    request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Wi-Fi config file not found\"}");
  }
}

void handleStats(AsyncWebServerRequest *request) {
  uint64_t cardSize = 0;
  uint64_t usedBytes = 0;
  uint64_t freeBytes = 0;
  bool sd_present = SD.cardType() != CARD_NONE;

  if (sd_present) {
    cardSize = SD.cardSize();
    freeBytes = (cardSize > usedBytes) ? (cardSize - usedBytes) : 0;
  }

  size_t freeSpiffs = 0;
  if (LittleFS.begin(true)) {
    freeSpiffs = LittleFS.totalBytes() - LittleFS.usedBytes();
  }

  float cardSizeGB = cardSize / (1024.0 * 1024.0 * 1024.0);
  float usedGB = usedBytes / (1024.0 * 1024.0 * 1024.0);
  float freeGB = freeBytes / (1024.0 * 1024.0 * 1024.0);

  String json = "{";
  json += "\"uptime\":" + String(millis() / 1000);
  json += ",\"cpu0\":" + String(getCpuFrequencyMhz());
  json += ",\"cpu1\":" + String(getXtalFrequencyMhz());
  json += ",\"temperature\":" + String(temperatureRead());
  json += ",\"freespiffs\":" + String(freeSpiffs);
  json += ",\"sdcard_size_gb\":" + String(cardSizeGB, 2);
  json += ",\"sdcard_free_gb\":" + String(freeGB, 2);
  json += ",\"sdcard_present\":" + String(sd_present ? "true" : "false");
  json += ",\"totalram\":" + String(ESP.getHeapSize());
  json += ",\"freeram\":" + String(ESP.getFreeHeap());
  json += ",\"ssid\":\"" + WiFi.SSID() + "\"";
  json += ",\"ipaddress\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";

  request->send(200, "application/json", json);
}

void appendFile(fs::FS &fs, const char * path, const char * message, String messagestring){
  logs = fs.open(path, FILE_APPEND);
  if(!logs){
    //Serial.println("Failed to open file for appending");
    return;
  }
  if(logs.print(message)|logs.print(messagestring)){
    //Serial.println("Message appended");
  } else {
    //Serial.println("Append failed");
  }
  logs.close();
}

void deleteFile(fs::FS &fs, const char * path){
  //Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    //Serial.println("File deleted");
  } else {
    //Serial.println("Delete failed");
  }
}

bool checkReceived(void) {
  delay(1);
  if (samplecount >= minsample && micros() - lastTime > 100000) {
    detachInterrupt(rx_pin1);
    detachInterrupt(rx_pin2);
    return true;
  } else {
    return false;
  }
}

void printReceived() {
  OutputLog = "";
  appendFile(SD, "/logs.txt", "-------------------------------------------------------\n", "");
  //Serial.print("Count=");
  //Serial.println(samplecount);
  OutputLog += "\n";
  OutputLog += "Count=";
  OutputLog += String(samplecount);
  OutputLog += "\n";

  for (int i = 0; i < samplecount; i++) {
    //Serial.print(sample[i]);
    //Serial.print(",");
    OutputLog += String(sample[i]);
    OutputLog += ",";
  }
  //Serial.println();
  //Serial.println();
  OutputLog += "\n";
  appendFile(SD, "/logs.txt", NULL, OutputLog.c_str());
}

void RECEIVE_ATTR receiver() {
  const long time = micros();
  const unsigned int duration = time - lastTime;

  if (duration > 100000) {
    samplecount = 0;
  }

  if (duration >= 100 && samplecount < samplesize) {
    sample[samplecount++] = duration;
  }

  /*if (duration >= 100) {
    sample[samplecount++] = duration;
  }*/

  if (samplecount >= samplesize) {
    samplecount = samplesize - 1;
    bool received = checkReceived();
    if (received) {}
    detachInterrupt(rx_pin1);
    detachInterrupt(rx_pin2);
  }

  if (mod == 0) {
    if (samplecount == 1 and digitalRead(rx_pin2) != HIGH){
      samplecount = 0;
    }

    else if (samplecount == 1 and digitalRead(rx_pin1) != HIGH){
      samplecount = 0;
    }
  }

  lastTime = time;
}

void signalanalyse(){
  OutputLog = "";
  #define signalstorage 10

  int signalanz=0;
  int timingdelay[signalstorage];
  float pulse[signalstorage];
  long signaltimings[signalstorage*2];
  int signaltimingscount[signalstorage];
  long signaltimingssum[signalstorage];
  long signalsum=0;
  
  for (int i = 0; i<signalstorage; i++){
    signaltimings[i*2] = 100000;
    signaltimings[i*2+1] = 0;
    signaltimingscount[i] = 0;
    signaltimingssum[i] = 0;
  }
  for (int i = 1; i<samplecount; i++){
    signalsum+=sample[i];
  }

  for (int p = 0; p<signalstorage; p++){

  for (int i = 1; i<samplecount; i++){
    if (p==0){
      if (sample[i]<signaltimings[p*2]){
        signaltimings[p*2]=sample[i];
      }
    }else{
      if (sample[i]<signaltimings[p*2] && sample[i]>signaltimings[p*2-1]){
        signaltimings[p*2]=sample[i];
      }
    }
  }

  for (int i = 1; i<samplecount; i++){
    if (sample[i]<signaltimings[p*2]+error_toleranz && sample[i]>signaltimings[p*2+1]){
      signaltimings[p*2+1]=sample[i];
    }
  }

  for (int i = 1; i<samplecount; i++){
    if (sample[i]>=signaltimings[p*2] && sample[i]<=signaltimings[p*2+1]){
      signaltimingscount[p]++;
      signaltimingssum[p]+=sample[i];
    }
  }
  }
  int firstsample = signaltimings[0];
  signalanz=signalstorage;

  for (int i = 0; i<signalstorage; i++){
    if (signaltimingscount[i] == 0){
      signalanz=i;
      i=signalstorage;
    }
  }

  for (int s=1; s<signalanz; s++){
  for (int i=0; i<signalanz-s; i++){
    if (signaltimingscount[i] < signaltimingscount[i+1]){
      int temp1 = signaltimings[i*2];
      int temp2 = signaltimings[i*2+1];
      int temp3 = signaltimingssum[i];
      int temp4 = signaltimingscount[i];
      signaltimings[i*2] = signaltimings[(i+1)*2];
      signaltimings[i*2+1] = signaltimings[(i+1)*2+1];
      signaltimingssum[i] = signaltimingssum[i+1];
      signaltimingscount[i] = signaltimingscount[i+1];
      signaltimings[(i+1)*2] = temp1;
      signaltimings[(i+1)*2+1] = temp2;
      signaltimingssum[i+1] = temp3;
      signaltimingscount[i+1] = temp4;
    }
  }
  }

  for (int i=0; i<signalanz; i++){
    timingdelay[i] = signaltimingssum[i]/signaltimingscount[i];
  }

  if (firstsample == sample[1] and firstsample < timingdelay[0]){
    sample[1] = timingdelay[0];
  }

  bool lastbin=0;
  for (int i=1; i<samplecount; i++){
    float r = (float)sample[i]/timingdelay[0];
    int calculate = r;
    r = r-calculate;
    r*=10;
    if (r>=5){calculate+=1;}
    if (calculate>0){
      if (lastbin==0){
        lastbin=1;
      }else{
      lastbin=0;
    }
      if (lastbin==0 && calculate>8){
        //Serial.print(" [Pause: ");
        //Serial.print(sample[i]);
        //Serial.println(" samples]");
        OutputLog += " [Pause: ";
        OutputLog += String(sample[i]);
        OutputLog += " samples]\n";

      } else{
        for (int b=0; b<calculate; b++){
          //Serial.print(lastbin);
          OutputLog += String(lastbin);
        }
      }
    }
  }
  //Serial.println();
  //Serial.print("Samples/Symbol: ");
  //Serial.println(timingdelay[0]);
  //Serial.println();
  OutputLog += "\nSamples/Symbol: ";
  OutputLog += String(timingdelay[0]);
  OutputLog += "\n\n";

  int smoothcount=0;

  for (int i=1; i<samplecount; i++){
    float r = (float)sample[i]/timingdelay[0];
    int calculate = r;
    r = r-calculate;
    r*=10;
    if (r>=5){calculate+=1;}
    if (calculate>0){
      samplesmooth[smoothcount] = calculate*timingdelay[0];
      smoothcount++;
    }
  }
  appendFile(SD, "/logs.txt", NULL, "\n");
  //Serial.println("Rawdata corrected:");
  //Serial.print("Count=");
  //Serial.println(smoothcount+1);
  lastIndex = smoothcount - 1;
  lastSampleSmooth = samplesmooth[lastIndex];

  OutputLog += "Rawdata corrected:\n";
  OutputLog += "Count=";
  OutputLog += String(smoothcount + 1);
  OutputLog += "\n";

  for (int i=0; i<smoothcount; i++){
    //Serial.print(samplesmooth[i]);
    //Serial.print(",");
    OutputLog += String(samplesmooth[i]);
    OutputLog += ",";
  }
  //Serial.println();
  //Serial.println();
  appendFile(SD, "/logs.txt", NULL, OutputLog.c_str());
  appendFile(SD, "/logs.txt", NULL, "\n");
  appendFile(SD, "/logs.txt", "-------------------------------------------------------\n", "");
  return;
}

void enableReceive() {
  rx_pin1 = digitalPinToInterrupt(rx_pin1);
  rx_pin2 = digitalPinToInterrupt(rx_pin2);
  pinMode(rx_pin1, INPUT);
  pinMode(rx_pin2, INPUT);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(rx_pin1, receiver, CHANGE);
  attachInterrupt(rx_pin2, receiver, CHANGE);
}

void setup() {
  Serial.begin(38400);

  if (!LittleFS.begin(true)) {
    LittleFS.format();
    delay(1000);
    ESP.restart();
  }

  delay(2000);
  sdspi.begin(18, 19, 23, 22);
  SD.begin(22, sdspi);

  connectToWiFi();

  if (!MDNS.begin(hostname.c_str())) {
    //Serial.println("Error setting up MDNS responder!");
  }

  controlserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/HTML/index.html", "text/html");
  });

  controlserver.on("/rxconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/HTML/rxconfig.html", "text/html");
  });

  controlserver.on("/viewlog", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/HTML/viewlog.html", "text/html");
  });

  controlserver.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/logs.txt", "text/plain");
  });

  controlserver.on("/txconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/HTML/txconfig.html", "text/html");
  });

  controlserver.on("/delete", HTTP_POST, [](AsyncWebServerRequest *request){
    deleteFile(SD, "/logs.txt");
    request->send(200, "application/json", "{\"status\":\"deleted\"}");
  });

  controlserver.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/HTML/config.html", "text/html");
  });

  controlserver.on("/updatewifi", HTTP_POST, handleUpdateWiFi);
  controlserver.on("/deletewificonfig", HTTP_POST, handleDeleteWiFiConfig);
  controlserver.on("/stats", HTTP_GET, handleStats);

  controlserver.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "application/json", "{\"success\":true,\"message\":\"Device rebooting\"}");
    delay(200);
    ESP.restart();
  });

  controlserver.on("/connectioncheck", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"status\":\"ok\"}");
  });

  controlserver.on("/javascript.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/HTML/javascript.js", "text/javascript");
  });

  controlserver.on("/setrx", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!request->hasArg("module") || !request->hasArg("frequency") ||
      !request->hasArg("setrxbw") || !request->hasArg("mod") ||
      !request->hasArg("deviation") || !request->hasArg("datarate")) {

      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing parameters\"}");
      return;
    }

    tmp_module = request->arg("module");
    tmp_frequency = request->arg("frequency");
    tmp_setrxbw = request->arg("setrxbw");
    tmp_mod = request->arg("mod");
    tmp_deviation = request->arg("deviation");
    tmp_datarate = request->arg("datarate");

    if (request->hasArg("configmodule")) {
      frequency = tmp_frequency.toFloat();
      setrxbw = tmp_setrxbw.toFloat();
      mod = tmp_mod.toInt();
      deviation = tmp_deviation.toFloat();
      datarate = tmp_datarate.toInt();

      if (tmp_module == "1") {
        ELECHOUSE_cc1101.setModul(0);
        ELECHOUSE_cc1101.Init();
      } else if (tmp_module == "2") {
        ELECHOUSE_cc1101.setModul(1);
        ELECHOUSE_cc1101.Init();
      }

      if (mod == 2) {
        ELECHOUSE_cc1101.setDcFilterOff(0);
      } else if (mod == 0) {
        ELECHOUSE_cc1101.setDcFilterOff(1);
        ELECHOUSE_cc1101.setDeviation(deviation);
      }

      ELECHOUSE_cc1101.setModulation(mod);
      ELECHOUSE_cc1101.setMHZ(frequency);
      ELECHOUSE_cc1101.setSyncMode(0);
      ELECHOUSE_cc1101.setPktFormat(3);
      ELECHOUSE_cc1101.setRxBW(setrxbw);
      ELECHOUSE_cc1101.setDRate(datarate);
      enableReceive();
      raw_rx = "1";
      request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"RX configuration applied successfully.\"}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing configmodule parameter\"}");
    }
  });

  controlserver.on("/stoprx", HTTP_POST, [](AsyncWebServerRequest *request) {
    ELECHOUSE_cc1101.setModul(0);
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.setModul(1);
    ELECHOUSE_cc1101.setSidle();

    raw_rx = "0";
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"RX stopped.\"}");
  });

  controlserver.on("/settx", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasArg("module") || !request->hasArg("frequency") || 
      !request->hasArg("rawdata") || !request->hasArg("mod") || !request->hasArg("deviation")) {
      request->send(400, "text/plain", "Missing parameters");
      return;
    }

    tmp_module = request->arg("module");
    tmp_frequency = request->arg("frequency");
    transmit = request->arg("rawdata");
    tmp_deviation = request->arg("deviation");
    tmp_mod = request->arg("mod");

    int counter = 0;
    int pos = 0;
    frequency = tmp_frequency.toFloat();
    deviation = tmp_deviation.toFloat();
    mod = tmp_mod.toInt();

    for (int i = 0; i < transmit.length(); i++){
      if (transmit.substring(i, i+1) == ",") {
        data_to_send[counter++] = transmit.substring(pos, i).toInt();
        pos = i+1;
      }
    }
    if (pos < transmit.length()) data_to_send[counter++] = transmit.substring(pos).toInt();

    int tx_pin = (tmp_module == "1") ? 2 : 25;
    ELECHOUSE_cc1101.setModul((tmp_module == "1") ? 0 : 1);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(mod);
    ELECHOUSE_cc1101.setMHZ(frequency);
    ELECHOUSE_cc1101.setDeviation(deviation);
    ELECHOUSE_cc1101.SetTx();
    pinMode(tx_pin, OUTPUT);

    for (int i = 0; i < counter; i += 2) {
      digitalWrite(tx_pin, HIGH);
      delayMicroseconds(data_to_send[i]);
      digitalWrite(tx_pin, LOW);
      delayMicroseconds(data_to_send[i+1]);
      //Serial.print(data_to_send[i]);
      //Serial.print(",");
    }

    ELECHOUSE_cc1101.setSidle();
    request->send(200, "text/plain", "Signal has been transmitted");
});


  controlserver.on("/setjammer", HTTP_POST, [](AsyncWebServerRequest *request){
    if (!request->hasArg("module") || !request->hasArg("frequency") || !request->hasArg("power")) {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing parameters (module, frequency, power required)\"}");
      return;
    }

    tmp_module = request->arg("module");
    frequency = request->arg("frequency").toFloat();
    int power_jammer = request->arg("power").toInt();

    if (tmp_module != "1" && tmp_module != "2") {
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid module (must be 1 or 2)\"}");
      return;
    }

    int moduleIndex = (tmp_module == "1") ? 0 : 1;
    int tx_pin = (tmp_module == "1") ? tx_pin1 : tx_pin2;

    pinMode(tx_pin, OUTPUT);
    ELECHOUSE_cc1101.setModul(moduleIndex);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setModulation(2);
    ELECHOUSE_cc1101.setMHZ(frequency);
    ELECHOUSE_cc1101.setPA(power_jammer);
    ELECHOUSE_cc1101.SetTx();

    jammer_tx = "1";
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Jammer started\"}");
  });

  controlserver.on("/stopjammer", HTTP_POST, [](AsyncWebServerRequest *request) {
    ELECHOUSE_cc1101.setModul(0);
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.setModul(1);
    ELECHOUSE_cc1101.setSidle();

    jammer_tx = "0";
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Jammer stopped\"}");
  });

  controlserver.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/HTML/style.css", "text/css");
  });

  controlserver.begin();
  ELECHOUSE_cc1101.addSpiPin(sck_pin, miso_pin, mosi_pin, cs_pin1, 0);
  ELECHOUSE_cc1101.addSpiPin(sck_pin, miso_pin, mosi_pin, cs_pin2, 1);
  
  enableReceive();
}

void loop() {
  if(raw_rx == "1") {
    if(checkReceived()){
      printReceived();
      signalanalyse();
      enableReceive();
      delay(700);
    }
  }
  if(jammer_tx == "1") {
    
    if (tmp_module == "1") {
      for (int i = 0; i<12; i+=2){
        digitalWrite(tx_pin1,HIGH);
        delayMicroseconds(jammer[i]);
        digitalWrite(tx_pin1,LOW);
        delayMicroseconds(jammer[i+1]);
      }
    }
    else if (tmp_module == "2") {
      for (int i = 0; i<12; i+=2){
        digitalWrite(tx_pin2,HIGH);
        delayMicroseconds(jammer[i]);
        digitalWrite(tx_pin2,LOW);
        delayMicroseconds(jammer[i+1]);
      }
    }
  }
}