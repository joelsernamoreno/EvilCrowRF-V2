#include "config.h"

#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <WiFiClient.h> 
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ESP32-targz.h>
#include <SPIFFSEditor.h>
#include <EEPROM.h>
#include "SPIFFS.h"
#include "SPI.h"
#include <WiFiAP.h>
#include "FS.h"
#include "SD.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"  // install from library manager


#define signalstorage 10

SPIClass sdspi(VSPI);

// Wifi parameters
const char* ssid = "Evil Crow RF v2";
const char* password = "123456789";
const int wifi_channel = 12;

// HTML and CSS style
const String HTML_CSS_STYLING = "<html><head><meta charset=\"utf-8\"><title>Evil Crow RF</title>" \
                                "<link rel=\"stylesheet\" href=\"style.css\"><script src=\"lib.js\"></script></head>";

static unsigned long Blinktime = 0;

unsigned long sample[samplesize];
unsigned long samplesmooth[samplesize];
int samplecount;
static unsigned long lastTime = 0;
String transmit = "";
long data_to_send[BUFFER_MAX_SIZE];
long transmit_push[BUFFER_MAX_SIZE];
String tmp_module;
String tmp_frequency;
String tmp_setrxbw;
String tmp_mod;
int mod;
String tmp_deviation;
float deviation;
String tmp_datarate;

int datarate;
float frequency;
float setrxbw;
String raw_rx = "0";
String bindata;
int samplepulse;
int pos = 0;
int transmissions;
byte jammer[11] = {0xff,0xff,};

String bindataprotocol;
String bindata_protocol;

// Wi-Fi config storage
int storage_status;
String tmp_config1;
String tmp_config2;
String config_wifi;
String ssid_new;
String password_new;
String tmp_channel_new;
String tmp_mode_new;
int channel_new;
int mode_new;

// Tesla
const uint16_t pulseWidth = 400;
const uint16_t messageDistance = 23;
const uint8_t transmtesla = 5;
const uint8_t messageLength = 43;

const uint8_t sequence[messageLength] = { 
  0x02,0xAA,0xAA,0xAA,  // Preamble of 26 bits by repeating 1010
  0x2B,                 // Sync byte
  0x2C,0xCB,0x33,0x33,0x2D,0x34,0xB5,0x2B,0x4D,0x32,0xAD,0x2C,0x56,0x59,0x96,0x66,
  0x66,0x5A,0x69,0x6A,0x56,0x9A,0x65,0x5A,0x58,0xAC,0xB3,0x2C,0xCC,0xCC,0xB4,0xD2,
  0xD4,0xAD,0x34,0xCA,0xB4,0xA0};

struct JammerConfig {
  boolean active;
  int module;
  float frequency;
  int power;  
} jammerConfig;

struct buttonConfig {
    boolean setted;
    float deviation;
    float frequency;
    int modulation;
    boolean tesla;
    int module;
    int transmissions;
    int signal_size;
    long data[BUFFER_MAX_SIZE];
} buttonConfig[2];

struct FZSubSignal {
    boolean setted;
    int version;
    float frequency;
    int protocol;       // Now: RAW or BinRAW (first format with pulses and second with hex values)
    int modulation;     // 2-FSK, GFSK, ASK, 4-FSK, MSK
    int te;
    int size;
    int data[FZ_SUB_MAX_SIZE];    
} fzSubSignal;

// File
File logs;
File file;

AsyncWebServer controlserver(WEB_SERVER_PORT);

void readConfigWiFi(fs::FS &fs, String path) {
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    storage_status = 0;
    return;
  }
  
  while(file.available()){
    config_wifi = file.readString();
    int file_len = config_wifi.length()+1;
    int index_config = config_wifi.indexOf('\n');
    tmp_config1 = config_wifi.substring(0, index_config-1);
    
    tmp_config2 = config_wifi.substring(index_config+1, file_len-3);
    storage_status = 1;
  }
  file.close();
}

void writeConfigWiFi(fs::FS &fs, const char * path, String message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
      return;
    }
    if (file.println(message)) {
    } else {
    }
    file.close();
}

// handles uploads
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  String path = filename;
  path.toLowerCase();
  if (path.indexOf(".sub") >= 0)
    path = "/DATA/FZSUB/" + filename;
  else if(path.indexOf(".xml") >= 0)
    path = "/DATA/URH/" + filename;
  else path = "/DATA/" + filename;

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    request->_tempFile = SD.open(path, "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
  }
  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    request->_tempFile.close();
    request->redirect("/");
  }
}

// handles uploads SD Files
void handleUploadSD(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  removeDir(SD, "/HTML");
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    request->_tempFile = SD.open("/" + filename, "w");
  }
  if (len) {
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
  }
  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    request->_tempFile.close();
    tarGzFS.begin();

    TarUnpacker *TARUnpacker = new TarUnpacker();

    TARUnpacker->haltOnError(true);   // stop on fail (manual restart/reset required)
    TARUnpacker->setTarVerify(true);  // true = enables health checks but slows down the overall process
    TARUnpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);         // prevent the partition from exploding, recommended
    TARUnpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback); // prints the untarring progress for each individual file
    TARUnpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback); // print the filenames as they're expanded
    TARUnpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback); // tar log verbosity

    if( !TARUnpacker->tarExpander(tarGzFS, "/HTML.tar", tarGzFS, "/") ) {
      Serial.printf("tarExpander failed with return code #%d\n", TARUnpacker->tarGzGetError() );
    }
    deleteFile(SD, "/HTML.tar");
    request->redirect("/");
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  // TODO: remove function
  deleteFile(SD, "/dir.txt");

  File root = fs.open(dirname);
  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }

  File file = root.openNextFile();
  while(file) {
    if(file.isDirectory()){
      appendFile(SD, "/dir.txt","  DIR : ", file.name());
      if(levels) {
        listDir(fs, file.name(), levels -1);
      }
    } else {
      appendFile(SD, "/dir.txt","", "<br>");
      appendFile(SD, "/dir.txt","", file.name());
      appendFile(SD, "/dir.txt","  SIZE: ", "");
      appendFileLong(SD, "/dir.txt",file.size());
    }
    file = root.openNextFile();
  }
}

void appendFile(fs::FS &fs, const char * path, const char * message, String messagestring){
  logs = fs.open(path, FILE_APPEND);
  if(!logs) {
    return;
  }
  logs.print(message);
  logs.print(messagestring);
  logs.close();
}

void appendFileLong(fs::FS &fs, const char * path, unsigned long messagechar){
  logs = fs.open(path, FILE_APPEND);
  if(!logs){
    return;
  }
  logs.print(messagechar);
  logs.close();
}

void deleteFile(fs::FS &fs, const char * path) {
  fs.remove(path);
}

void readFile(fs::FS &fs, String path) {
  File file = fs.open(path);
  if(!file){
    return;
  }
  while(file.available()){
    bindataprotocol = file.readString();
  }
  file.close();
}


void removeDir(fs::FS &fs, const char * dirname) {
  File root = fs.open(dirname);
  if(!root){
    return;
  }
  if(!root.isDirectory()){
    return;
  }
  File file = root.openNextFile();
  while(file){   
    if(file.isDirectory()){
      deleteFile(SD, file.name());
    } else {
      deleteFile(SD, file.name());
    }
    file = root.openNextFile();
  }
}

bool checkReceived(void) {
  delay(1);
  if (samplecount >= MIN_SAMPLE && micros()-lastTime > 100000) {
    detachInterrupt(MOD0_GDO2);
    detachInterrupt(MOD1_GDO2);
    return 1;
  } else {
    return 0;
  }
}

void printReceived(){
  Serial.print("Count=");
  Serial.println(samplecount);
  appendFile(SD, "/logs.txt", NULL, "<br>\n");
  appendFile(SD, "/logs.txt", NULL, "Count=");
  appendFileLong(SD, "/logs.txt", samplecount);
  appendFile(SD, "/logs.txt", NULL, "<br>");
  
  for (int i = 1; i<samplecount; i++){
    Serial.print(sample[i]);
    Serial.print(",");
    appendFileLong(SD, "/logs.txt", sample[i]);
    appendFile(SD, "/logs.txt", NULL, ",");  
  }
  Serial.println();
  Serial.println();
  appendFile(SD, "/logs.txt", "<br>\n", "<br>\n");
  appendFile(SD, "/logs.txt", "\n", "\n");
}

void RECEIVE_ATTR receiver() {
  const long time = micros();
  const unsigned int duration = time - lastTime;
  if (duration > 100000) samplecount = 0;
  if (duration >= 100) sample[samplecount++] = duration;
  if (samplecount >= samplesize) {
    detachInterrupt(MOD0_GDO2);
    detachInterrupt(MOD1_GDO2);
    checkReceived();
  }
  if (mod == 0 && tmp_module == "2") {
    if (samplecount == 1 and digitalRead(MOD1_GDO2) != HIGH){
      samplecount = 0;
    }
  }
  lastTime = time;
}

void enableReceive() {
  pinMode(MOD0_GDO2,INPUT);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(digitalPinToInterrupt(MOD0_GDO2), receiver, CHANGE);
  pinMode(MOD1_GDO2,INPUT);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(digitalPinToInterrupt(MOD1_GDO2), receiver, CHANGE);
}

void parse_data() {
  bindata_protocol = "";
  int data_begin_bits = 0;
  int data_end_bits = 0;
  int data_begin_pause = 0;
  int data_end_pause = 0;
  int data_count = 0;
  for (int c = 0; c<bindataprotocol.length(); c++){
    if (bindataprotocol.substring(c,c+4) == "bits"){
      data_count++;
    }
  }
  for (int d = 0; d<data_count; d++){
    data_begin_bits = bindataprotocol.indexOf("<message bits=", data_end_bits);
    data_end_bits = bindataprotocol.indexOf("decoding_index=", data_begin_bits+1);
    bindata_protocol += bindataprotocol.substring(data_begin_bits+15, data_end_bits-2);
    data_begin_pause = bindataprotocol.indexOf("pause=", data_end_pause);
    data_end_pause = bindataprotocol.indexOf(" timestamp=", data_begin_pause+1);
    bindata_protocol += "[Pause: ";
    bindata_protocol += bindataprotocol.substring(data_begin_pause+7, data_end_pause-1);
    bindata_protocol += " samples]\n";
  }
  bindata_protocol.replace(" ","");
  bindata_protocol.replace("\n","");
  bindata_protocol.replace("Pause:","");
  Serial.println("Parsed Data:");
  Serial.println(bindata_protocol);
}

void sendTeslaSignal(float freq, int module) {
  pinMode(MOD0_GDO0,OUTPUT);
  ELECHOUSE_cc1101.setModul(module);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(freq);
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.SetTx();
  for (uint8_t t=0; t<transmtesla; t++) {
    for (uint8_t i=0; i<messageLength; i++) sendByte(sequence[i]);
    digitalWrite(MOD0_GDO0, LOW);
    delay(messageDistance);
  }
  ELECHOUSE_cc1101.setSidle();
}

void sendButtonSignal(int button) {
  raw_rx = "0";
  if (buttonConfig[button].tesla) {
    sendTeslaSignal(buttonConfig[button].frequency, buttonConfig[button].module);
    return;
  }
  pinMode(MOD1_GDO0, OUTPUT);
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(buttonConfig[button].modulation);
  ELECHOUSE_cc1101.setMHZ(buttonConfig[button].frequency);
  ELECHOUSE_cc1101.setDeviation(buttonConfig[button].deviation);
  ELECHOUSE_cc1101.SetTx();

  for (int t = 0; t < buttonConfig[button].transmissions; t++) {
      for (int i = 0; i < buttonConfig[button].signal_size; i++){
        digitalWrite(MOD1_GDO0, i%2 == 0 ? HIGH : LOW);
        delayMicroseconds(buttonConfig[button].data[i]);
        Serial.print(buttonConfig[button].data[i]);
        Serial.print(",");
      }
      delay(DELAY_BETWEEN_RETRANSMISSIONS); //Set this for the delay between retransmissions
    }
   Serial.println();
   ELECHOUSE_cc1101.setSidle();
}

void sendByte(uint8_t dataByte) {
  for (int8_t bit=7; bit>=0; bit--) { // MSB
    digitalWrite(MOD0_GDO0, (dataByte & (1 << bit)) != 0 ? HIGH : LOW);
    delayMicroseconds(pulseWidth);
  }
}

void sendBits(int *buff, int length, int gdo0) {
  for (int i = 0; i < length; i++) {
    digitalWrite(gdo0, buff[i] < 0 ? LOW : HIGH);
    delayMicroseconds(abs(buff[i]));
  }
  digitalWrite(gdo0, LOW);
}

void parseFZSubLine(char *line) {
  // using pure C functions but works... :/
  const char sep[2] = ":";
  const char values_sep[2] = " ";
  
  char *key = strtok(line, sep);
  char *value;

  fzSubSignal.setted = true;  // TODO: set only if fz filetype is recognized
  
  if (key != NULL) {
    value = strtok(NULL, sep);
    if(!strcmp(key, "Version")) {
      fzSubSignal.version = atoi(value);
    } else if(!strcmp(key, "Frequency")) {
      fzSubSignal.frequency = atof(value) / 1000000.0f;
    } else if(!strcmp(key, "Preset")) {
      fzSubSignal.modulation = 2;   // ASK/OOK; TODO: translate strings like: FuriHalSubGhzPresetOok270Async
    } else if(!strcmp(key, "RAW_Data")) {
      char *pulse = strtok(value, values_sep);
      int i = fzSubSignal.size;          
      while (pulse != NULL && i < FZ_SUB_MAX_SIZE) {
        fzSubSignal.data[i] = atoi(pulse);
        pulse = strtok(NULL, values_sep);
        i++;
      }
      fzSubSignal.size = i;
    }
  }
}

void loadFZSubSignal(String path) {
  memset(&fzSubSignal, 0, sizeof(struct FZSubSignal));
  File dataFile = SD.open(path, FILE_READ);
  if (dataFile) {
    char *buf = (char *) malloc(MAX_LINE_SIZE);
    String line;
    while (dataFile.available()) {
      line = dataFile.readStringUntil('\n');  // \n character is discarded from buffer
      line.toCharArray(buf, MAX_LINE_SIZE);
      parseFZSubLine(buf);
    }
    Serial.print("FZ signal -> Freq:");
    Serial.print(fzSubSignal.frequency);
    Serial.print(", Mod:");
    Serial.print(fzSubSignal.modulation);
    Serial.print(", Size:");
    Serial.print(fzSubSignal.size);
    Serial.println(".");
    dataFile.close();
    free(buf);      
  }
}

void power_management() {
  EEPROM.begin(eepromsize);
  byte z = EEPROM.read(eepromsize-2);

  if (digitalRead(BUTTON2) != LOW) {
    if (z == 1){
      go_deep_sleep();
    }
  } else {
    if (z == 0) {
      EEPROM.write(eepromsize-2, 1);
      EEPROM.commit();
      go_deep_sleep();
    } else {
      EEPROM.write(eepromsize-2, 0);
      EEPROM.commit();
    }
  }
}

void go_deep_sleep() {
  Serial.println("Going to sleep now");
  ELECHOUSE_cc1101.setModul(0);
  ELECHOUSE_cc1101.goSleep();
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.goSleep();
  led_blink(5, 250);
  esp_deep_sleep_start();
}

void led_blink(int blinkrep,int blinktimer){
  for (int i = 0; i<blinkrep; i++){
    digitalWrite(LED, HIGH);
    delay(blinktimer);
    digitalWrite(LED, LOW);
    delay(blinktimer);
  }
}

void poweron_blink(){
  if (millis()-Blinktime > 10000){
    digitalWrite(LED, LOW);
  }
  if (millis()-Blinktime > 10100){
    digitalWrite(LED, HIGH);
    Blinktime = millis();
  }
}

void force_reset() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);

  Serial.begin(SERIAL_BAUDRATE);
  power_management();

  SPIFFS.begin(FORMAT_ON_FAIL);

  readConfigWiFi(SPIFFS,"/configwifi.txt");
  ssid_new = tmp_config1;
  password_new = tmp_config2;
  readConfigWiFi(SPIFFS,"/configmode.txt");
  tmp_channel_new = tmp_config1;
  tmp_mode_new = tmp_config2;
  channel_new = tmp_channel_new.toInt();
  mode_new = tmp_mode_new.toInt();

  jammerConfig.active = false;
  
  if (storage_status == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password,wifi_channel,8);
  }

  if(storage_status == 1) {
    if(mode_new == 1) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(ssid_new.c_str(), password_new.c_str(),channel_new,8);
    } else if(mode_new == 2) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid_new.c_str(), password_new.c_str());
    } 
  }
  delay(DELAY_BETWEEN_RETRANSMISSIONS);

  sdspi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_SS);
  SD.begin(SD_SS, sdspi);

  /* ENDPOINTS */
  controlserver.serveStatic("/", SD, "/HTML/").setDefaultFile("index.html");

  controlserver.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    JsonArray array = doc.to<JsonArray>();
    
    char response[JSON_DOC_SIZE];

    String fs = request->arg("fs");
    String path = request->arg("path");
    
    File root = fs.equals("spiffs") ? SPIFFS.open(path) : SD.open(path);
    if (!root || !root.isDirectory()) {
        request->send(HTTP_BAD_REQUEST);
    }
    File file = root.openNextFile();
    String name;
    while (file) {
      name = file.name(); // copy value & avoid pointer corruption
      JsonObject jsonFileObject = array.createNestedObject();
      jsonFileObject["name"] = name;
      if (file.isDirectory()) {
        jsonFileObject["type"] = "directory";
        jsonFileObject["size"] = 0;
      } else {
        jsonFileObject["type"] = "file";
        jsonFileObject["size"] = file.size();        
      }
      file = root.openNextFile();
    }
    serializeJson(doc, response);
    doc.clear();
    request->send(HTTP_OK, "application/json", response);
  });
  
  controlserver.on("/listxmlfiles", HTTP_GET, [](AsyncWebServerRequest *request) {
    listDir(SD, "/URH", 0);
    request->send(SD, "/dir.txt", "text/html");
  });

  controlserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200); }, handleUpload);

  controlserver.on("/uploadsd", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200); }, handleUploadSD);

  controlserver.on("/setjammer", HTTP_POST, [](AsyncWebServerRequest *request){
    raw_rx = "0";
    if (request->hasArg("configmodule") && request->hasArg("turn")) {
      if (request->arg("turn").equals("ON")) {
        jammerConfig.module = request->arg("module").toInt()-1;
        jammerConfig.frequency = request->arg("frequency").toFloat();
        jammerConfig.power = request->arg("powerjammer").toInt();
        pinMode(jammerConfig.module == 0 ? MOD0_GDO0 : MOD1_GDO0 ,OUTPUT);
        ELECHOUSE_cc1101.setModul(jammerConfig.module);
        ELECHOUSE_cc1101.Init();
        ELECHOUSE_cc1101.setModulation(2);
        ELECHOUSE_cc1101.setMHZ(jammerConfig.frequency);
        ELECHOUSE_cc1101.setPA(jammerConfig.power);
        ELECHOUSE_cc1101.SetTx();
        jammerConfig.active = true;
        request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Jammer started!\")</script>");
      } else if(request->arg("turn").equals("OFF")) {
        jammerConfig.active = false;
        ELECHOUSE_cc1101.setModul(jammerConfig.module);
        ELECHOUSE_cc1101.setSidle();
        request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Jammer stopped.\")</script>");
      } else {
        request->send(HTTP_BAD_REQUEST);
      };
    } else request->send(HTTP_BAD_REQUEST);
  });

  controlserver.on("/transmit/raw", HTTP_POST, [](AsyncWebServerRequest *request) {
    raw_rx = "0";

    if (request->hasArg("configmodule")) {
      int counter = 0;
      int pos = 0;
      String rawdata = request->arg("rawdata");
      int module = request->arg("module").toInt() - 1;
      float frequency = request->arg("frequency").toFloat();
      float deviation = request->arg("deviation").toFloat();
      int modulation = request->arg("mod").toInt();
      int transmissions = request->arg("transmissions").toInt();

      for (int i = 0; i<rawdata.length(); i++) {
        if (rawdata.substring(i, i+1) == ",") {
          data_to_send[counter] = rawdata.substring(pos, i).toInt();
          pos = i+1;
          counter++;
        }
      }
      if (module < 0 || module > 1) {
        request->send(HTTP_BAD_REQUEST);
        return;
      }
      int gdo0 = module == 0 ? MOD0_GDO0 : MOD1_GDO0;
      pinMode(gdo0, OUTPUT);
      ELECHOUSE_cc1101.setModul(module);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(modulation);
      ELECHOUSE_cc1101.setMHZ(frequency);
      ELECHOUSE_cc1101.setDeviation(deviation);
      ELECHOUSE_cc1101.SetTx();
      for (int r = 0; r < transmissions; r++) {
        for (int i = 0; i < counter; i++){
          digitalWrite(gdo0, i%2 == 0 ? HIGH : LOW);
          delayMicroseconds(data_to_send[i]);
          Serial.print(data_to_send[i]);
          Serial.print(",");
        }
        delay(DELAY_BETWEEN_RETRANSMISSIONS); //Set this for the delay between retransmissions
      }       
      Serial.println();
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Signal has been transmitted\")</script>");
      ELECHOUSE_cc1101.setSidle();
    } else {
      request->send(HTTP_BAD_REQUEST);
    }
  });

  controlserver.on("/transmit/tesla", HTTP_POST, [](AsyncWebServerRequest *request) {
    raw_rx = "0";
    if (request->hasArg("configmodule")) {
      float frequency = request->arg("frequency").toFloat();
      sendTeslaSignal(frequency, 0);
      ELECHOUSE_cc1101.setSidle();
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Signal has been transmitted\")</script>");
    }
  });

  controlserver.on("/transmit/binary", HTTP_POST, [](AsyncWebServerRequest *request) {
    raw_rx = "0";
    if (request->hasArg("configmodule")) {
      float frequency = request->arg("frequency").toFloat();
      int module = request->arg("module").toInt() - 1;
      String bindata = request->arg("binarydata");
      float deviation = request->arg("deviation").toFloat();
      int mod = request->arg("mod").toInt();
      int samplepulse = request->arg("samplepulse").toInt();
      int transmissions = request->arg("transmissions").toInt();
      
      if (module < 0 || module > 1) {
        request->send(HTTP_BAD_REQUEST);
        return;
      }
      for (int i = 0; i < BUFFER_MAX_SIZE; i++) data_to_send[i] = 0;
      
      Serial.println();
      bindata.replace(" ","");
      bindata.replace("\n","");
      bindata.replace("Pause:","");
      Serial.println(bindata);
      
      int counter = 0;
      int pos = 0;      
      int count_binconvert = 0;
      String lastbit_convert="1";
      
      for (int i = 0; i < bindata.length()+1; i++) {
        if (lastbit_convert != bindata.substring(i, i+1)) {
          if (lastbit_convert == "1") {
            lastbit_convert="0";
          } else if (lastbit_convert == "0") {
            lastbit_convert="1";
          }
          count_binconvert++;
        }
        if (bindata.substring(i, i+1)=="[") {
          data_to_send[count_binconvert]= bindata.substring(i+1,bindata.indexOf("]",i)).toInt();
          lastbit_convert="0";
          i+= bindata.substring(i,bindata.indexOf("]",i)).length();
        } else {
          data_to_send[count_binconvert] += samplepulse;
        }
      }
      for (int i = 0; i<count_binconvert; i++) {
        Serial.print(data_to_send[i]);
        Serial.print(",");
      }
      int gdo0 = module == 0 ? MOD0_GDO0 : MOD1_GDO0;

      pinMode(gdo0, OUTPUT);
      ELECHOUSE_cc1101.setModul(module);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(mod);
      ELECHOUSE_cc1101.setMHZ(frequency);
      ELECHOUSE_cc1101.setDeviation(deviation);
      ELECHOUSE_cc1101.SetTx();
      delay(1000);      
      for (int r = 0; r<transmissions; r++) {
        for (int i = 0; i<count_binconvert; i++){
          digitalWrite(gdo0, i%2 == 0 ? HIGH : LOW);
          delayMicroseconds(data_to_send[i]);
        }
        delay(DELAY_BETWEEN_RETRANSMISSIONS);
      }
      ELECHOUSE_cc1101.setSidle();
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Signal has been transmitted\")</script>");
    }
  });

  controlserver.on("/transmit/file/urh", HTTP_POST, [](AsyncWebServerRequest *request){
    raw_rx = "0";
    if (request->hasArg("configmodule")) {
      int pos = 0;
      float frequency = request->arg("frequency").toFloat();
      float deviation = request->arg("deviation").toFloat();
      int mod = request->arg("mod").toInt();
      int samplepulse = request->arg("samplepulse").toInt();
      String filepath = request->arg("filepath");
      bindata_protocol = "";
      for (int i=0; i<1000; i++){
        data_to_send[i]=0;
      }
      readFile(SD, filepath);
      parse_data();
      int count_binconvert=0;
      String lastbit_convert="1";
      for (int i = 0; i < bindata_protocol.length()+1; i++) {
        if (lastbit_convert != bindata_protocol.substring(i, i+1)) {
          if (lastbit_convert == "1") {
            lastbit_convert="0";
          } else if (lastbit_convert == "0") {
            lastbit_convert="1";
          }
          count_binconvert++;
        }
        if (bindata_protocol.substring(i, i+1)=="["){
          data_to_send[count_binconvert]= bindata_protocol.substring(i+1,bindata_protocol.indexOf("]",i)).toInt();
          lastbit_convert="0";
          i+= bindata_protocol.substring(i,bindata_protocol.indexOf("]",i)).length();
        } else {
          data_to_send[count_binconvert]+=samplepulse;
        }
      }
      Serial.println("Data to Send");
      for (int i = 0; i<count_binconvert; i++){
        Serial.print(data_to_send[i]);
        Serial.print(",");
      }
      pinMode(MOD0_GDO0,OUTPUT);
      ELECHOUSE_cc1101.setModul(0);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(mod);
      ELECHOUSE_cc1101.setMHZ(frequency);
      ELECHOUSE_cc1101.setDeviation(deviation);
      ELECHOUSE_cc1101.SetTx();
      delay(1000);
      for (int i = 0; i<count_binconvert; i++){
        digitalWrite(MOD0_GDO0, i%2 == 0 ? HIGH : LOW);
        delayMicroseconds(data_to_send[i]);
      }
      ELECHOUSE_cc1101.setSidle();
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Signal has been transmitted\")</script>");
    }
  });

  controlserver.on("/transmit/file/fz", HTTP_POST, [](AsyncWebServerRequest *request){
    raw_rx = "0";
    if (request->hasArg("configmodule")) {
      String filepath = request->arg("filepath");
      loadFZSubSignal(filepath);
      pinMode(MOD0_GDO0, OUTPUT);
      ELECHOUSE_cc1101.setModul(0);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(fzSubSignal.modulation);
      ELECHOUSE_cc1101.setMHZ(fzSubSignal.frequency);
      ELECHOUSE_cc1101.setDeviation(0);
      ELECHOUSE_cc1101.SetTx();
      sendBits(fzSubSignal.data, fzSubSignal.size, MOD0_GDO0);
      ELECHOUSE_cc1101.setSidle();
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Signal has been transmitted\")</script>");
    } else {
      request->send(HTTP_BAD_REQUEST);
    }    
  });

  controlserver.on("/setrx", HTTP_POST, [](AsyncWebServerRequest *request){
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
        if(mod == 2) {
          ELECHOUSE_cc1101.setDcFilterOff(0);
        }
        if(mod == 0) {
          ELECHOUSE_cc1101.setDcFilterOff(1);
        }
      }
      ELECHOUSE_cc1101.setSyncMode(0);            // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
      ELECHOUSE_cc1101.setPktFormat(3);           // Format of RX and TX data. 0 = Normal mode, use FIFOs for RX and TX. 1 = Synchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins. 2 = Random TX mode; sends random data using PN9 generator. Used for test. Works as normal mode, setting 0 (00), in RX. 3 = Asynchronous serial mode, Data in on GDO0 and data out on either of the GDOx pins.
      ELECHOUSE_cc1101.setModulation(mod);        // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
      ELECHOUSE_cc1101.setRxBW(setrxbw);
      ELECHOUSE_cc1101.setMHZ(frequency);
      ELECHOUSE_cc1101.setDeviation(deviation);   // Set the Frequency deviation in kHz. Value from 1.58 to 380.85. Default is 47.60 kHz.
      ELECHOUSE_cc1101.setDRate(datarate);        // Set the Data Rate in kBaud. Value from 0.02 to 1621.83. Default is 99.97 kBaud!
      enableReceive();
      raw_rx = "1";
      request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"RX Config OK\")</script>");
    }
  });

  controlserver.on("/setbtn", HTTP_POST, [](AsyncWebServerRequest *request){
    raw_rx = "0";
    int button = request->arg("button").toInt() - 1;
    if (button < 0 || button > 1) 
      request->send(HTTP_BAD_REQUEST, "text/html", "Button value is not valid.");
    else {
      String setted = request->arg("set");
      if (setted.equals("Set") && request->hasArg("tesla")) {
        buttonConfig[button].setted = true;
        buttonConfig[button].tesla = true;
        buttonConfig[button].frequency = request->arg("frequency").toFloat();
        request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Asigned tesla signal to button\")</script>");
      } else if (setted.equals("Set")) {
        String rawdata = request->arg("rawdata");
        buttonConfig[button].deviation = request->arg("deviation").toFloat();
        buttonConfig[button].frequency = request->arg("frequency").toFloat();
        buttonConfig[button].modulation = request->arg("mod").toInt();
        buttonConfig[button].transmissions = request->arg("transmissions").toInt();
        buttonConfig[button].setted = true;
        buttonConfig[button].tesla = false;
        int c = 0;
        for (int i = 0; i < rawdata.length(); i++) {
          if (rawdata.substring(i, i+1) == ","){
            buttonConfig[button].data[c] = rawdata.substring(pos, i).toInt();
            pos = i+1;
            c++;
          }
        }
        buttonConfig[button].signal_size = c;
        request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Raw signal configured at button\")</script>");
      } else {
        buttonConfig[button].setted = false;
        buttonConfig[button].tesla = false;
        request->send(200, "text/html", HTML_CSS_STYLING + "<script>alert(\"Cleared button config\")</script>");
      }
      request->send(HTTP_BAD_REQUEST, "text/html", "Button value is not valid.");
    }
  });

  controlserver.on("/logview", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/logs.txt", "text/html");
  });

  controlserver.on("/logdownload", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/logs.txt", String(), true);
  });

  controlserver.on("/logdelete", HTTP_GET, [](AsyncWebServerRequest *request){
    deleteFile(SD, "/logs.txt");
    request->send(200, "text/html", HTML_CSS_STYLING+ "<body onload=\"JavaScript:AutoRedirect()\">"
    "<br><h2>File cleared!<br>You will be redirected in 5 seconds.</h2></body>");
    appendFile(SD, "/logs.txt","Viewlog:\n", "<br>\n");
  });

  controlserver.on("/cleanspiffs", HTTP_GET, [](AsyncWebServerRequest *request){
    SPIFFS.remove("/");
    request->send(200, "text/html", HTML_CSS_STYLING+ "<body onload=\"JavaScript:AutoRedirect()\">"
    "<br><h2>SPIFFS cleared!<br>You will be redirected in 5 seconds.</h2></body>" );
  });


  controlserver.on("/setwificonfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    String ssid_value = request->arg("ssid");
    String password_value = request->arg("password");
    String channel_value = request->arg("channel");
    String mode_value = request->arg("mode");
    
    if (request->hasArg("configmodule")) {
      deleteFile(SPIFFS, "/configwifi.txt");
      deleteFile(SPIFFS, "/configmode.txt");
      writeConfigWiFi(SPIFFS, "/configwifi.txt", ssid_value);
      writeConfigWiFi(SPIFFS, "/configwifi.txt", password_value);
      writeConfigWiFi(SPIFFS, "/configmode.txt", channel_value);
      writeConfigWiFi(SPIFFS, "/configmode.txt", mode_value);
      WiFi.disconnect();
      force_reset();
    }
  });
  controlserver.on("/deletewificonfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasArg("configmodule")) {
      deleteFile(SPIFFS, "/configwifi.txt");
      deleteFile(SPIFFS, "/configmode.txt");
      force_reset();
    }
  });
  /* end section of endpoints */

  AsyncElegantOTA.begin(&controlserver);
  controlserver.begin();

  ELECHOUSE_cc1101.addSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_SS0, 0); // (0) first module
  ELECHOUSE_cc1101.addSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_SS1, 1); // (1) second module
  appendFile(SD, "/logs.txt","Viewlog:\n", "<br>\n");
}

void signalanalyse(){
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
  for (int p = 0; p<signalstorage; p++) {
    for (int i = 1; i<samplecount; i++) {
      if (p==0){
        if (sample[i]<signaltimings[p*2]){
          signaltimings[p*2]=sample[i];
        }
      } else {
        if (sample[i]<signaltimings[p*2] && sample[i]>signaltimings[p*2-1]){
          signaltimings[p*2]=sample[i];
        }
      }
    }
    for (int i = 1; i<samplecount; i++) {
      if (sample[i]<signaltimings[p*2] + ERROR_TOLERANZ && sample[i]>signaltimings[p*2+1]) {
        signaltimings[p*2+1]=sample[i];
      }
    }
    for (int i = 1; i<samplecount; i++) {
      if (sample[i]>=signaltimings[p*2] && sample[i]<=signaltimings[p*2+1]) {
        signaltimingscount[p]++;
        signaltimingssum[p]+=sample[i];
      }
    }
  }
  int firstsample = signaltimings[0];
  signalanz = signalstorage;
  for (int i = 0; i<signalstorage; i++) {
    if (signaltimingscount[i] == 0) {
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
      if (lastbin == 0 && calculate > 8){
        Serial.print(" [Pause: ");
        Serial.print(sample[i]);
        Serial.println(" samples]");
        appendFile(SD, "/logs.txt",NULL, " [Pause: ");
        appendFileLong(SD, "/logs.txt", sample[i]);
        appendFile(SD, "/logs.txt"," samples]", "\n");
      }else{
        for (int b=0; b<calculate; b++){
          Serial.print(lastbin);
          appendFileLong(SD, "/logs.txt", lastbin);
        }
      }
    }
  }
  Serial.println();
  Serial.print("Samples/Symbol: ");
  Serial.println(timingdelay[0]);
  Serial.println();
  appendFile(SD, "/logs.txt","<br>\n", "<br>\n");
  appendFile(SD, "/logs.txt",NULL, "Samples/Symbol: ");
  appendFileLong(SD, "/logs.txt", timingdelay[0]);
  appendFile(SD, "/logs.txt",NULL, "<br>\n");

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
  Serial.println("Rawdata corrected:");
  Serial.print("Count=");
  Serial.println(smoothcount+1);
  appendFile(SD, "/logs.txt",NULL, "Count=");
  appendFileLong(SD, "/logs.txt", smoothcount+1);
  appendFile(SD, "/logs.txt","\n", "<br>\n");
  appendFile(SD, "/logs.txt",NULL, "Rawdata corrected:\n");
  for (int i=0; i<smoothcount; i++){
    Serial.print(samplesmooth[i]);
    Serial.print(",");
    transmit_push[i] = samplesmooth[i];
    appendFileLong(SD, "/logs.txt", samplesmooth[i]);
    appendFile(SD, "/logs.txt", NULL, ",");  
  }
  Serial.println();
  Serial.println();
  appendFile(SD, "/logs.txt", NULL, "<br>\n");
  appendFile(SD, "/logs.txt", "-------------------------------------------------------\n", "<br>");
  return;
}

void loop() {
  poweron_blink();
  if(raw_rx == "1") {
    if(checkReceived()){
      printReceived();
      signalanalyse();
      enableReceive();
      delay(200);
      delay(500);
    }
  }
  if(jammerConfig.active) {
    raw_rx = "0";
    if (jammerConfig.module == 0 || jammerConfig.module == 1) {
      for (int i = 0; i < sizeof(jammer); i++) {
        digitalWrite(jammerConfig.module == 0 ? MOD0_GDO0 : MOD1_GDO0, i%2 == 0 ? HIGH : LOW);
        delayMicroseconds(jammer[i]);
      }
    }
  }
  if (digitalRead(BUTTON1) == LOW && buttonConfig[0].setted) {
        sendButtonSignal(0);
  }
  if (digitalRead(BUTTON2) == LOW && buttonConfig[1].setted) {
        sendButtonSignal(1);
  }
}
