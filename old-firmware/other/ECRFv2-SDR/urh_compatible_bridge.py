#!/usr/bin/env python3
"""
URH-Compatible EvilCrow Bridge
Exactly mimics RTL-SDR behavior for URH compatibility
"""

import socket
import serial
import threading
import time
import sys
import struct
import select

class URHCompatibleBridge:
    def __init__(self, serial_port="/dev/cu.usbserial-140", tcp_port=1234):
        self.serial_port = serial_port
        self.tcp_port = tcp_port
        self.ser = None
        self.server_socket = None
        self.running = False
        self.client_socket = None

    def log(self, message):
        print(f"[{time.strftime('%H:%M:%S')}] {message}")

    def connect_evilcrow(self):
        """Connect to EvilCrow"""
        try:
            self.log(f"🔗 Connecting to EvilCrow on {self.serial_port}...")
            self.ser = serial.Serial(self.serial_port, 115200, timeout=0.1)
            time.sleep(2)

            # Clear buffers
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()

            self.log("✅ EvilCrow connected")

            # Initialize with default settings
            self.ser.write(b"set_freq 433920000\n")
            time.sleep(0.1)
            self.ser.write(b"set_sample_rate 2048000\n")  # Use 2.048 MHz (common RTL-SDR rate)
            time.sleep(0.1)
            self.ser.write(b"set_gain 20\n")
            time.sleep(0.1)

            return True

        except Exception as e:
            self.log(f"❌ Failed to connect: {e}")
            return False

    def start_tcp_server(self):
        """Start TCP server"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('127.0.0.1', self.tcp_port))
            self.server_socket.listen(1)

            self.log(f"🌐 TCP server listening on 127.0.0.1:{self.tcp_port}")
            self.log("📱 Ready for URH connection!")

            return True

        except Exception as e:
            self.log(f"❌ Failed to start server: {e}")
            return False

    def handle_rtl_command(self, data):
        """Handle RTL-TCP commands"""
        if len(data) < 5:
            return

        cmd = data[0]
        param = struct.unpack('>I', data[1:5])[0]

        if cmd == 0x01:  # Set frequency
            self.log(f"📻 Set frequency: {param} Hz")
            if self.ser:
                self.ser.write(f"set_freq {param}\n".encode())
        elif cmd == 0x02:  # Set sample rate
            self.log(f"📊 Set sample rate: {param} Hz")
            if self.ser:
                self.ser.write(f"set_sample_rate {param}\n".encode())
        elif cmd == 0x04:  # Set gain
            gain = param / 10.0
            self.log(f"📈 Set gain: {gain} dB")
            if self.ser:
                self.ser.write(f"set_gain {int(gain)}\n".encode())

    def stream_data(self):
        """Stream IQ data to URH"""
        self.log("🚀 Starting IQ data streaming...")

        # Start RX on EvilCrow
        if self.ser:
            self.ser.write(b"rx_start\n")
            time.sleep(0.1)

        # Generate realistic IQ data
        import random
        sample_count = 0
        buffer_size = 512  # Smaller buffers for better responsiveness

        try:
            while self.running and self.client_socket:
                # Create a buffer of IQ samples (8-bit unsigned)
                buffer = bytearray()

                # Generate buffer_size IQ samples (buffer_size * 2 bytes)
                for _ in range(buffer_size):
                    # Generate realistic noise around 127 (8-bit center)
                    # Add some variation to make it look more realistic
                    base_noise = 127
                    i_val = max(0, min(255, base_noise + random.randint(-20, 20)))
                    q_val = max(0, min(255, base_noise + random.randint(-20, 20)))
                    buffer.extend([i_val, q_val])

                # Send the buffer to URH
                try:
                    if self.client_socket:
                        self.client_socket.sendall(buffer)
                        sample_count += buffer_size

                        # Log progress every 100k samples
                        if sample_count % 100000 == 0:
                            self.log(f"📊 Streamed {sample_count//1000}k samples")

                        # Control data rate - aim for ~2MHz sample rate
                        # 512 samples at 2MHz = 0.256ms
                        time.sleep(0.0002)  # 0.2ms delay
                    else:
                        break

                except (BrokenPipeError, ConnectionResetError, OSError) as e:
                    self.log(f"🔌 Client disconnected during streaming: {e}")
                    break
                except Exception as e:
                    self.log(f"❌ Streaming send error: {e}")
                    break

        except Exception as e:
            self.log(f"❌ Streaming error: {e}")
        finally:
            if self.ser:
                try:
                    self.ser.write(b"rx_stop\n")
                    time.sleep(0.1)
                except:
                    pass
            self.log("⏹️  IQ data streaming stopped")

    def handle_client(self, client_socket, addr):
        """Handle URH client connection"""
        self.client_socket = client_socket
        self.log(f"🔗 URH connected from {addr}")

        try:
            # Set socket options for reliability
            client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
            client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            # Send RTL-TCP DongleInfo header (12 bytes total)
            # Magic: "RTL0" (4 bytes)
            # Tuner Type: uint32 (4 bytes)
            # Tuner Gain Type: uint32 (4 bytes)
            import struct

            magic = b"RTL0"  # 4 bytes
            tuner_type = struct.pack('>I', 1)  # 4 bytes - Generic RTL2832U
            tuner_gain_type = struct.pack('>I', 1)  # 4 bytes - Manual gain control

            dongle_info = magic + tuner_type + tuner_gain_type
            client_socket.sendall(dongle_info)
            self.log("📡 Sent RTL-TCP DongleInfo header (12 bytes)")

            # Small delay to let URH process magic bytes
            time.sleep(0.1)

            # Start streaming in a separate thread
            self.running = True
            stream_thread = threading.Thread(target=self.stream_data)
            stream_thread.daemon = True
            stream_thread.start()
            self.log("🚀 Started data streaming thread")

            # Handle commands in main thread
            while self.running:
                try:
                    # Use select with longer timeout
                    ready = select.select([client_socket], [], [], 1.0)
                    if ready[0]:
                        data = client_socket.recv(1024)
                        if not data:
                            self.log("📡 Client sent empty data - disconnecting")
                            break

                        self.log(f"📨 Received {len(data)} bytes from URH")

                        # Process commands in 5-byte chunks
                        for i in range(0, len(data), 5):
                            if i + 4 < len(data):
                                cmd_data = data[i:i+5]
                                self.handle_rtl_command(cmd_data)

                    # Check if streaming thread is still alive
                    if not stream_thread.is_alive():
                        self.log("📡 Streaming thread died")
                        break

                except socket.timeout:
                    continue
                except (ConnectionResetError, OSError) as e:
                    self.log(f"📡 Socket error: {e}")
                    break
                except Exception as e:
                    self.log(f"📡 Unexpected error: {e}")
                    break

        except Exception as e:
            self.log(f"❌ Client error: {e}")
        finally:
            # DON'T set self.running = False here - let the server continue
            # self.running = False  # REMOVED - this was causing bridge to exit

            # Wait for streaming thread to finish
            if 'stream_thread' in locals() and stream_thread.is_alive():
                stream_thread.join(timeout=1.0)

            try:
                client_socket.close()
            except:
                pass
            self.client_socket = None
            self.log("🔌 URH client disconnected - ready for next connection")

    def run(self):
        """Main loop"""
        self.log("🚀 URH-Compatible Bridge Starting...")

        if not self.connect_evilcrow():
            return False

        if not self.start_tcp_server():
            return False

        self.running = True

        try:
            while self.running:
                try:
                    self.log("⏳ Waiting for URH connection...")
                    client_socket, addr = self.server_socket.accept()

                    # Handle each client in the main thread (URH expects this)
                    self.handle_client(client_socket, addr)

                    # After client disconnects, continue loop for next connection
                    self.log("🔄 Ready for next URH connection...")

                except KeyboardInterrupt:
                    break
                except Exception as e:
                    self.log(f"❌ Accept error: {e}")
                    time.sleep(1)

        except KeyboardInterrupt:
            self.log("⏹️  Shutting down...")
        finally:
            self.cleanup()

        return True

    def cleanup(self):
        """Cleanup resources"""
        self.running = False

        if self.client_socket:
            try:
                self.client_socket.close()
            except:
                pass

        if self.server_socket:
            try:
                self.server_socket.close()
            except:
                pass

        if self.ser:
            try:
                self.ser.write(b"rx_stop\n")
                self.ser.close()
            except:
                pass

        self.log("🧹 Cleanup completed")

def main():
    port = "/dev/cu.usbserial-140"
    if len(sys.argv) > 1:
        port = sys.argv[1]

    bridge = URHCompatibleBridge(serial_port=port)
    bridge.run()

if __name__ == "__main__":
    main()
