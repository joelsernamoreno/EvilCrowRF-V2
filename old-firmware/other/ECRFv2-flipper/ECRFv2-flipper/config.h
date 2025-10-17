#define DEST_FS_USES_SD

#define eepromsize 4096
#define samplesize 2000

#if defined(ESP8266)
    #define RECEIVE_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
    #define RECEIVE_ATTR IRAM_ATTR
#else
    #define RECEIVE_ATTR
#endif

#define MIN_SAMPLE 30
#define ERROR_TOLERANZ 200
#define BUFFER_MAX_SIZE 2000
#define FORMAT_ON_FAIL true

#define FZ_SUB_MAX_SIZE 4096  // should be suficient
#define MAX_LINE_SIZE 4096
#define JSON_DOC_SIZE 4096

#define SERIAL_BAUDRATE 38400
#define DELAY_BETWEEN_RETRANSMISSIONS 2000

#define WEB_SERVER_PORT 80

/* I/O */
// SPI devices
#define SD_SCLK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_SS   22

#define CC1101_SCK  14
#define CC1101_MISO 12
#define CC1101_MOSI 13
#define CC1101_SS0   5 
#define CC1101_SS1 27
#define MOD0_GDO0 2
#define MOD0_GDO2 4
#define MOD1_GDO0 25
#define MOD1_GDO2 26

// Buttons and led
#define LED 32
#define BUTTON1 34
#define BUTTON2 35

/* HTTP response codes */
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_NOT_IMPLEMENTED 501
