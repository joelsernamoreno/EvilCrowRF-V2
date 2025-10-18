# EvilCrowRF-V2

![EvilCrow](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/Logo1.png)

**Idea, development and implementation:** Joel Serna (@JoelSernaMoreno).

**Main collaborator:** Little Satan (https://github.com/LSatan/)

**Other collaborators:** Jordi Castelló (@iordic), Eduardo Blázquez (@_eblazquez), Federico Maggi (@phretor), Andrea Guglielmini (@Guglio95) and RFQuack (@rfquack).

**PCB design:** Ignacio Díaz Álvarez (@Nacon_96), Forensic Security (@ForensicSec) and April Brother (@aprbrother).

**Manufacturer and distributor:** April Brother (@aprbrother).

**Distributor from United Kingdom:** KSEC Worldwide (@KSEC_KC).

The developers and collaborators of this project do not earn money with this. 
You can invite me for a coffee to further develop Low-Cost hacking devices. If you don't invite me for a coffee, nothing happens, I will continue developing devices.

[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/E1E614OA5)

**Available with April Brother (shipping from China):** 

* Evil Crow RF V2 Tindie: https://www.tindie.com/products/aprbrother/evil-crow-rf-v2-rf-transceiver-rf/
* Evil Crow RF V2 Alibaba: https://www.alibaba.com/product-detail/Evil-Crow-RF2-signal-receiver-with_1600467911757.html 
* Evil Crow RF V2 Aliexpress: https://www.aliexpress.com/item/3256807682636637.html

**Available with SAPSAN Cybersec & Military (shipping from EU, Poland):**

* https://sapsan-sklep.pl/en/products/evil-crow-rf-v2

**Available with KSEC Worldwide (shipping from United Kingdom):**

* Evil Crow RF V2: https://labs.ksec.co.uk/product/evil-crow-rf-v2/
* Evil Crow RF V2 Lite: https://labs.ksec.co.uk/product/evil-crow-rf2-lite/

**Available with Tienda Espía (shipping from Mexico):**

* https://tiendaespia.com.mx/producto/evil-crow-rf-v2-radiofrequency-hacking-device/

**Discord Group:** https://discord.gg/evilcrowrf

**Summary:**

1. Disclaimer
2. Introduction
3. Firmware
	* Installation
	* First steps with Evil Crow RF V2
	* Home
	* RX Config
	* Log Viewer
	* TX Config
	* Config
4. Evil Crow RF V2 Support
5. Other compatible firmware

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
3. Download and install the latest version of Arduino IDE (v2.3.6): https://www.arduino.cc/en/main/software
4. Download Evil Crow RF V2 repository: git clone https://github.com/joelsernamoreno/EvilCrowRF-V2.git
5. Download the ESPAsyncWebServer library in the Arduino library directory: git clone https://github.com/ESP32Async/ESPAsyncWebServer.git
6. Download the ElegantOTA library in the Arduino library directory: git clone https://github.com/ayushsharma82/ElegantOTA.git
7. Edit ElegantOTA/src/ElegantOTA.h and chage the following:

* #define ELEGANTOTA_USE_ASYNC_WEBSERVER 0 to #define ELEGANTOTA_USE_ASYNC_WEBSERVER 1

8. Download the AsyncTCP library in the Arduino library directory: git clone https://github.com/ESP32Async/AsyncTCP.git
9. Open Arduino IDE
10. Go to File - Preferences. Locate the field "Additional Board Manager URLs:" Add "https://espressif.github.io/arduino-esp32/package_esp32_index.json" without quotes. Click "Ok"
11. Select Tools - Board - Boards Manager. Search for "esp32". Install "esp32 by Espressif system version 3.3.2". Click "Close".
12. Open the EvilCrowRF-V2/firmware/firmware/firmware.ino sketch
13. Select Tools:
    * Board - "ESP32 Dev Module".
    * Flash Size - "4MB (32Mb)".
    * CPU Frequency - "80MHz (WiFi/BT)".
    * Flash Frequency - "40MHz"
    * Flash Mode - "DIO"
14. Upload the code to the Evil Crow RF V2 device
15. Copy the EvilCrowRF-V2/firmware/SD/HTML folder to a MicroSD card.

![SD](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/sd.png)

**Notes about SD:** 

* Web server will not load, is a blank page or displays nothing:

Check you have copied the relevant files to the SD card and the SD card is inserted in the Evil Crow RF V2 device.

* My files are on the SD card but the web server will not work:

Check your SD card size. It is recommended to use a small card. 32GB or smaller is sufficent for operation. Cards larger than this have been shown to cause issues and not work.

## First steps with Evil Crow RF V2

0. Check & verify you have copied the relevant files to your SD card.
1. Insert the MicroSD card into the Evil Crow RF V2 and connect the device to an external battery or laptop.
2. Set up a Wi-Fi AP with your mobile phone:
	* **SSID:** Evil Crow RF v2
	* **Password:** 123456789ECRFv2
3. Connect your laptop to the same Wi-Fi network.
4. Open a browser and access the web panel: http://evilcrow-rf.local/

**Note:** If you cannot access the web panel, use the IP address assigned to Evil Crow RF v2 or follow below steps **only if you are running Linux OS:**
 * check if avahi-deamon is installed and running on your PC. You can do this with executing "sudo systemctl status avahi-daemon" in terminal
 * If service is not running, install it using your package manager (apt, yum, dnf, Packman, rpm,...)
 * After successful installation, start avahi-daemon service with "sudo systemctl start avahi-daemon && sudo systemctl enable avahi-daemon"
 * In case evilcrow-rf.local is still not reachable, use http://"IP address", where "IP address" is IP assigned to Evil Crow RF v2.

## Home

The Home page shows interesting information about the device.

![Home](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/home.png)

## RX Config

The RX Config page allows you to configure the CC101 modules for receiving signals. The received signals are displayed in the Log Viewer.

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Modulation: (example ASK/OOK)
* Frequency: (example 433.92)
* Rx bandwidth: (example 200)
* Deviation: (example 0)
* Data rate: (example 5)

![RX](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/rx.png)

## Log Viewer

![RXLog](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/rx-log.png)

## TX Config

The TX Config page allows you to transmit a raw data signal or enable/disable the jammer.

![TXCONFIG](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/txconfig.png)

* **TX Raw Data:**

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Modulation: (example ASK/OOK)
* Frequency: (example 433.92)
* RAW Data: (raw data or raw data corrected displayed in Log Viewer)
* Deviation: (example 0)

![TXRAW](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/txraw.png)

* **Jammer:**

* Module: (1 for first CC1101 module, 2 for second CC1101 module)
* Frequency: (example: 433.92)
* Jammer Power: (example: 12)

![JAMMER](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/jammer.png)

## Config

The Config page allows you to change the Wi-Fi configuration.

**NOTE:** Evil Crow RF V2 is configured in STATION MODE. You can change the ssid and password from the web panel.

The changes will be stored in the device, every time you restart Evil Crow RF V2 the new Wi-Fi settings will be applied. If you want to return to the default settings, you can delete the stored Wi-Fi configuration from the web panel.

![CONFIG](https://github.com/joelsernamoreno/EvilCrowRF-V2/blob/main/images/configwifi.png)

# Evil Crow RF V2 Support

* You can ask in the Discord group: https://discord.gg/jECPUtdrnW
* You can open issue or send me a message via twitter (@JoelSernaMoreno).

# Other compatible firmware

**You can also use alternative firmware options, for example:**

* https://github.com/h-RAT/EvilCrowRF_Custom_Firmware_CC1101_FlipperZero

* https://github.com/realdaveblanch/Evil-Crow-RF-v2-Custom-ROM/

* https://github.com/tutejshy-bit/tut-rf
