"""Background serial reader for the receiver Arduino."""

import time

import serial
from PyQt6.QtCore import QThread, pyqtSignal


class SerialWorker(QThread):
    record_received = pyqtSignal(str)
    status_message = pyqtSignal(str)
    connection_lost = pyqtSignal(str)

    def __init__(self, port: str, baud_rate: int = 9600, parent=None):
        super().__init__(parent)
        self.port = port
        self.baud_rate = baud_rate
        self._running = True
        self._serial = None

    def run(self):
        try:
            self._serial = serial.Serial(
                port=self.port,
                baudrate=self.baud_rate,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.25,
            )
            # Allow the Arduino USB serial interface to settle.
            time.sleep(1.5)
            self._serial.reset_input_buffer()
            self.status_message.emit(f"Connected to {self.port}")

            while self._running:
                try:
                    raw_line = self._serial.readline()
                    if not raw_line:
                        continue

                    line = raw_line.decode("utf-8", errors="replace").strip()
                    if line.startswith("DATA,"):
                        self.record_received.emit(line)
                    # Startup text such as "Radio init Passed !!!" is ignored.
                except serial.SerialException as exc:
                    self.connection_lost.emit(str(exc))
                    break

        except serial.SerialException as exc:
            self.connection_lost.emit(str(exc))
        finally:
            if self._serial and self._serial.is_open:
                self._serial.close()
            self.status_message.emit("Serial reader stopped")

    def stop(self):
        self._running = False
        self.wait(1500)
