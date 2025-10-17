#ifndef ELECHOUSE_CC1101_SRC_DRV_h
#define ELECHOUSE_CC1101_SRC_DRV_h

#include <Arduino.h>

// CC1101 CONFIG REGSITER
#define CC1101_IOCFG2 0x00
#define CC1101_IOCFG1 0x01
#define CC1101_IOCFG0 0x02
#define CC1101_FIFOTHR 0x03
#define CC1101_SYNC1 0x04
#define CC1101_SYNC0 0x05
#define CC1101_PKTLEN 0x06
#define CC1101_PKTCTRL1 0x07
#define CC1101_PKTCTRL0 0x08
#define CC1101_ADDR 0x09
#define CC1101_CHANNR 0x0A
#define CC1101_FSCTRL1 0x0B
#define CC1101_FSCTRL0 0x0C
#define CC1101_FREQ2 0x0D
#define CC1101_FREQ1 0x0E
#define CC1101_FREQ0 0x0F
#define CC1101_MDMCFG4 0x10
#define CC1101_MDMCFG3 0x11
#define CC1101_MDMCFG2 0x12
#define CC1101_MDMCFG1 0x13
#define CC1101_MDMCFG0 0x14
#define CC1101_DEVIATN 0x15
#define CC1101_MCSM2 0x16
#define CC1101_MCSM1 0x17
#define CC1101_MCSM0 0x18
#define CC1101_FOCCFG 0x19
#define CC1101_BSCFG 0x1A
#define CC1101_AGCCTRL2 0x1B
#define CC1101_AGCCTRL1 0x1C
#define CC1101_AGCCTRL0 0x1D
#define CC1101_WOREVT1 0x1E
#define CC1101_WOREVT0 0x1F
#define CC1101_WORCTRL 0x20
#define CC1101_FREND1 0x21
#define CC1101_FREND0 0x22
#define CC1101_FSCAL3 0x23
#define CC1101_FSCAL2 0x24
#define CC1101_FSCAL1 0x25
#define CC1101_FSCAL0 0x26
#define CC1101_RCCTRL1 0x27
#define CC1101_RCCTRL0 0x28
#define CC1101_FSTEST 0x29
#define CC1101_PTEST 0x2A
#define CC1101_AGCTEST 0x2B
#define CC1101_TEST2 0x2C
#define CC1101_TEST1 0x2D
#define CC1101_TEST0 0x2E

#define CC1101_SRES 0x30
#define CC1101_SFSTXON 0x31
#define CC1101_SXOFF 0x32
#define CC1101_SCAL 0x33
#define CC1101_SRX 0x34
#define CC1101_STX 0x35
#define CC1101_SIDLE 0x36

#define CC1101_SWOR 0x38
#define CC1101_SPWD 0x39
#define CC1101_SFRX 0x3A
#define CC1101_SFTX 0x3B
#define CC1101_SWORRST 0x3C
#define CC1101_SNOP 0x3D

#define CC1101_PARTNUM 0x30
#define CC1101_VERSION 0x31
#define CC1101_FREQEST 0x32
#define CC1101_LQI 0x33
#define CC1101_RSSI 0x34
#define CC1101_MARCSTATE 0x35
#define CC1101_WORTIME1 0x36
#define CC1101_WORTIME0 0x37
#define CC1101_PKTSTATUS 0x38
#define CC1101_VCO_VC_DAC 0x39
#define CC1101_TXBYTES 0x3A
#define CC1101_RXBYTES 0x3B

#define CC1101_PATABLE 0x3E
#define CC1101_TXFIFO 0x3F
#define CC1101_RXFIFO 0x3F

class ELECHOUSE_CC1101
{
private:
    void SpiStart(void);
    void SpiEnd(void);
    void SpiWriteReg(byte addr, byte value);
    byte SpiReadReg(byte addr);
    void SpiWriteBurstReg(byte addr, byte *buffer, byte num);
    void SpiReadBurstReg(byte addr, byte *buffer, byte num);
    void SpiStrobe(byte strobe);

public:
    void Init(void);
    void setSpiPin(byte sck, byte miso, byte mosi, byte ss);
    void setGDO(byte gdo0, byte gdo2);
    void setModulation(byte m);
    void setPA(int p);
    void setMHZ(float mhz);
    void setCCMode(bool s);
    void setSyncMode(byte s);
    void setAddr(byte s);
    void setChannel(byte chnl);
    void setChsp(float f);
    void setRxBW(float f);
    void setDRate(float d);
    void setDeviation(float d);
    void setFilterLength(byte f);
    byte getCC1101_PARTNUM(void);
    byte getCC1101_VERSION(void);
    byte getCC1101_MARCSTATE(void);
    byte getCC1101_RXBYTES(void);
    byte getCC1101_TXBYTES(void);
    byte getCC1101_STATUS(void);
    byte receive(void);
    void sendData(byte *txBuffer, byte size);
    void receiveData(byte *rxBuffer, byte size);
    void Reset(void);
    void SpiInit(void);
    void SpiMode(byte config);
    void setSpi(void);
    void setSpiModule(byte module);
    void RegConfigSettings(void);
    byte SpiReadStatus(byte addr);
    void setClb(byte b, byte s, byte e);
    void setLeng(byte b);
    byte Check_RXFIFO(void);
    byte Check_TXFIFO(void);
    byte Bytes_In_RXFIFO(void);
    byte Bytes_In_TXFIFO(void);
    void TXFIFO_Flush(void);
    void RXFIFO_Flush(void);
    void SetRx(void);
    void SetTx(void);
    void setSidle(void);
};

extern ELECHOUSE_CC1101 ELECHOUSE_cc1101;
extern byte rxBuffer[64];

#endif
