"""PyQt6 receiver-side telemetry dashboard."""

from datetime import datetime

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel, QMainWindow,
    QMessageBox, QPushButton, QVBoxLayout, QWidget
)
from serial.tools import list_ports

from data_parser import DataRecordError, parse_data_line
from serial_worker import SerialWorker


DARK_STYLE = """
QMainWindow, QWidget { background: #101820; color: #e8eef5; font-family: Arial; }
QLabel#title { font-size: 25px; font-weight: 700; color: #ffffff; }
QLabel#subtitle { color: #91a2b4; font-size: 11px; }
QFrame[card="true"] { background: #1a2733; border: 1px solid #2a3b4b; border-radius: 12px; }
QLabel.cardTitle { color: #8fa6ba; font-size: 12px; font-weight: 700; }
QLabel.value { color: #f4f7fa; font-size: 23px; font-weight: 600; }
QLabel.smallValue { color: #e8eef5; font-size: 15px; font-weight: 600; }
QLabel.muted { color: #91a2b4; font-size: 11px; }
QComboBox, QPushButton { background: #263747; border: 1px solid #3b5266; border-radius: 7px; padding: 8px 12px; color: #ffffff; }
QPushButton:hover { background: #344d62; }
QPushButton:disabled { color: #718091; background: #202c37; }
"""


class ValueCard(QFrame):
    def __init__(self, title, value="—", unit="", parent=None):
        super().__init__(parent)
        self.setProperty("card", "true")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 13, 16, 13)
        layout.setSpacing(5)

        heading = QLabel(title.upper())
        heading.setProperty("class", "cardTitle")
        heading.setObjectName("cardTitle")
        self.value_label = QLabel(value)
        self.value_label.setProperty("class", "value")
        self.value_label.setObjectName("value")
        self.unit_label = QLabel(unit)
        self.unit_label.setProperty("class", "muted")
        self.unit_label.setObjectName("muted")

        layout.addWidget(heading)
        layout.addWidget(self.value_label)
        layout.addWidget(self.unit_label)

    def set_value(self, value, unit=None):
        self.value_label.setText(str(value))
        if unit is not None:
            self.unit_label.setText(unit)


class InfoCard(QFrame):
    def __init__(self, title, rows, parent=None):
        super().__init__(parent)
        self.setProperty("card", "true")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 14, 16, 14)
        layout.setSpacing(10)

        heading = QLabel(title.upper())
        heading.setObjectName("cardTitle")
        heading.setProperty("class", "cardTitle")
        layout.addWidget(heading)

        self.values = {}
        grid = QGridLayout()
        grid.setHorizontalSpacing(20)
        grid.setVerticalSpacing(9)
        for row, (key, label) in enumerate(rows):
            name = QLabel(label)
            name.setObjectName("muted")
            name.setProperty("class", "muted")
            value = QLabel("—")
            value.setObjectName("smallValue")
            value.setProperty("class", "smallValue")
            value.setAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
            grid.addWidget(name, row, 0)
            grid.addWidget(value, row, 1)
            self.values[key] = value
        layout.addLayout(grid)

    def set_value(self, key, value):
        self.values[key].setText(str(value))


class Dashboard(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Embedded Telemetry | Receiver Monitor")
        self.resize(1120, 780)
        self.setMinimumSize(900, 650)
        self.worker = None
        self.last_record_time = None
        self.valid_record_count = 0

        root = QWidget()
        self.setCentralWidget(root)
        page = QVBoxLayout(root)
        page.setContentsMargins(22, 18, 22, 18)
        page.setSpacing(14)

        # Header
        header = QHBoxLayout()
        title_box = QVBoxLayout()
        title = QLabel("RECEIVER TELEMETRY")
        title.setObjectName("title")
        subtitle = QLabel("NRF24 DATA MONITOR  /  LIVE SERIAL DASHBOARD")
        subtitle.setObjectName("subtitle")
        title_box.addWidget(title)
        title_box.addWidget(subtitle)
        header.addLayout(title_box)
        header.addStretch()

        self.status_dot = QLabel("●")
        self.status_dot.setStyleSheet("color:#ef5350; font-size:25px;")
        self.status_text = QLabel("COMMUNICATION LOST")
        self.status_text.setStyleSheet("font-weight:700; color:#ef5350;")
        status_box = QVBoxLayout()
        status_box.setAlignment(Qt.AlignmentFlag.AlignRight)
        status_box.addWidget(self.status_dot, alignment=Qt.AlignmentFlag.AlignRight)
        status_box.addWidget(self.status_text)
        header.addLayout(status_box)
        page.addLayout(header)

        # Port controls
        controls = QHBoxLayout()
        controls.addWidget(QLabel("Receiver serial port"))
        self.port_combo = QComboBox()
        self.refresh_button = QPushButton("Refresh")
        self.connect_button = QPushButton("Connect")
        self.disconnect_button = QPushButton("Disconnect")
        self.disconnect_button.setEnabled(False)
        controls.addWidget(self.port_combo, 1)
        controls.addWidget(self.refresh_button)
        controls.addWidget(self.connect_button)
        controls.addWidget(self.disconnect_button)
        controls.addWidget(QLabel("9600 baud"))
        page.addLayout(controls)

        # Summary cards
        summary = QGridLayout()
        summary.setHorizontalSpacing(12)
        summary.setVerticalSpacing(12)
        self.sequence_card = ValueCard("Sequence", "—", "Latest record")
        self.temp_card = ValueCard("Temperature", "—", "°C")
        self.humidity_card = ValueCard("Humidity", "—", "%")
        self.voltage_card = ValueCard("Supply voltage", "—", "V")
        for col, card in enumerate(
            [self.sequence_card, self.temp_card, self.humidity_card, self.voltage_card]
        ):
            summary.addWidget(card, 0, col)
        page.addLayout(summary)

        # Main information cards
        details = QGridLayout()
        details.setHorizontalSpacing(12)
        details.setVerticalSpacing(12)

        self.gps_card = InfoCard("GPS position", [
            ("latitude", "Latitude"), ("longitude", "Longitude"),
            ("utc", "UTC time"), ("fix", "Fix quality"),
            ("satellites", "Satellites"), ("hdop", "HDOP"),
            ("altitude", "Altitude"), ("geoid", "Geoid separation"),
        ])
        self.motion_card = InfoCard("Motion / MPU6050", [
            ("pitch", "Pitch"), ("roll", "Roll"), ("yaw", "Yaw"),
            ("mpu_valid", "Sensor validity"),
        ])
        self.environment_card = InfoCard("Environment / DHT11", [
            ("temperature", "Temperature"), ("humidity", "Humidity"),
            ("dht_valid", "Sensor validity"),
        ])
        self.system_card = InfoCard("System information", [
            ("timestamp", "Record timestamp"), ("uptime", "Uptime value"),
            ("voltage", "Supply voltage"), ("status", "Operational status"),
            ("last_received", "Last DATA received"),
        ])

        details.addWidget(self.gps_card, 0, 0, 2, 1)
        details.addWidget(self.motion_card, 0, 1)
        details.addWidget(self.environment_card, 1, 1)
        details.addWidget(self.system_card, 0, 2, 2, 1)
        details.setColumnStretch(0, 1)
        details.setColumnStretch(1, 1)
        details.setColumnStretch(2, 1)
        page.addLayout(details, 1)

        footer = QHBoxLayout()
        self.port_status = QLabel("Not connected")
        self.port_status.setObjectName("muted")
        self.record_status = QLabel("Valid DATA records: 0")
        self.record_status.setObjectName("muted")
        footer.addWidget(self.port_status)
        footer.addStretch()
        footer.addWidget(self.record_status)
        page.addLayout(footer)

        self.refresh_button.clicked.connect(self.refresh_ports)
        self.connect_button.clicked.connect(self.connect_serial)
        self.disconnect_button.clicked.connect(self.disconnect_serial)

        self.watchdog = QTimer(self)
        self.watchdog.setInterval(250)
        self.watchdog.timeout.connect(self.check_communication)
        self.watchdog.start()

        self.refresh_ports()
        self.setStyleSheet(DARK_STYLE)

    def refresh_ports(self):
        current = self.port_combo.currentData()
        self.port_combo.clear()
        ports = list(list_ports.comports())
        for port in ports:
            self.port_combo.addItem(f"{port.device} — {port.description}", port.device)
        if current:
            index = self.port_combo.findData(current)
            if index >= 0:
                self.port_combo.setCurrentIndex(index)
        if not ports:
            self.port_combo.addItem("No serial ports found", None)

    def connect_serial(self):
        port = self.port_combo.currentData()
        if not port:
            QMessageBox.warning(self, "No serial port", "Connect the receiver Arduino and refresh the port list.")
            return

        self.last_record_time = None
        self.set_communication(False, "WAITING FOR DATA")
        self.worker = SerialWorker(port, 9600)
        self.worker.record_received.connect(self.handle_record)
        self.worker.status_message.connect(self.handle_worker_status)
        self.worker.connection_lost.connect(self.handle_connection_lost)
        self.worker.finished.connect(self.worker_finished)
        self.worker.start()

        self.connect_button.setEnabled(False)
        self.disconnect_button.setEnabled(True)
        self.refresh_button.setEnabled(False)
        self.port_combo.setEnabled(False)
        self.port_status.setText(f"Opening {port}…")

    def disconnect_serial(self):
        if self.worker and self.worker.isRunning():
            self.worker.stop()
        self.set_communication(False, "DISCONNECTED")
        self.reset_controls()

    def worker_finished(self):
        self.set_communication(False, "DISCONNECTED")
        self.reset_controls()

    def reset_controls(self):
        self.connect_button.setEnabled(True)
        self.disconnect_button.setEnabled(False)
        self.refresh_button.setEnabled(True)
        self.port_combo.setEnabled(True)
        self.port_status.setText("Not connected")

    def handle_worker_status(self, message):
        if message.startswith("Connected"):
            self.port_status.setText(message)

    def handle_connection_lost(self, message):
        self.set_communication(False, "SERIAL ERROR")
        self.port_status.setText(f"Serial error: {message}")

    def handle_record(self, line):
        try:
            record = parse_data_line(line)
        except DataRecordError as exc:
            self.port_status.setText(f"Rejected DATA line: {exc}")
            return

        self.last_record_time = datetime.now()
        self.valid_record_count += 1
        self.set_communication(True, "COMMUNICATION ACTIVE")
        self.record_status.setText(f"Valid DATA records: {self.valid_record_count}")
        self.sequence_card.set_value(record.sequence)
        self.system_card.set_value("last_received", self.last_record_time.strftime("%H:%M:%S"))

        # Display sensor values only when the corresponding validity flag is set.
        if record.dht_valid:
            self.temp_card.set_value(f"{record.temperature:.2f}")
            self.humidity_card.set_value(f"{record.humidity:.2f}")
            self.environment_card.set_value("temperature", f"{record.temperature:.2f} °C")
            self.environment_card.set_value("humidity", f"{record.humidity:.2f} %")
            self.environment_card.set_value("dht_valid", "VALID")
        else:
            self.temp_card.set_value("INVALID", "DHT11 data")
            self.humidity_card.set_value("INVALID", "DHT11 data")
            self.environment_card.set_value("temperature", "—")
            self.environment_card.set_value("humidity", "—")
            self.environment_card.set_value("dht_valid", "INVALID")

        if record.mpu_valid:
            self.motion_card.set_value("pitch", f"{record.pitch:.2f}°")
            self.motion_card.set_value("roll", f"{record.roll:.2f}°")
            self.motion_card.set_value("yaw", f"{record.yaw:.2f}°")
            self.motion_card.set_value("mpu_valid", "VALID")
        else:
            for key in ("pitch", "roll", "yaw"):
                self.motion_card.set_value(key, "—")
            self.motion_card.set_value("mpu_valid", "INVALID")

        if record.gps_valid:
            self.gps_card.set_value("latitude", record.latitude)
            self.gps_card.set_value("longitude", record.longitude)
            self.gps_card.set_value("utc", record.utc)
            self.gps_card.set_value("fix", f"{record.gps_fix}")
            self.gps_card.set_value("satellites", record.satellites)
            self.gps_card.set_value("hdop", f"{record.hdop:.2f}")
            self.gps_card.set_value("altitude", f"{record.altitude:.2f} m")
            self.gps_card.set_value("geoid", f"{record.geoid_separation:.2f} m")
        else:
            for key in ("latitude", "longitude", "utc", "fix", "satellites", "hdop", "altitude", "geoid"):
                self.gps_card.set_value(key, "—")

        self.voltage_card.set_value(f"{record.voltage:.2f}")
        self.system_card.set_value("timestamp", record.timestamp_text)
        self.system_card.set_value("uptime", f"{record.uptime:.2f}")
        self.system_card.set_value("voltage", f"{record.voltage:.2f} V")
        self.system_card.set_value("status", str(record.operational_status))

    def check_communication(self):
        if self.last_record_time is None:
            return
        elapsed = (datetime.now() - self.last_record_time).total_seconds()
        if elapsed > 5:
            self.set_communication(False, "COMMUNICATION LOST")

    def set_communication(self, active, message):
        if active:
            self.status_dot.setStyleSheet("color:#35d07f; font-size:25px;")
            self.status_text.setStyleSheet("font-weight:700; color:#35d07f;")
        else:
            self.status_dot.setStyleSheet("color:#ef5350; font-size:25px;")
            self.status_text.setStyleSheet("font-weight:700; color:#ef5350;")
        self.status_text.setText(message)

    def closeEvent(self, event):
        if self.worker and self.worker.isRunning():
            self.worker.stop()
        event.accept()
