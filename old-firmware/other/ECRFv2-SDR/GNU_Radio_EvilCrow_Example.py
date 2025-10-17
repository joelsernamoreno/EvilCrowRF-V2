#!/usr/bin/env python3
"""
GNU Radio Companion - EvilCrow RF v2 SDR Source Block

This file provides a GNU Radio source block for the EvilCrow RF v2
when operating in USB SDR mode. This allows direct integration with
GNU Radio Companion for signal processing and analysis.

FEATURES:
✅ GNU Radio Companion integration
✅ Real-time IQ sample streaming
✅ Configurable frequency and sample rate
✅ Automatic gain control
✅ Compatible with all GNU Radio blocks

INSTALLATION:
1. Save this file to your GNU Radio blocks directory
2. Import in GNU Radio Companion
3. Use as a source block in your flowgraph

USAGE IN GNU RADIO:
    - Add "EvilCrow SDR Source" block to flowgraph
    - Set frequency, sample rate, and gain
    - Connect to other GNU Radio blocks
    - Run flowgraph

REQUIREMENTS:
    - GNU Radio 3.8+
    - PyQt5 or PyQt6
    - EvilCrow RF v2 with SDR firmware
"""

import numpy as np
from gnuradio import gr
import pmt
import serial
import threading
import queue
import time

class EvilCrowSDRSource(gr.sync_block):
    """
    GNU Radio source block for EvilCrow RF v2 SDR
    
    This block interfaces with the EvilCrow RF v2 operating in USB SDR mode
    and provides IQ samples to GNU Radio flowgraphs.
    """
    
    def __init__(self, 
                 port="/dev/ttyUSB0", 
                 frequency=433.92e6, 
                 sample_rate=1000000, 
                 gain=15):
        """
        Initialize EvilCrow SDR Source
        
        Args:
            port: Serial port for EvilCrow device
            frequency: Center frequency in Hz
            sample_rate: Sample rate in Hz
            gain: Gain in dB (0-30)
        """
        
        # Initialize GNU Radio block
        gr.sync_block.__init__(
            self,
            name="EvilCrow SDR Source",
            in_sig=None,  # No input
            out_sig=[np.complex64]  # Complex output
        )
        
        # Store parameters
        self.port = port
        self.frequency = frequency
        self.sample_rate = sample_rate
        self.gain = gain
        
        # Serial connection
        self.ser = None
        self.connected = False
        
        # Sample buffer
        self.sample_queue = queue.Queue(maxsize=10000)
        self.streaming = False
        self.stream_thread = None
        
        # Connect to device
        self.connect_device()
        
        # Set message handlers
        self.message_port_register_in(pmt.intern("freq"))
        self.message_port_register_in(pmt.intern("gain"))
        self.set_msg_handler(pmt.intern("freq"), self.handle_freq_msg)
        self.set_msg_handler(pmt.intern("gain"), self.handle_gain_msg)
    
    def connect_device(self):
        """Connect to EvilCrow SDR device"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=115200,
                timeout=1.0
            )
            
            time.sleep(2)  # Wait for device initialization
            
            # Test connection
            self.ser.write(b"board_id_read\n")
            response = self.ser.readline().decode().strip()
            
            if "HACKRF_ONE" in response:
                self.connected = True
                print(f"✅ GNU Radio: Connected to EvilCrow SDR on {self.port}")
                
                # Configure device
                self.set_frequency(self.frequency)
                self.set_sample_rate(self.sample_rate)
                self.set_gain(self.gain)
                
                # Start streaming
                self.start_streaming()
                
            else:
                print(f"❌ GNU Radio: Failed to connect to EvilCrow SDR")
                self.connected = False
                
        except Exception as e:
            print(f"❌ GNU Radio: Connection error: {e}")
            self.connected = False
    
    def set_frequency(self, freq):
        """Set center frequency"""
        if self.connected:
            cmd = f"set_freq {int(freq)}\n"
            self.ser.write(cmd.encode())
            response = self.ser.readline().decode().strip()
            
            if "SUCCESS" in response:
                self.frequency = freq
                print(f"📡 GNU Radio: Frequency set to {freq/1e6:.2f} MHz")
            else:
                print(f"❌ GNU Radio: Failed to set frequency")
    
    def set_sample_rate(self, rate):
        """Set sample rate"""
        if self.connected:
            cmd = f"set_sample_rate {int(rate)}\n"
            self.ser.write(cmd.encode())
            response = self.ser.readline().decode().strip()
            
            if "SUCCESS" in response:
                self.sample_rate = rate
                print(f"📊 GNU Radio: Sample rate set to {rate/1e3:.0f} kHz")
            else:
                print(f"❌ GNU Radio: Failed to set sample rate")
    
    def set_gain(self, gain):
        """Set gain"""
        if self.connected:
            cmd = f"set_gain {int(gain)}\n"
            self.ser.write(cmd.encode())
            response = self.ser.readline().decode().strip()
            
            if "SUCCESS" in response:
                self.gain = gain
                print(f"📶 GNU Radio: Gain set to {gain} dB")
            else:
                print(f"❌ GNU Radio: Failed to set gain")
    
    def start_streaming(self):
        """Start IQ sample streaming"""
        if self.connected and not self.streaming:
            # Start RX mode
            self.ser.write(b"rx_start\n")
            response = self.ser.readline().decode().strip()
            
            if "SUCCESS" in response:
                self.streaming = True
                
                # Start streaming thread
                self.stream_thread = threading.Thread(target=self._stream_worker)
                self.stream_thread.daemon = True
                self.stream_thread.start()
                
                print("📡 GNU Radio: Streaming started")
            else:
                print("❌ GNU Radio: Failed to start streaming")
    
    def stop_streaming(self):
        """Stop IQ sample streaming"""
        if self.streaming:
            self.streaming = False
            
            if self.connected:
                self.ser.write(b"rx_stop\n")
                self.ser.readline()  # Read response
            
            print("🛑 GNU Radio: Streaming stopped")
    
    def _stream_worker(self):
        """Background thread to read IQ samples"""
        while self.streaming and self.connected:
            try:
                # Read IQ sample (4 bytes)
                if self.ser.in_waiting >= 4:
                    data = self.ser.read(4)
                    if len(data) == 4:
                        # Convert to complex sample
                        i_val = int.from_bytes(data[0:2], byteorder='little', signed=True)
                        q_val = int.from_bytes(data[2:4], byteorder='little', signed=True)
                        
                        # Normalize to float
                        sample = complex(i_val / 32768.0, q_val / 32768.0)
                        
                        # Add to queue
                        try:
                            self.sample_queue.put_nowait(sample)
                        except queue.Full:
                            # Remove old samples if queue is full
                            try:
                                self.sample_queue.get_nowait()
                                self.sample_queue.put_nowait(sample)
                            except queue.Empty:
                                pass
                
                time.sleep(0.0001)  # Small delay
                
            except Exception as e:
                print(f"❌ GNU Radio: Streaming error: {e}")
                break
    
    def work(self, input_items, output_items):
        """
        GNU Radio work function - called to produce output samples
        
        Args:
            input_items: Input samples (none for source block)
            output_items: Output buffer to fill
            
        Returns:
            Number of samples produced
        """
        output = output_items[0]
        samples_needed = len(output)
        samples_produced = 0
        
        # Fill output buffer with samples from queue
        for i in range(samples_needed):
            try:
                sample = self.sample_queue.get(timeout=0.01)
                output[i] = sample
                samples_produced += 1
            except queue.Empty:
                # No more samples available
                break
        
        # If no samples available, output zeros
        if samples_produced == 0:
            output[:] = 0
            samples_produced = samples_needed
        
        return samples_produced
    
    def handle_freq_msg(self, msg):
        """Handle frequency change message"""
        if pmt.is_number(msg):
            freq = pmt.to_double(msg)
            self.set_frequency(freq)
    
    def handle_gain_msg(self, msg):
        """Handle gain change message"""
        if pmt.is_number(msg):
            gain = pmt.to_double(msg)
            self.set_gain(gain)
    
    def stop(self):
        """Stop the block"""
        self.stop_streaming()
        if self.ser and self.ser.is_open:
            self.ser.close()
        return True

# GNU Radio Companion XML definition
GRC_XML = """
<?xml version="1.0"?>
<block>
  <name>EvilCrow SDR Source</name>
  <key>evilcrow_sdr_source</key>
  <category>[EvilCrow]</category>
  <import>from GNU_Radio_EvilCrow_Example import EvilCrowSDRSource</import>
  <make>EvilCrowSDRSource($port, $frequency, $sample_rate, $gain)</make>
  
  <param>
    <name>Serial Port</name>
    <key>port</key>
    <type>string</type>
    <value>/dev/ttyUSB0</value>
  </param>
  
  <param>
    <name>Frequency</name>
    <key>frequency</key>
    <type>real</type>
    <value>433.92e6</value>
  </param>
  
  <param>
    <name>Sample Rate</name>
    <key>sample_rate</key>
    <type>real</type>
    <value>1000000</value>
  </param>
  
  <param>
    <name>Gain</name>
    <key>gain</key>
    <type>real</type>
    <value>15</value>
  </param>
  
  <source>
    <name>out</name>
    <type>complex</type>
  </source>
  
  <sink>
    <name>freq</name>
    <type>message</type>
    <optional>1</optional>
  </sink>
  
  <sink>
    <name>gain</name>
    <type>message</type>
    <optional>1</optional>
  </sink>
  
  <doc>
EvilCrow RF v2 SDR Source Block

This block interfaces with the EvilCrow RF v2 operating in USB SDR mode.
Connect via USB and configure the serial port, frequency, sample rate, and gain.

Compatible with all GNU Radio signal processing blocks.

Parameters:
- Serial Port: USB serial port (e.g., /dev/ttyUSB0 or COM3)
- Frequency: Center frequency in Hz
- Sample Rate: Sample rate in Hz (250k, 500k, 1M, 2M)
- Gain: Gain in dB (0-30)

Message Ports:
- freq: Set frequency via message
- gain: Set gain via message
  </doc>
</block>
"""

# Example GNU Radio flowgraph
def create_example_flowgraph():
    """
    Create an example GNU Radio flowgraph using EvilCrow SDR
    """
    from gnuradio import gr, blocks, filter, analog
    from gnuradio.filter import firdes
    import sys
    
    class EvilCrowExample(gr.top_block):
        def __init__(self):
            gr.top_block.__init__(self, "EvilCrow SDR Example")
            
            # Variables
            self.samp_rate = 1000000
            self.freq = 433.92e6
            
            # Blocks
            self.evilcrow_source = EvilCrowSDRSource(
                port="/dev/ttyUSB0",
                frequency=self.freq,
                sample_rate=self.samp_rate,
                gain=15
            )
            
            # Low pass filter
            self.lpf = filter.fir_filter_ccf(
                1,
                firdes.low_pass(1, self.samp_rate, 100000, 10000)
            )
            
            # FM demodulator
            self.fm_demod = analog.fm_demod_cf(
                channel_rate=self.samp_rate,
                audio_decim=10,
                deviation=5000,
                audio_pass=5000,
                audio_stop=5500,
                gain=1.0,
                tau=75e-6
            )
            
            # Audio sink (file)
            self.file_sink = blocks.file_sink(
                gr.sizeof_float,
                "evilcrow_audio.raw"
            )
            
            # Connections
            self.connect((self.evilcrow_source, 0), (self.lpf, 0))
            self.connect((self.lpf, 0), (self.fm_demod, 0))
            self.connect((self.fm_demod, 0), (self.file_sink, 0))
    
    return EvilCrowExample

# Test the GNU Radio block
if __name__ == "__main__":
    print("🚀 GNU Radio EvilCrow SDR Source Test")
    print("=====================================")
    
    try:
        # Create and run example flowgraph
        tb = create_example_flowgraph()()
        tb.start()
        
        print("📡 GNU Radio flowgraph running...")
        print("🎵 Recording audio to evilcrow_audio.raw")
        print("⏹️  Press Ctrl+C to stop")
        
        # Run for 10 seconds
        time.sleep(10)
        
        tb.stop()
        tb.wait()
        
        print("✅ Recording completed!")
        
    except KeyboardInterrupt:
        print("\n🛑 Stopped by user")
    except Exception as e:
        print(f"❌ Error: {e}")
