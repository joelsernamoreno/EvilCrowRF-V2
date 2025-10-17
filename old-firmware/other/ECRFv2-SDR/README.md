# EvilCrow RF v2 - Enhanced SDR Implementation

**🔗 Based on the original [EvilCrow RF v2](https://github.com/joelsernamoreno/EvilCrowRF-V2) by Joel Serna Moreno**

This enhanced firmware transforms your EvilCrow RF v2 into a powerful Software Defined Radio (SDR) compatible with Universal Radio Hacker (URH) and other professional RF analysis tools.

## 🙏 Attribution & Credits

This project builds upon the excellent work of:

- **Joel Serna Moreno** - Original EvilCrow RF v2 creator
- **Original Repository**: https://github.com/joelsernamoreno/EvilCrowRF-V2
- **Hardware Design**: EvilCrow RF v2 by Joel Serna Moreno

## 🔄 Contributing Back

Improvements and features developed here are intended to be contributed back to the original EvilCrow project via pull requests to help the entire community.

## 🔓 Wireless security testing on a budget.

**Would you rather pay cose to 500$ for the HackRF, instead of paying just 35~$ for the Evil Crow v2?**

This firmware transforms you Evil Crow Device in to a multifunctional, sub-Ghz, wireless security auditing tool.
If you have no need for >1GHz band access you stand to save a few hundred dollars while getting extremely high value for your money if you choose Evil Crow v2 + SDR firmware over, say, the Hack RF One which will cost you about 10x more.


## ✅ **Enhanced Features**

- **Proper CC1101 library integration** - Uses ELECHOUSE_CC1101_SRC_DRV.h
- **USB SDR functionality** - HackRF protocol compatibility
- **Real CC1101 hardware support** - Actual RF chip communication
- **URH compatibility** - Works with Universal Radio Hacker
- **GNU Radio support** - Compatible with GNU Radio Companion
- **Web interface** - Built-in web control panel
- **Hardware buttons** - Physical button controls

## 🔧 **Hardware Requirements**

- EvilCrow RF v2 hardware
- CC1101 transceiver module (properly connected)
- USB connection to PC
- **Remove SD card** if experiencing SPI conflicts

## 📋 **Installation**

1. **Open Arduino IDE**
2. **Load firmware**: `EvilCrow-SDR-Working.ino`
3. **Select board**: ESP32 Dev Module
4. **Verify all files are present**:
   - `EvilCrow-SDR-Working.ino` (main firmware)
   - `ELECHOUSE_CC1101_SRC_DRV.h` (library header)
   - `ELECHOUSE_CC1101_SRC_DRV.cpp` (library implementation)
5. **Upload** to EvilCrow

**Note**: The library files are included locally in the project folder to ensure proper compilation.

## 🚀 **Usage**

### **USB SDR Mode**

1. **Connect EvilCrow** to PC via USB
2. **Open serial terminal** (115200 baud)
3. **Send commands**:
   ```
   board_id_read
   set_freq 433920000
   set_sample_rate 250000
   rx_start
   ```

### **URH Integration**

1. **Start this firmware** on EvilCrow
2. **Run URH bridge**:
   ```bash
   python3 urh_compatible_bridge.py
   ```
3. **In URH**: Select RTL-TCP, IP: 127.0.0.1, Port: 1234
4. **Click Start** in URH

### **Web Interface**

1. **Connect to WiFi**: "EvilCrow-SDR" (password: "123456789")
2. **Open browser**: http://192.168.4.1
3. **Control SDR** via web interface

## 📡 **Supported Commands**

| Command                | Description     | Example                  |
| ---------------------- | --------------- | ------------------------ |
| `board_id_read`        | Get device info | Returns board ID         |
| `set_freq <Hz>`        | Set frequency   | `set_freq 433920000`     |
| `set_sample_rate <Hz>` | Set sample rate | `set_sample_rate 250000` |
| `set_gain <dB>`        | Set gain        | `set_gain 20`            |
| `rx_start`             | Start receiving | Begins IQ streaming      |
| `rx_stop`              | Stop receiving  | Stops IQ streaming       |

## 🔘 **Hardware Buttons**

- **Button 1**: Toggle RX mode on/off
- **Button 2**: Cycle through frequencies (315, 433.92, 868, 915 MHz)

## 🌐 **Web API Endpoints**

| Endpoint             | Method | Description     |
| -------------------- | ------ | --------------- |
| `/api/sdr/status`    | GET    | Get SDR status  |
| `/api/sdr/start`     | POST   | Start receiving |
| `/api/sdr/stop`      | POST   | Stop receiving  |
| `/api/sdr/frequency` | POST   | Set frequency   |

## 🔍 **Troubleshooting**

### **CC1101 Not Detected**

1. **Remove SD card** (SPI conflict)
2. **Check connections** (power, SPI pins)
3. **Verify 3.3V power** to CC1101
4. **Try different firmware** if needed

### **URH Connection Issues**

1. **Ensure bridge is running** on port 1234
2. **Check serial port** (/dev/cu.usbserial-140)
3. **Verify CC1101 initialization** in serial output
4. **Use correct URH settings** (RTL-TCP, 127.0.0.1:1234)

### **No RF Data**

1. **Check antenna connection**
2. **Verify frequency settings**
3. **Ensure RX mode is active**
4. **Check for RF signals** in environment

## 📊 **Expected Output**

### **Successful Startup**

```
🚀 EvilCrow RF v2 - Working USB SDR Platform
📡 HackRF Compatible | GNU Radio | SDR# | URH
================================================

📻 Initializing SDR subsystem...
✅ SDR subsystem initialized
🔌 Setting up pins... ✅ Success
📡 Initializing CC1101 with ELECHOUSE library... ✅ Success (CC1101 initialized with ELECHOUSE library)
📊 Library: ELECHOUSE_CC1101_SRC_DRV
📡 Default frequency: 433.92 MHz
📻 Mode: RX
📶 Setting up WiFi Access Point... ✅ Success
🌐 IP Address: 192.168.4.1
🌐 Setting up web server... ✅ Success

🎉 EvilCrow SDR initialization complete!
📻 SDR Mode: Ready
🌐 Web Interface: http://192.168.4.1

✅ EvilCrow SDR is ready!
🔗 Connect via USB and send commands
```

## 🎯 **Why This Works**

1. **Proper Library Usage**: Uses ELECHOUSE_CC1101_SRC_DRV instead of manual SPI
2. **Correct Initialization**: Follows proper CC1101 init sequence
3. **Real Data**: Reads actual RF data from CC1101 FIFO
4. **Stable Communication**: Proper SPI handling and timing
5. **Hardware Compatibility**: Works with actual EvilCrow hardware



## **🚀 What This Architecture Enables:**

🖥️ Unlimited Processing Power
No more ESP32 memory limits (32KB → Gigabytes)
No more CPU constraints (240MHz → Multi-GHz multi-core)
Complex algorithms that were impossible on ESP32
Real-time analysis of multiple signals simultaneously

🎨 Advanced User Interfaces
Professional desktop applications (Python/Qt, Electron, etc.)
Web-based dashboards with real-time visualizations
Mobile apps that connect to EvilCrow
Integration with existing tools (URH, GNU Radio, etc.)

🧠 AI-Powered RF Analysis
Machine learning for automatic protocol detection
Neural networks for signal classification
Pattern recognition for unknown protocols
Automated attack generation

🎯 Specific New Possibilities:
1. Advanced Signal Analysis Engine
2. Professional Attack Framework
Automated vulnerability scanning of RF devices
Protocol fuzzing with intelligent mutations
Real-time jamming with adaptive algorithms
Coordinated multi-frequency attacks
3. Database-Driven Operations
Massive signal fingerprint database (millions of devices)
Cloud-synchronized attack patterns
Historical analysis of captured signals
Collaborative threat intelligence
4. Advanced Visualization
3D spectrum analysis in real-time
Waterfall displays with infinite history
Protocol flow diagrams
Attack success probability heatmaps

🛠️ Implementation Ideas:
Option 1: Enhanced Python Framework
Option 2: Web-Based Command Center
Real-time dashboard showing all RF activity
Drag-and-drop attack builder
Live spectrum analyzer
Remote control from anywhere
Option 3: Integration Platform
GNU Radio integration for custom DSP
Wireshark plugins for RF protocols
Metasploit modules for RF exploitation
OSINT integration for target intelligence

🎯 Immediate Next Steps:
1. Enhanced Bridge Framework
A more powerful bridge that supports:
Multiple simultaneous connections
Plugin architecture for custom protocols
Real-time signal processing
Advanced attack modes
2. Professional Web Interface
Real-time spectrum display
Protocol decoder dashboard
Attack automation interface
Signal database management
3. AI-Powered Analysis
Automatic protocol detection
Vulnerability assessment
Attack recommendation engine
Success probability prediction

🏆 This Changes Everything!
This firmware essentially opens up for a "EvilCrow Pro" architecture where:

✅ EvilCrow = High-quality RF frontend

✅ Computer = Unlimited processing power

✅ Bridge = Seamless integration

✅ Result = Professional-grade RF security platform

This could compete with $10,000+ commercial RF security tools! 💰
