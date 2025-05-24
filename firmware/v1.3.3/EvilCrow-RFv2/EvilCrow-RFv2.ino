#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include "MemoryManager.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include "esp_task_wdt.h"
#define DEST_FS_USES_SD
#include <ESP32-targz.h>
#include <SPIFFSEditor.h>
#include <EEPROM.h>
#include "SPIFFS.h"
#include "SPI.h"
#include <WiFiAP.h>
#include "FS.h"
#include "SD.h"

#define eepromsize 4096
#define samplesize 2000

// Replace static arrays with pointers
unsigned long *sample = nullptr;
unsigned long *samplesmooth = nullptr;
long *data_to_send = nullptr;
long *data_button1 = nullptr;
long *data_button2 = nullptr;
long *data_button3 = nullptr;
long *transmit_push = nullptr;

// Memory pool allocations
void allocateBuffers()
{
  sample = (unsigned long *)MEM_MGR.getSamplePool()->allocate(samplesize * sizeof(unsigned long));
  samplesmooth = (unsigned long *)MEM_MGR.getSamplePool()->allocate(samplesize * sizeof(unsigned long));
  data_to_send = (long *)MEM_MGR.getSamplePool()->allocate(2000 * sizeof(long));
  data_button1 = (long *)MEM_MGR.getSamplePool()->allocate(2000 * sizeof(long));
  data_button2 = (long *)MEM_MGR.getSamplePool()->allocate(2000 * sizeof(long));
  data_button3 = (long *)MEM_MGR.getSamplePool()->allocate(2000 * sizeof(long));
  transmit_push = (long *)MEM_MGR.getSamplePool()->allocate(2000 * sizeof(long));
}

void freeBuffers()
{
  if (sample)
    MEM_MGR.getSamplePool()->free(sample);
  if (samplesmooth)
    MEM_MGR.getSamplePool()->free(samplesmooth);
  if (data_to_send)
    MEM_MGR.getSamplePool()->free(data_to_send);
  if (data_button1)
    MEM_MGR.getSamplePool()->free(data_button1);
  if (data_button2)
    MEM_MGR.getSamplePool()->free(data_button2);
  if (data_button3)
    MEM_MGR.getSamplePool()->free(data_button3);
  if (transmit_push)
    MEM_MGR.getSamplePool()->free(transmit_push);

  sample = samplesmooth = nullptr;
  data_to_send = data_button1 = data_button2 = data_button3 = transmit_push = nullptr;
}

// Config SSID, password and channel
const char *ssid = "Evil Crow RF v2";     // Enter your SSID here
const char *password = "123456789ECRFv2"; // Enter your Password here
const int wifi_channel = 1;               // Enter your preferred Wi-Fi Channel

// HTML and CSS style
// const String MENU = "<body><p>Evil Crow RF v1.0</p><div id=\"header\"><body><nav id='menu'><input type='checkbox' id='responsive-menu' onclick='updatemenu()'><label></label><ul><li><a href='/'>Home</a></li><li><a class='dropdown-arrow'>Config</a><ul class='sub-menus'><li><a href='/txconfig'>RAW TX Config</a></li><li><a href='/txbinary'>Binary TX Config</a></li><li><a href='/rxconfig'>RAW RX Config</a></li><li><a href='/btnconfig'>Button TX Config</a></li></ul></li><li><a class='dropdown-arrow'>RX Log</a><ul class='sub-menus'><li><a href='/viewlog'>RX Logs</a></li><li><a href='/delete'>Delete Logs</a></li><li><a href='/downloadlog'>Download Logs</a></li><li><a href='/cleanspiffs'>Clean SPIFFS</a></li></ul></li><li><a class='dropdown-arrow'>URH Protocol</a><ul class='sub-menus'><li><a href='/txprotocol'>TX Protocol</a></li><li><a href='/listxmlfiles'>List Protocol</a></li><li><a href='/uploadxmlfiles'>Upload Protocol</a></li><li><a href='/cleanspiffs'>Clean SPIFFS</a></li></ul></li><li><a href='/jammer'>Simple Jammer</a></li><li><a href='/update'>OTA Update</a></li></ul></nav><br></div>";
const String HTML_CSS_STYLING = "<html><head><meta charset=\"utf-8\"><title>Evil Crow RF</title><link rel=\"stylesheet\" href=\"style.css\"><script src=\"lib.js\"></script></head>";

// Pushbutton Pins
int push1 = 34;
int push2 = 35;

int led = 32;
static unsigned long Blinktime = 0;

int error_toleranz = 200;

int RXPin = 26;
int RXPin0 = 4;
int TXPin0 = 2;
int Gdo0 = 25;
const int minsample = 30;
static unsigned long lastTime = 0;
String transmit = "";
String tmp_module;
String tmp_frequency;
String tmp_xmlname;
String tmp_codelen;
String tmp_setrxbw;
String tmp_mod;
int mod;
String tmp_deviation;
float deviation;
String tmp_datarate;
String tmp_powerjammer;
int power_jammer;
int datarate;
float frequency;
float setrxbw;
String raw_rx = "0";
String jammer_tx = "0";
const bool formatOnFail = true;
String webString;
String bindata;
int samplepulse;
String tmp_samplepulse;
String tmp_transmissions;
int counter = 0;
int pos = 0;
int transmissions;
int pushbutton1 = 0;
int pushbutton2 = 0;
byte jammer[11] = {
    0xff,
    0xff,
};

// BTN Sending Config
int btn_set_int;
String btn_set;
String btn1_frequency;
String btn1_mod;
String btn1_rawdata;
String btn1_deviation;
String btn1_transmission;
String btn2_frequency;
String btn2_mod;
String btn2_rawdata;
String btn2_deviation;
String btn2_transmission;
float tmp_btn1_deviation;
float tmp_btn2_deviation;
float tmp_btn1_frequency;
float tmp_btn2_frequency;
int tmp_btn1_mod;
int tmp_btn2_mod;
int tmp_btn1_transmission;
int tmp_btn2_transmission;
String bindataprotocol;
String bindata_protocol;
String btn1tesla = "0";
String btn2tesla = "0";
float tmp_btn1_tesla_frequency;
float tmp_btn2_tesla_frequency;

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

// ElegantOTA
unsigned long ota_progress_millis = 0;

const uint8_t sequence[messageLength] = {
    0x02, 0xAA, 0xAA, 0xAA, // Preamble of 26 bits by repeating 1010
    0x2B,                   // Sync byte
    0x2C, 0xCB, 0x33, 0x33, 0x2D, 0x34, 0xB5, 0x2B, 0x4D, 0x32, 0xAD, 0x2C, 0x56, 0x59, 0x96, 0x66,
    0x66, 0x5A, 0x69, 0x6A, 0x56, 0x9A, 0x65, 0x5A, 0x58, 0xAC, 0xB3, 0x2C, 0xCC, 0xCC, 0xB4, 0xD2,
    0xD4, 0xAD, 0x34, 0xCA, 0xB4, 0xA0};

// Jammer
int jammer_pin;

// File
File logs;
File file;

AsyncWebServer controlserver(80);

esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true};

void onOTAStart()
{
  Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final)
{
  if (millis() - ota_progress_millis > 1000)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success)
{
  if (success)
  {
    Serial.println("OTA update finished successfully!");
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
}

void readConfigWiFi(fs::FS &fs, String path)
{
  File file = fs.open(path);

  if (!file || file.isDirectory())
  {
    storage_status = 0;
    return;
  }

  while (file.available())
  {
    config_wifi = file.readString();
    int file_len = config_wifi.length() + 1;
    int index_config = config_wifi.indexOf('\n');
    tmp_config1 = config_wifi.substring(0, index_config - 1);

    tmp_config2 = config_wifi.substring(index_config + 1, file_len - 3);
    storage_status = 1;
  }
  file.close();
}

void writeConfigWiFi(fs::FS &fs, const char *path, String message)
{
  File file = fs.open(path, FILE_APPEND);

  if (!file)
  {
    return;
  }

  if (file.println(message))
  {
  }
  else
  {
  }
  file.close();
}

// handles uploads
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  if (!index)
  {
    logmessage = "Upload Start: " + String(filename);
    request->_tempFile = SD.open("/URH/" + filename, "w");
  }

  if (len)
  {
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
  }

  if (final)
  {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    request->_tempFile.close();
    request->redirect("/");
  }
}

// handles uploads SD Files
void handleUploadSD(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  removeDir(SD, "/HTML");
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();

  if (!index)
  {
    logmessage = "Upload Start: " + String(filename);
    request->_tempFile = SD.open("/" + filename, "w");
  }

  if (len)
  {
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
  }

  if (final)
  {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    request->_tempFile.close();
    tarGzFS.begin();

    TarUnpacker *TARUnpacker = new TarUnpacker();

    TARUnpacker->haltOnError(true);                                                            // stop on fail (manual restart/reset required)
    TARUnpacker->setTarVerify(true);                                                           // true = enables health checks but slows down the overall process
    TARUnpacker->setupFSCallbacks(targzTotalBytesFn, targzFreeBytesFn);                        // prevent the partition from exploding, recommended
    TARUnpacker->setTarProgressCallback(BaseUnpacker::defaultProgressCallback);                // prints the untarring progress for each individual file
    TARUnpacker->setTarStatusProgressCallback(BaseUnpacker::defaultTarStatusProgressCallback); // print the filenames as they're expanded
    TARUnpacker->setTarMessageCallback(BaseUnpacker::targzPrintLoggerCallback);                // tar log verbosity

    if (!TARUnpacker->tarExpander(tarGzFS, "/HTML.tar", tarGzFS, "/"))
    {
      Serial.printf("tarExpander failed with return code #%d\n", TARUnpacker->tarGzGetError());
    }
    deleteFile(SD, "/HTML.tar");
    request->redirect("/");
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  deleteFile(SD, "/dir.txt");

  File root = fs.open(dirname);
  if (!root)
  {
    return;
  }
  if (!root.isDirectory())
  {
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      appendFile(SD, "/dir.txt", "  DIR : ", file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      appendFile(SD, "/dir.txt", "", "<br>");
      appendFile(SD, "/dir.txt", "", file.name());
      appendFile(SD, "/dir.txt", "  SIZE: ", "");
      appendFileLong(SD, "/dir.txt", file.size());
    }
    file = root.openNextFile();
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message, String messagestring)
{

  logs = fs.open(path, FILE_APPEND);
  if (!logs)
  {
    // Serial.println("Failed to open file for appending");
    return;
  }
  if (logs.print(message) | logs.print(messagestring))
  {
    // Serial.println("Message appended");
  }
  else
  {
    // Serial.println("Append failed");
  }
  logs.close();
}

void appendFileLong(fs::FS &fs, const char *path, unsigned long messagechar)
{
  // Serial.printf("Appending to file: %s\n", path);

  logs = fs.open(path, FILE_APPEND);
  if (!logs)
  {
    // Serial.println("Failed to open file for appending");
    return;
  }
  if (logs.print(messagechar))
  {
    // Serial.println("Message appended");
  }
  else
  {
    // Serial.println("Append failed");
  }
  logs.close();
}

void deleteFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    // Serial.println("File deleted");
  }
  else
  {
    // Serial.println("Delete failed");
  }
}

void readFile(fs::FS &fs, String path)
{
  // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    // Serial.println("Failed to open file for reading");
    return;
  }

  // Serial.print("Read from file: ");
  while (file.available())
  {
    bindataprotocol = file.readString();
    // Serial.println("");
    // Serial.println(bindataprotocol);
  }
  file.close();
}

void removeDir(fs::FS &fs, const char *dirname)
{

  File root = fs.open(dirname);
  if (!root)
  {
    // Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    // Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    // Serial.print(file.name());
    // Serial.println("");

    if (file.isDirectory())
    {
      deleteFile(SD, file.name());
    }
    else
    {
      deleteFile(SD, file.name());
    }
    file = root.openNextFile();
  }
}

bool checkReceived(void)
{

  delay(1);
  if (samplecount >= minsample && micros() - lastTime > 100000)
  {
    detachInterrupt(RXPin0);
    detachInterrupt(RXPin);
    return 1;
  }
  else
  {
    return 0;
  }
}

void printReceived()
{

  Serial.print("Count=");
  Serial.println(samplecount);
  appendFile(SD, "/logs.txt", NULL, "<br>\n");
  appendFile(SD, "/logs.txt", NULL, "Count=");
  appendFileLong(SD, "/logs.txt", samplecount);
  appendFile(SD, "/logs.txt", NULL, "<br>");

  for (int i = 1; i < samplecount; i++)
  {
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

RECEIVE_ATTR void handleReceive()
{
  // Check if buffers are allocated
  if (!sample || !samplesmooth)
  {
    log_e("RF buffers not allocated");
    return;
  }

  const unsigned long time = micros();
  const unsigned long duration = time - lastTime;

  // Ignore if duration is too short
  if (duration < 100)
    return;

  // Store the sample with bounds checking
  if (samplecount < samplesize)
  {
    sample[samplecount++] = duration;
  }

  lastTime = time;
}

bool processSamples()
{
  if (!sample || !samplesmooth || samplecount < minsample)
  {
    log_e("Invalid sample buffer state");
    return false;
  }

  // Calculate average
  unsigned long average = 0;
  for (int i = 0; i < samplecount; i++)
  {
    average += sample[i];
  }
  average /= samplecount;

  // Process samples with error tolerance
  int smoothcount = 0;
  for (int i = 0; i < samplecount; i++)
  {
    if (abs(int(sample[i] - average)) < error_toleranz)
    {
      samplesmooth[smoothcount++] = sample[i];
    }
  }

  // Check if we have enough valid samples
  if (smoothcount < minsample)
  {
    log_w("Not enough valid samples: %d", smoothcount);
    return false;
  }

  // Process the data...

  return true;
}

void clearSamples()
{
  samplecount = 0;
  if (sample)
    memset(sample, 0, samplesize * sizeof(unsigned long));
  if (samplesmooth)
    memset(samplesmooth, 0, samplesize * sizeof(unsigned long));
}

void cleanupRF()
{
  // Cleanup RF-related resources
  clearSamples();

  // Force memory cleanup if fragmentation is high
  if (MEM_MGR.getFragmentation() > 70)
  {
    MEM_MGR.defragment();
  }
}

void RECEIVE_ATTR receiver()
{
  const long time = micros();
  const unsigned int duration = time - lastTime;

  if (duration > 100000)
  {
    samplecount = 0;
  }

  if (duration >= 100)
  {
    sample[samplecount++] = duration;
  }

  if (samplecount >= samplesize)
  {
    detachInterrupt(RXPin0);
    detachInterrupt(RXPin);
    checkReceived();
  }

  if (mod == 0 && tmp_module == "2")
  {
    if (samplecount == 1 and digitalRead(RXPin) != HIGH)
    {
      samplecount = 0;
    }
  }

  lastTime = time;
}

void enableReceive()
{
  pinMode(RXPin0, INPUT);
  RXPin0 = digitalPinToInterrupt(RXPin0);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(RXPin0, receiver, CHANGE);
  pinMode(RXPin, INPUT);
  RXPin = digitalPinToInterrupt(RXPin);
  ELECHOUSE_cc1101.SetRx();
  samplecount = 0;
  attachInterrupt(RXPin, receiver, CHANGE);
}

void parse_data()
{

  bindata_protocol = "";
  int data_begin_bits = 0;
  int data_end_bits = 0;
  int data_begin_pause = 0;
  int data_end_pause = 0;
  int data_count = 0;

  for (int c = 0; c < bindataprotocol.length(); c++)
  {
    if (bindataprotocol.substring(c, c + 4) == "bits")
    {
      data_count++;
    }
  }

  for (int d = 0; d < data_count; d++)
  {
    data_begin_bits = bindataprotocol.indexOf("<message bits=", data_end_bits);
    data_end_bits = bindataprotocol.indexOf("decoding_index=", data_begin_bits + 1);
    bindata_protocol += bindataprotocol.substring(data_begin_bits + 15, data_end_bits - 2);

    data_begin_pause = bindataprotocol.indexOf("pause=", data_end_pause);
    data_end_pause = bindataprotocol.indexOf(" timestamp=", data_begin_pause + 1);
    bindata_protocol += "[Pause: ";
    bindata_protocol += bindataprotocol.substring(data_begin_pause + 7, data_end_pause - 1);
    bindata_protocol += " samples]\n";
  }
  bindata_protocol.replace(" ", "");
  bindata_protocol.replace("\n", "");
  bindata_protocol.replace("Pause:", "");
  Serial.println("Parsed Data:");
  Serial.println(bindata_protocol);
}

void sendSignals()
{
  pinMode(TXPin0, OUTPUT);
  ELECHOUSE_cc1101.setModul(0);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(frequency);
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.SetTx();

  for (uint8_t t = 0; t < transmtesla; t++)
  {
    for (uint8_t i = 0; i < messageLength; i++)
      sendByte(sequence[i]);
    digitalWrite(TXPin0, LOW);
    delay(messageDistance);
  }
}

void sendSignalsBT1()
{
  pinMode(TXPin0, OUTPUT);
  ELECHOUSE_cc1101.setModul(0);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(tmp_btn1_tesla_frequency);
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.SetTx();

  for (uint8_t t = 0; t < transmtesla; t++)
  {
    for (uint8_t i = 0; i < messageLength; i++)
      sendByte(sequence[i]);
    digitalWrite(TXPin0, LOW);
    delay(messageDistance);
  }
}

void sendSignalsBT2()
{
  pinMode(TXPin0, OUTPUT);
  ELECHOUSE_cc1101.setModul(0);
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setModulation(2);
  ELECHOUSE_cc1101.setMHZ(tmp_btn2_tesla_frequency);
  ELECHOUSE_cc1101.setDeviation(0);
  ELECHOUSE_cc1101.SetTx();

  for (uint8_t t = 0; t < transmtesla; t++)
  {
    for (uint8_t i = 0; i < messageLength; i++)
      sendByte(sequence[i]);
    digitalWrite(TXPin0, LOW);
    delay(messageDistance);
  }
}

void sendByte(uint8_t dataByte)
{
  for (int8_t bit = 7; bit >= 0; bit--)
  { // MSB
    digitalWrite(TXPin0, (dataByte & (1 << bit)) != 0 ? HIGH : LOW);
    delayMicroseconds(pulseWidth);
  }
}

void power_management()
{
  EEPROM.begin(eepromsize);

  pinMode(push2, INPUT);
  pinMode(led, OUTPUT);

  byte z = EEPROM.read(eepromsize - 2);
  if (digitalRead(push2) != LOW)
  {
    if (z == 1)
    {
      go_deep_sleep();
    }
  }
  else
  {
    if (z == 0)
    {
      EEPROM.write(eepromsize - 2, 1);
      EEPROM.commit();
      go_deep_sleep();
    }
    else
    {
      EEPROM.write(eepromsize - 2, 0);
      EEPROM.commit();
    }
  }
}

void go_deep_sleep()
{
  Serial.println("Going to sleep now");
  ELECHOUSE_cc1101.setModul(0);
  ELECHOUSE_cc1101.goSleep();
  ELECHOUSE_cc1101.setModul(1);
  ELECHOUSE_cc1101.goSleep();
  led_blink(5, 250);
  esp_deep_sleep_start();
}

void led_blink(int blinkrep, int blinktimer)
{
  for (int i = 0; i < blinkrep; i++)
  {
    digitalWrite(led, HIGH);
    delay(blinktimer);
    digitalWrite(led, LOW);
    delay(blinktimer);
  }
}

void poweron_blink()
{
  if (millis() - Blinktime > 10000)
  {
    digitalWrite(led, LOW);
  }
  if (millis() - Blinktime > 10100)
  {
    digitalWrite(led, HIGH);
    Blinktime = millis();
  }
}

void force_reset()
{
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);
  while (true)
    ;
}

void setup()
{
  Serial.begin(115200);

  // Initialize memory management system
  if (!MEM_MGR.init())
  {
    log_e("Failed to initialize memory management system");
    while (1)
      delay(1000); // Fatal error
  }

  // Initialize web request handler
  WEB_HANDLER.init();

  // Allocate memory buffers
  allocateBuffers();
  if (!sample || !samplesmooth || !data_to_send || !data_button1 || !data_button2 || !data_button3 || !transmit_push)
  {
    log_e("Failed to allocate memory buffers");
    while (1)
      delay(1000); // Fatal error
  }

  // Initialize WiFi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, wifi_channel);

  // Initialize filesystem
  SPIFFS.begin(true);
  sdspi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_SS);
  SD.begin(SD_SS, sdspi);

  // Initialize pins
  pinMode(push1, INPUT);
  pinMode(push2, INPUT);

  // Setup web routes with memory-managed handlers
  controlserver.on("/settx", HTTP_POST, [](AsyncWebServerRequest *request)
                   {
        raw_rx = "0"; // Keep legacy state variable for now
        WEB_HANDLER.handleTxRequest(request); });

  controlserver.on("/setrx", HTTP_POST, [](AsyncWebServerRequest *request)
                   { WEB_HANDLER.handleRxRequest(request); });

  controlserver.on("/settxbinary", HTTP_POST, [](AsyncWebServerRequest *request)
                   {
        raw_rx = "0"; // Keep legacy state variable for now
        WEB_HANDLER.handleBinaryRequest(request); });

  controlserver.on("/settxtesla", HTTP_POST, [](AsyncWebServerRequest *request)
                   {
        raw_rx = "0"; // Keep legacy state variable for now
        WEB_HANDLER.handleTeslaRequest(request); });

  controlserver.on("/setjammer", HTTP_POST, [](AsyncWebServerRequest *request)
                   {
        raw_rx = "0"; // Keep legacy state variable for now
        WEB_HANDLER.handleJammerRequest(request); });

  // Static file handlers remain unchanged
  controlserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(SD, "/HTML/index.html", "text/html"); });

  controlserver.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(SD, "/HTML/style.css", "text/css"); });

  controlserver.on("/lib.js", HTTP_GET, [](AsyncWebServerRequest *request)
                   { request->send(SD, "/HTML/javascript.js", "text/javascript"); });

  // Initialize the server
  controlserver.begin();

  // Initialize CC1101 radio
  ELECHOUSE_cc1101.addSpiPin(14, 12, 13, 5, 0);
  ELECHOUSE_cc1101.addSpiPin(14, 12, 13, 27, 1);

  // Print initial memory stats
  MEM_MGR.printStats();
}

// Update loop to handle request processing
void loop()
{
  static unsigned long lastStats = 0;
  unsigned long now = millis();

  // Print memory stats every 30 seconds
  if (now - lastStats > 30000)
  {
    MEM_MGR.printStats();
    lastStats = now;
  }

  // Rest of the loop code...
  ElegantOTA.loop();
  poweron_blink();

  pushbutton1 = digitalRead(push1);
  pushbutton2 = digitalRead(push2);

  if (raw_rx == "1")
  {
    if (checkReceived())
    {
      printReceived();
      signalanalyse();
      enableReceive();
      delay(200);
      // sdspi.end();
      // sdspi.begin(18, 19, 23, 22);
      // SD.begin(22, sdspi);
      delay(500);
    }
  }

  if (jammer_tx == "1")
  {
    raw_rx = "0";

    if (tmp_module == "1")
    {
      for (int i = 0; i < 12; i += 2)
      {
        digitalWrite(2, HIGH);
        delayMicroseconds(jammer[i]);
        digitalWrite(2, LOW);
        delayMicroseconds(jammer[i + 1]);
      }
    }
    else if (tmp_module == "2")
    {
      for (int i = 0; i < 12; i += 2)
      {
        digitalWrite(25, HIGH);
        delayMicroseconds(jammer[i]);
        digitalWrite(25, LOW);
        delayMicroseconds(jammer[i + 1]);
      }
    }
  }

  if (pushbutton1 == LOW)
  {
    if (btn1tesla == "1")
    {
      sendSignalsBT1();
    }
    else
    {
      raw_rx = "0";
      tmp_btn1_deviation = btn1_deviation.toFloat();
      tmp_btn1_mod = btn1_mod.toInt();
      tmp_btn1_frequency = btn1_frequency.toFloat();
      tmp_btn1_transmission = btn1_transmission.toInt();
      pinMode(25, OUTPUT);
      ELECHOUSE_cc1101.setModul(1);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(tmp_btn1_mod);
      ELECHOUSE_cc1101.setMHZ(tmp_btn1_frequency);
      ELECHOUSE_cc1101.setDeviation(tmp_btn1_deviation);
      // delay(400);
      ELECHOUSE_cc1101.SetTx();

      for (int r = 0; r < tmp_btn1_transmission; r++)
      {
        for (int i = 0; i < counter; i += 2)
        {
          digitalWrite(25, HIGH);
          delayMicroseconds(data_button1[i]);
          digitalWrite(25, LOW);
          delayMicroseconds(data_button1[i + 1]);
          Serial.print(data_button1[i]);
          Serial.print(",");
        }
        delay(2000); // Set this for the delay between retransmissions
      }
      Serial.println();
      ELECHOUSE_cc1101.setSidle();
    }
  }

  if (pushbutton2 == LOW)
  {
    if (btn2tesla == "1")
    {
      sendSignalsBT2();
    }
    else
    {
      raw_rx = "0";
      tmp_btn2_deviation = btn2_deviation.toFloat();
      tmp_btn2_mod = btn2_mod.toInt();
      tmp_btn2_frequency = btn2_frequency.toFloat();
      tmp_btn2_transmission = btn2_transmission.toInt();
      pinMode(25, OUTPUT);
      ELECHOUSE_cc1101.setModul(1);
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setModulation(tmp_btn2_mod);
      ELECHOUSE_cc1101.setMHZ(tmp_btn2_frequency);
      ELECHOUSE_cc1101.setDeviation(tmp_btn2_deviation);
      ELECHOUSE_cc1101.SetTx();

      for (int r = 0; r < tmp_btn2_transmission; r++)
      {
        for (int i = 0; i < counter; i += 2)
        {
          digitalWrite(25, HIGH);
          delayMicroseconds(data_button2[i]);
          digitalWrite(25, LOW);
          delayMicroseconds(data_button2[i + 1]);
          Serial.print(data_button2[i]);
          Serial.print(",");
        }
        delay(2000); // Set this for the delay between retransmissions
      }
      Serial.println();
      ELECHOUSE_cc1101.setSidle();
    }
  }
}
