#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include "MemoryManager.h"
#include "WebRequestHandler.h"
#include "RFSignalProcessor.h"
#include "AttackManager.h"
#include <WiFiClient.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
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

// Replace global arrays with RF processor instance
// The RF processor manages its own memory

// Memory pool allocations
void allocateBuffers()
{
}

void freeBuffers()
{
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
  const unsigned long time = micros();
  const unsigned long duration = time - lastTime;

  RF_PROCESSOR.processPulse(duration);
  lastTime = time;
}

bool processReceivedSignal()
{
  if (!RF_PROCESSOR.smoothSignal())
  {
    log_w("Failed to smooth signal");
    return false;
  }

  // Calculate signal quality
  float quality = RF_PROCESSOR.calculateSignalQuality();
  log_i("Signal quality: %.1f%%", quality);

  // Analyze spectral properties
  RFSignalProcessor::SpectralInfo spectrum;
  if (RF_PROCESSOR.analyzeSpectrum(spectrum))
  {
    log_i("Spectral Analysis:");
    log_i("  Dominant Freq: %.1f Hz", spectrum.dominantFreq);
    log_i("  SNR: %.1f dB", spectrum.signalToNoise);
    log_i("  Bandwidth: %.1f Hz", spectrum.bandwidth);
    log_i("  Symbol Rate: %u baud", spectrum.symbolRate);
  }

  // Detect patterns
  RFSignalProcessor::PatternInfo pattern;
  if (RF_PROCESSOR.detectPattern(pattern))
  {
    log_i("Pattern Analysis:");
    log_i("  Length: %u bits", pattern.length);
    log_i("  Repeats: %u", pattern.repeats);
    log_i("  Confidence: %.1f%%", pattern.confidence * 100);
  }

  // Identify protocol
  RFSignalProcessor::ProtocolInfo protocol;
  if (RF_PROCESSOR.identifyProtocol(protocol))
  {
    log_i("Protocol Identified: %s", protocol.name);
    log_i("  Base Unit: %u us", protocol.baseUnit);
    log_i("  Min Repeats: %u", protocol.minRepeats);
  }

  // Compress and analyze pulses
  if (RF_PROCESSOR.compressSignal())
  {
    uint32_t period = 0, zeroPulse = 0, onePulse = 0;
    if (RF_PROCESSOR.analyzePulses(period, zeroPulse, onePulse))
    {
      log_i("Pulse Analysis:");
      log_i("  Period: %u us", period);
      log_i("  Zero Pulse: %u us", zeroPulse);
      log_i("  One Pulse: %u us", onePulse);
    }
  }

  return true;
}

void transmitSignal(const long *timings, size_t count, int repetitions)
{
  RF_PROCESSOR.transmitRaw(timings, count, repetitions);
}

void setup()
{
  Serial.begin(115200);

  // Initialize systems in order
  if (!MEM_MGR.init())
  {
    log_e("Failed to initialize memory management");
    while (1)
      delay(1000);
  }

  if (!RF_PROCESSOR.init())
  {
    log_e("Failed to initialize RF processor");
    while (1)
      delay(1000);
  }

  // Initialize web handler after memory system
  WEB_HANDLER.init();

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

  // Add attack routes
  addAttackRoutes(controlserver);

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

  // Button handling with RF processor
  if (digitalRead(push1) == HIGH && btn1tesla == "1")
  {
    float freq = rfConfig.frequency;
    ELECHOUSE_cc1101.setModul(0);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setMHZ(freq);
    ELECHOUSE_cc1101.setPA(12);
    sendSignals();
  }

  // Rest of the loop code...
}
