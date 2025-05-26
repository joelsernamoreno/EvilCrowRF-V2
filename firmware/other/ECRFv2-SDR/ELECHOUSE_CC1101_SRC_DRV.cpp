#include "ELECHOUSE_CC1101_SRC_DRV.h"
#include <SPI.h>

#define WRITE_BURST 0x40
#define READ_SINGLE 0x80
#define READ_BURST 0xC0
#define BYTES_IN_RXFIFO 0x7F
#define max_modul 6

byte modulation = 2;
byte frend0;
byte chan = 0;
int pa = 12;
byte last_pa;
byte SCK_PIN;
byte MISO_PIN;
byte MOSI_PIN;
byte SS_PIN;
byte GDO0;
byte GDO2;
byte SCK_PIN_M[max_modul];
byte MISO_PIN_M[max_modul];
byte MOSI_PIN_M[max_modul];
byte SS_PIN_M[max_modul];
byte gdo0_M[max_modul];
byte gdo2_M[max_modul];
byte Modul_Status[max_modul];

SPIClass *CC1101_spi;

ELECHOUSE_CC1101 ELECHOUSE_cc1101;

// Implementation of ELECHOUSE_CC1101 class methods
void ELECHOUSE_CC1101::Init(void)
{
    setSpi();
    SpiStart(); // Use original SPI initialization
    digitalWrite(SS_PIN, HIGH);
    digitalWrite(SCK_PIN, HIGH);
    digitalWrite(MOSI_PIN, LOW);
    Reset();             // CC1101 reset
    RegConfigSettings(); // CC1101 register config
    SpiEnd();
}

void ELECHOUSE_CC1101::setSpiPin(byte sck, byte miso, byte mosi, byte ss)
{
    SCK_PIN = sck;
    MISO_PIN = miso;
    MOSI_PIN = mosi;
    SS_PIN = ss;
}

void ELECHOUSE_CC1101::setGDO(byte gdo0, byte gdo2)
{
    GDO0 = gdo0;
    GDO2 = gdo2;
    pinMode(GDO0, INPUT);
    pinMode(GDO2, INPUT);
}

void ELECHOUSE_CC1101::Reset(void)
{
    digitalWrite(SS_PIN, LOW);
    delay(1);
    digitalWrite(SS_PIN, HIGH);
    delay(1);
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    CC1101_spi->transfer(CC1101_SRES);
    while (digitalRead(MISO_PIN))
        ;
    digitalWrite(SS_PIN, HIGH);
}

void ELECHOUSE_CC1101::SpiInit(void)
{
    pinMode(SCK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(SS_PIN, OUTPUT);
}

void ELECHOUSE_CC1101::RegConfigSettings(void)
{
    // Default register settings for optimal RF performance
    SpiWriteReg(CC1101_IOCFG2, 0x0B);   // GDO2 Output Pin Configuration
    SpiWriteReg(CC1101_IOCFG0, 0x06);   // GDO0 Output Pin Configuration
    SpiWriteReg(CC1101_PKTLEN, 0xFF);   // Packet Length
    SpiWriteReg(CC1101_PKTCTRL1, 0x04); // Packet Automation Control
    SpiWriteReg(CC1101_PKTCTRL0, 0x05); // Packet Automation Control
    SpiWriteReg(CC1101_ADDR, 0x00);     // Device Address
    SpiWriteReg(CC1101_CHANNR, 0x00);   // Channel Number
    SpiWriteReg(CC1101_FSCTRL1, 0x0B);  // Frequency Synthesizer Control
    SpiWriteReg(CC1101_FSCTRL0, 0x00);  // Frequency Synthesizer Control
    SpiWriteReg(CC1101_MDMCFG4, 0xF8);  // Modem Configuration
    SpiWriteReg(CC1101_MDMCFG3, 0x83);  // Modem Configuration
    SpiWriteReg(CC1101_MDMCFG2, 0x03);  // Modem Configuration
    SpiWriteReg(CC1101_MDMCFG1, 0x22);  // Modem Configuration
    SpiWriteReg(CC1101_MDMCFG0, 0xF8);  // Modem Configuration
    SpiWriteReg(CC1101_DEVIATN, 0x44);  // Modem Deviation Setting
    SpiWriteReg(CC1101_MCSM2, 0x07);    // Main Radio Control State Machine Configuration
    SpiWriteReg(CC1101_MCSM1, 0x30);    // Main Radio Control State Machine Configuration
    SpiWriteReg(CC1101_MCSM0, 0x18);    // Main Radio Control State Machine Configuration
    SpiWriteReg(CC1101_FOCCFG, 0x16);   // Frequency Offset Compensation Configuration
    SpiWriteReg(CC1101_BSCFG, 0x6C);    // Bit Synchronization Configuration
    SpiWriteReg(CC1101_AGCCTRL2, 0x43); // AGC Control
    SpiWriteReg(CC1101_AGCCTRL1, 0x40); // AGC Control
    SpiWriteReg(CC1101_AGCCTRL0, 0x91); // AGC Control
    SpiWriteReg(CC1101_WOREVT1, 0x87);  // High Byte Event0 Timeout
    SpiWriteReg(CC1101_WOREVT0, 0x6B);  // Low Byte Event0 Timeout
    SpiWriteReg(CC1101_WORCTRL, 0xFB);  // Wake On Radio Control
    SpiWriteReg(CC1101_FREND1, 0x56);   // Front End RX Configuration
    SpiWriteReg(CC1101_FREND0, 0x10);   // Front End TX Configuration
    SpiWriteReg(CC1101_FSCAL3, 0xE9);   // Frequency Synthesizer Calibration
    SpiWriteReg(CC1101_FSCAL2, 0x2A);   // Frequency Synthesizer Calibration
    SpiWriteReg(CC1101_FSCAL1, 0x00);   // Frequency Synthesizer Calibration
    SpiWriteReg(CC1101_FSCAL0, 0x1F);   // Frequency Synthesizer Calibration
    SpiWriteReg(CC1101_RCCTRL1, 0x41);  // RC Oscillator Configuration
    SpiWriteReg(CC1101_RCCTRL0, 0x00);  // RC Oscillator Configuration
    SpiWriteReg(CC1101_FSTEST, 0x59);   // Frequency Synthesizer Calibration Control
    SpiWriteReg(CC1101_PTEST, 0x7F);    // Production Test
    SpiWriteReg(CC1101_AGCTEST, 0x3F);  // AGC Test
    SpiWriteReg(CC1101_TEST2, 0x81);    // Various Test Settings
    SpiWriteReg(CC1101_TEST1, 0x35);    // Various Test Settings
    SpiWriteReg(CC1101_TEST0, 0x09);    // Various Test Settings
}

void ELECHOUSE_CC1101::setMHZ(float mhz)
{
    byte freq2 = 0;
    byte freq1 = 0;
    byte freq0 = 0;

    // Convert frequency to channel number
    float c = 26;
    float temp = mhz * c;
    freq2 = (byte)(temp / 65536);
    temp -= (freq2 * 65536);
    freq1 = (byte)(temp / 256);
    temp -= (freq1 * 256);
    freq0 = (byte)temp;

    SpiWriteReg(CC1101_FREQ2, freq2);
    SpiWriteReg(CC1101_FREQ1, freq1);
    SpiWriteReg(CC1101_FREQ0, freq0);
}

void ELECHOUSE_CC1101::sendData(byte *txBuffer, byte size)
{
    SpiWriteReg(CC1101_TXFIFO, size);
    SpiWriteBurstReg(CC1101_TXFIFO, txBuffer, size);
    SpiStrobe(CC1101_STX);
    while (!digitalRead(GDO0))
        ;
    while (digitalRead(GDO0))
        ;
    SpiStrobe(CC1101_SFTX);
}

byte ELECHOUSE_CC1101::receive(void)
{
    byte size;
    byte status[2];

    if (digitalRead(GDO0))
    {
        while (digitalRead(GDO0))
            ;
        size = SpiReadStatus(CC1101_RXBYTES);
        if (size & BYTES_IN_RXFIFO)
        {
            size = SpiReadReg(CC1101_RXFIFO);
            SpiReadBurstReg(CC1101_RXFIFO, status, 2);
            SpiReadBurstReg(CC1101_RXFIFO, rxBuffer, size);
            SpiStrobe(CC1101_SFRX);
            return size;
        }
        else
        {
            SpiStrobe(CC1101_SFRX);
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

// Helper functions
void ELECHOUSE_CC1101::SpiWriteReg(byte addr, byte value)
{
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    CC1101_spi->transfer(addr);
    CC1101_spi->transfer(value);
    digitalWrite(SS_PIN, HIGH);
}

byte ELECHOUSE_CC1101::SpiReadReg(byte addr)
{
    byte temp;
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    CC1101_spi->transfer(addr | READ_SINGLE);
    temp = CC1101_spi->transfer(0);
    digitalWrite(SS_PIN, HIGH);
    return temp;
}

void ELECHOUSE_CC1101::SpiWriteBurstReg(byte addr, byte *buffer, byte num)
{
    byte i, temp;
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    temp = addr | WRITE_BURST;
    CC1101_spi->transfer(temp);
    for (i = 0; i < num; i++)
    {
        CC1101_spi->transfer(buffer[i]);
    }
    digitalWrite(SS_PIN, HIGH);
}

void ELECHOUSE_CC1101::SpiReadBurstReg(byte addr, byte *buffer, byte num)
{
    byte i, temp;
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    temp = addr | READ_BURST;
    CC1101_spi->transfer(temp);
    for (i = 0; i < num; i++)
    {
        buffer[i] = CC1101_spi->transfer(0);
    }
    digitalWrite(SS_PIN, HIGH);
}

byte ELECHOUSE_CC1101::SpiReadStatus(byte addr)
{
    byte temp;
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    CC1101_spi->transfer(addr | READ_BURST);
    temp = CC1101_spi->transfer(0);
    digitalWrite(SS_PIN, HIGH);
    return temp;
}

void ELECHOUSE_CC1101::SpiStrobe(byte strobe)
{
    digitalWrite(SS_PIN, LOW);
    while (digitalRead(MISO_PIN))
        ;
    CC1101_spi->transfer(strobe);
    digitalWrite(SS_PIN, HIGH);
}

// Missing function implementations
void ELECHOUSE_CC1101::setSpi(void)
{
    CC1101_spi = &SPI;
    // EvilCrow hardware uses different SPI pins for CC1101
    SCK_PIN = 14;  // CC1101_SCK
    MISO_PIN = 12; // CC1101_MISO
    MOSI_PIN = 13; // CC1101_MOSI
    SS_PIN = 15;   // CC1101_CS - This is the correct pin!
    GDO0 = 4;      // CC1101_GD0
    GDO2 = 16;     // CC1101_GD1 (used as GDO2)
}

void ELECHOUSE_CC1101::SpiStart(void)
{
    // Initialize the SPI pins (like original firmware)
    pinMode(SCK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    pinMode(SS_PIN, OUTPUT);

// Enable SPI with custom pins (like original firmware)
#ifdef ESP32
    CC1101_spi->begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
#else
    CC1101_spi->begin();
#endif
}

void ELECHOUSE_CC1101::SpiEnd(void)
{
    CC1101_spi->end();
}

byte ELECHOUSE_CC1101::getCC1101_PARTNUM(void)
{
    return SpiReadStatus(CC1101_PARTNUM);
}

byte ELECHOUSE_CC1101::getCC1101_VERSION(void)
{
    return SpiReadStatus(CC1101_VERSION);
}

byte ELECHOUSE_CC1101::getCC1101_MARCSTATE(void)
{
    return SpiReadStatus(CC1101_MARCSTATE);
}

byte ELECHOUSE_CC1101::getCC1101_RXBYTES(void)
{
    return SpiReadStatus(CC1101_RXBYTES);
}

byte ELECHOUSE_CC1101::getCC1101_TXBYTES(void)
{
    return SpiReadStatus(CC1101_TXBYTES);
}

byte ELECHOUSE_CC1101::getCC1101_STATUS(void)
{
    return SpiReadStatus(CC1101_MARCSTATE);
}

byte ELECHOUSE_CC1101::Check_RXFIFO(void)
{
    return SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO;
}

byte ELECHOUSE_CC1101::Check_TXFIFO(void)
{
    return SpiReadStatus(CC1101_TXBYTES) & BYTES_IN_RXFIFO;
}

byte ELECHOUSE_CC1101::Bytes_In_RXFIFO(void)
{
    return SpiReadStatus(CC1101_RXBYTES) & BYTES_IN_RXFIFO;
}

byte ELECHOUSE_CC1101::Bytes_In_TXFIFO(void)
{
    return SpiReadStatus(CC1101_TXBYTES) & BYTES_IN_RXFIFO;
}

void ELECHOUSE_CC1101::TXFIFO_Flush(void)
{
    SpiStrobe(CC1101_SFTX);
}

void ELECHOUSE_CC1101::RXFIFO_Flush(void)
{
    SpiStrobe(CC1101_SFRX);
}

void ELECHOUSE_CC1101::receiveData(byte *rxBuffer, byte size)
{
    byte bytesInFifo = Bytes_In_RXFIFO();
    if (bytesInFifo > 0)
    {
        byte actualSize = (bytesInFifo < size) ? bytesInFifo : size;
        SpiReadBurstReg(CC1101_RXFIFO, rxBuffer, actualSize);
    }
}

// Additional helper functions for compatibility
void ELECHOUSE_CC1101::setModulation(byte m) { modulation = m; }
void ELECHOUSE_CC1101::setPA(int p) { pa = p; }
void ELECHOUSE_CC1101::setCCMode(bool s) { /* Not implemented */ }
void ELECHOUSE_CC1101::setSyncMode(byte s) { /* Not implemented */ }
void ELECHOUSE_CC1101::setAddr(byte s) { SpiWriteReg(CC1101_ADDR, s); }
void ELECHOUSE_CC1101::setChannel(byte chnl) { SpiWriteReg(CC1101_CHANNR, chnl); }
void ELECHOUSE_CC1101::setChsp(float f) { /* Not implemented */ }
void ELECHOUSE_CC1101::setRxBW(float f) { /* Not implemented */ }
void ELECHOUSE_CC1101::setDRate(float d) { /* Not implemented */ }
void ELECHOUSE_CC1101::setDeviation(float d) { /* Not implemented */ }
void ELECHOUSE_CC1101::setFilterLength(byte f) { /* Not implemented */ }
void ELECHOUSE_CC1101::SpiMode(byte config) { /* Not implemented */ }
void ELECHOUSE_CC1101::setSpiModule(byte module) { /* Not implemented */ }
void ELECHOUSE_CC1101::setClb(byte b, byte s, byte e) { /* Not implemented */ }
void ELECHOUSE_CC1101::setLeng(byte b) { SpiWriteReg(CC1101_PKTLEN, b); }

// Missing mode control functions
void ELECHOUSE_CC1101::SetRx(void)
{
    SpiStrobe(CC1101_SRX);
}

void ELECHOUSE_CC1101::SetTx(void)
{
    SpiStrobe(CC1101_STX);
}

void ELECHOUSE_CC1101::setSidle(void)
{
    SpiStrobe(CC1101_SIDLE);
}

// Global buffer for received data
byte rxBuffer[64];
