# EvilCrowRF-V2

![EvilCrow](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/Logo1.png)

**Idea, development and implementation:** Joel Serna (@JoelSernaMoreno).

**Main collaborator:** Little Satan (https://github.com/LSatan/)

**Other collaborators:** Eduardo Blázquez (@_eblazquez), Federico Maggi (@phretor), Andrea Guglielmini (@Guglio95) and RFQuack (@rfquack).

**PCB design:** Ignacio Díaz Álvarez (@Nacon_96), Forensic Security (@ForensicSec) and April Brother (@aprbrother).

**Manufacturer and distributor:** April Brother (@aprbrother).

**Distributor from United Kingdom:** KSEC Worldwide (@KSEC_KC).

The developers and collaborators of this project do not earn money with this. 
You can invite me for a coffee to further develop Low-Cost hacking devices. If you don't invite me for a coffee, nothing happens, I will continue developing devices.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/E1E614OA5)

**For sale with April Brother (shipping from China):** 

* Evil Crow RF V2 Aliexpress: https://aliexpress.com/item/1005004019072519.html
* Evil Crow RF V2 Lite (without NRF2401L) Aliexpress: https://aliexpress.com/item/1005004032930927.html
* Evil Crow RF V2 Alibaba: https://www.alibaba.com/product-detail/Evil-Crow-RF2-signal-receiver-with_1600467911757.html 

**For sale with KSEC Worldwide (shipping from United Kingdom):**

* Evil Crow RF V2: https://labs.ksec.co.uk/product/evil-crow-rf-v2/
* Evil Crow RF V2 Lite: https://labs.ksec.co.uk/product/evil-crow-rf2-lite/

**Discord Group:** https://discord.gg/jECPUtdrnW

**Summary:**

1. Disclaimer
2. Introduction
3. Firmware
	* Installation
	* First steps with Evil Crow RF V2
	* RX Config Example
	* RX Log Example
	* RAW TX Config Example
	* Binary TX Config Example
	* Pushbuttons Configuration
	* URH Parse example
	* OTA Update
	* Power management
	* Other Sketches
4. Evil Crow RF V2 Support

# Disclaimer

Evil Crow RF V2 is a basic device for professionals and cybersecurity enthusiasts.

We are not responsible for the incorrect use of Evil Crow RF V2.

We recommend using this device for testing, learning and fun :D

**Be careful with this device and the transmission of signals. Make sure to follow the laws that apply to your country.**

![EvilCrowRF](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/evilcrowrfv2.jpg)

# Introduction

Evil Crow RF V2 is a radiofrequency hacking device for pentest and Red Team operations, this device operates in the following radiofrequency bands:

* 300Mhz-348Mhz
* 387Mhz-464Mhz
* 779Mhz-928Mhz
* 2.4GHz

Evil Crow RF V2 has two CC1101 radiofrequency modules, these modules can be configured to transmit or receive on different frequencies at the same time. Additionally, Evil Crow RF V2 has a NRF24L01 module for other attacks.

Evil Crow RF V2 allows the following attacks:

* Signal receiver
* Signal transmitter
* Replay attack
* URH parse
* Mousejacking
* ...

**NOTE:** 

* All devices have been flashed with basic firmware Evil Crow RF V2 before shipping.
* Please do not ask me to implement new functions in this code. You can develop code for Evil Crow RF V2 and send PR with your new code.

# Firmware

The basic firmware allows to receive and transmit signals. You can configure the two radio modules through a web panel via WiFi.

## Installation

1. Install esptool: sudo apt install esptool
2. Install pyserial: sudo pip install pyserial
3. Download and Install the Arduino IDE: https://www.arduino.cc/en/main/software
4. Download Evil Crow RF V2 repository: git clone https://github.com/joelsernamoreno/EvilCrowRF-V2.git
5. Download the ESPAsyncWebServer library in the Arduino library directory: git clone https://github.com/me-no-dev/ESPAsyncWebServer.git
6. Download the AsyncElegantOTA library in the Arduino library directory: git clone https://github.com/ayushsharma82/AsyncElegantOTA.git
7. Download the AsyncTCP library in the Arduino library directory: git clone https://github.com/me-no-dev/AsyncTCP.git
8. Edit AsyncTCP/src/AsyncTCP.h and change the following:

* #define CONFIG_ASYNC_TCP_USE_WDT 1 to #define CONFIG_ASYNC_TCP_USE_WDT 0 

9. Open Arduino IDE
10. Go to File - Preferences. Locate the field "Additional Board Manager URLs:" Add "https://dl.espressif.com/dl/package_esp32_index.json" without quotes. Click "Ok"
11. Select Tools - Board - Boards Manager. Search for "esp32". Install "esp32 by Espressif system version 1.0.6". Click "Close".
12. Open the EvilCrowRF-V2/firmware/v1.3/EvilCrow-RFv2/EvilCrow-RFv2.ino sketch
13. Select Tools:
    * Board - "ESP32 Dev Module".
    * Flash Size - "4MB (32Mb)".
    * CPU Frequency - "80MHz (WiFi/BT)".
    * Flash Frequency - "40MHz"
    * Flash Mode - "DIO"
14. Upload the code to the Evil Crow RF V2 device
15. Copy the EvilCrowRF-V2/firmware/v1.0/SD/HTML folder to a MicroSD card.
16. Copy the EvilCrowRF-V2/firmware/v1.0/SD/URH folder to a MicroSD card.

![SD](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/sd.png)

## First steps with Evil Crow RF V2

1. Insert the MicroSD card into the Evil Crow RF V2 and connect the device to an external battery or laptop.
2. Visualize the wifi networks around you and connect to the Evil Crow RF V2 (default SSID: Evil Crow RF v2).
3. Enter the password for the wifi network (default password: 123456789).
4. Open a browser and access the web panel (default IP: 192.168.4.1).
5. Go!

![Webpanel](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/webpanel.png)

## RX Config Example

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Modulation: (example ASK/OOK)
* Frequency: (example 433.92)
* RxBW bandwidth: (example 58)
* Deviation: (example 0)
* Data rate: (example 5)

![RX](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/rx.png)

## RX Log Example

![RXLog](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/rx-log.png)

## RAW TX Config Example

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Modulation: (example ASK/OOK)
* Transmissions: (number transmissions)
* Frequency: (example 433.92)
* RAW Data: (raw data or raw data corrected displayed in RX Log)
* Deviation: (example 0)

![TXRAW](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/txraw.png)

## Binary TX Config Example

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Modulation: (example ASK/OOK)
* Transmissions: (number transmissions)
* Frequency: (example 433.92)
* Binary Data: (binary data displayed in RX Log)
* Sample Pulse: (samples/symbol displayed in RX Log)
* Deviation: (example 0)

![TXBINARY](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/txbinary.png)

## Pushbuttons Configuration

* Button: (1 for first pushbutton, 2 for second pushbutton)
* Modulation: (example ASK/OOK)
* Transmissions: (number transmissions)
* Frequency: (example 433.92)
* RAW Data: (raw data or raw data corrected displayed in RX Log)
* Deviation: (example 0)

![TXBUTTON](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/pushbutton.png)

## URH Parse example

Demo: https://youtube.com/watch?v=TAgtaAnLL6U

## OTA Update

Demo: https://www.youtube.com/watch?v=R0Mw3-EsuQA

## Power Management

1. In normal mode, press push2 + reset, then release reset: Evil Crow RF v2 blinks several times and goes to sleep. 
2. In sleep mode, press push2 + reset, then release reset to wake him up.

Demo: https://www.youtube.com/shorts/K_Qkss6-pEY

**NOTE:** If Evil Crow RF v2 is sleeping and you accidentally press reset, he'll go straight back to sleep. If he isn't asleep and you press reset then he will stay awake too.

## Other Sketches

* Mousejacking: EvilCrowRF-V2/firmware/other/standalone-mousejacking
* ...

# Evil Crow RF V2 Support

* You can ask in the Discord group: https://discord.gg/jECPUtdrnW
* You can open issue or send me a message via twitter (@JoelSernaMoreno).

