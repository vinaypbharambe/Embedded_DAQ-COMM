from PyQt6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QGridLayout,
    QLabel,
    QPushButton,
    QGroupBox,
    QTextEdit
)

from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont

from simulator_process import SimulatorProcess
from data_parser import DataParser


class Dashboard(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Embedded Data Acquisition Monitor")
        self.setMinimumSize(1100, 750)

        self.simulator = None
        self.parser = DataParser()

        # Store all start buttons
        self.start_buttons = {}

        self.setStyleSheet("""
            QMainWindow {
                background-color: #101820;
            }

            QLabel {
                color: #FFFFFF;
            }

            QGroupBox {
                background-color: #1B2733;
                border: 1px solid #34495E;
                border-radius: 10px;
                margin-top: 15px;
                padding: 15px;
                color: #00D4FF;
                font-size: 15px;
                font-weight: bold;
            }

            QGroupBox::title {
                subcontrol-origin: margin;
                left: 15px;
                padding: 0 5px;
            }

            QPushButton {
                border: none;
                border-radius: 7px;
                padding: 12px 15px;
                font-size: 12px;
                font-weight: bold;
                color: white;
            }

            QPushButton:disabled {
                background-color: #46515C;
                color: #AAB2B9;
            }

            QTextEdit {
                background-color: #0B1218;
                color: #00FF99;
                border: 1px solid #34495E;
                border-radius: 8px;
                padding: 10px;
                font-family: monospace;
                font-size: 12px;
            }
        """)

        self.init_ui()

    # =====================================
    # MAIN UI
    # =====================================

    def init_ui(self):

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        main_layout = QVBoxLayout(central_widget)
        main_layout.setSpacing(15)

        # HEADER

        header_layout = QHBoxLayout()

        title = QLabel("EMBEDDED DATA ACQUISITION MONITOR")
        title.setFont(QFont("Arial", 20, QFont.Weight.Bold))

        self.system_status = QLabel("● IDLE")

        self.system_status.setStyleSheet(
            "color: #FFA500; font-weight: bold;"
        )

        header_layout.addWidget(title)
        header_layout.addStretch()
        header_layout.addWidget(self.system_status)

        main_layout.addLayout(header_layout)

        # =====================================
        # TEST CONTROLS
        # =====================================

        control_group = QGroupBox("TEST CONTROLS")

        control_layout = QGridLayout()
        control_layout.setSpacing(10)

        # NORMAL TEST

        self.normal_button = QPushButton(
            "▶ NORMAL TEST"
        )

        self.normal_button.setStyleSheet(
            "background-color: #168A52;"
        )

        # DEMO MODE

        self.demo_button = QPushButton(
            "🎬 DEMO MODE"
        )

        self.demo_button.setStyleSheet(
            "background-color: #2563EB;"
        )

        # DHT11 ERROR

        self.dht_error_button = QPushButton(
            "🌡 DHT11 FAILURE"
        )

        self.dht_error_button.setStyleSheet(
            "background-color: #D97706;"
        )

        # MPU6050 ERROR

        self.mpu_error_button = QPushButton(
            "📐 MPU6050 FAILURE"
        )

        self.mpu_error_button.setStyleSheet(
            "background-color: #D97706;"
        )

        # GPS ERROR

        self.gps_error_button = QPushButton(
            "📡 GPS FAILURE"
        )

        self.gps_error_button.setStyleSheet(
            "background-color: #D97706;"
        )

        # COMMUNICATION ERROR

        self.communication_error_button = QPushButton(
            "🔌 COMMUNICATION FAILURE"
        )

        self.communication_error_button.setStyleSheet(
            "background-color: #D97706;"
        )

        # STOP

        self.stop_button = QPushButton(
            "■ STOP"
        )

        self.stop_button.setStyleSheet(
            "background-color: #C0392B;"
        )

        self.stop_button.setEnabled(False)

        # ADD BUTTONS TO GRID

        control_layout.addWidget(
            self.normal_button, 0, 0
        )

        control_layout.addWidget(
            self.demo_button, 0, 1
        )

        control_layout.addWidget(
            self.dht_error_button, 0, 2
        )

        control_layout.addWidget(
            self.mpu_error_button, 0, 3
        )

        control_layout.addWidget(
            self.gps_error_button, 1, 0
        )

        control_layout.addWidget(
            self.communication_error_button, 1, 1, 1, 2
        )

        control_layout.addWidget(
            self.stop_button, 1, 3
        )

        control_group.setLayout(control_layout)

        main_layout.addWidget(control_group)

        # STORE START BUTTONS

        self.start_buttons = {
            self.normal_button: "--normal",
            self.demo_button: "--demo",
            self.dht_error_button: "--error-dht11",
            self.mpu_error_button: "--error-mpu6050",
            self.gps_error_button: "--error-gps",
            self.communication_error_button: "--error-communication"
        }

        # CONNECT BUTTONS

        for button, mode in self.start_buttons.items():

            button.clicked.connect(
                lambda checked=False, selected_mode=mode:
                    self.start_simulator(selected_mode)
            )

        self.stop_button.clicked.connect(
            self.stop_test
        )

        # =====================================
        # SENSOR CARDS
        # =====================================

        sensor_layout = QGridLayout()

        self.dht11_card = self.create_sensor_card(
            "DHT11 - ENVIRONMENT",
            [
                ("Temperature", "temperature", "°C"),
                ("Humidity", "humidity", "%"),
                ("Sensor Status", "dht_status", "")
            ]
        )

        self.mpu_card = self.create_sensor_card(
            "MPU6050 - ORIENTATION",
            [
                ("Pitch", "pitch", "°"),
                ("Roll", "roll", "°"),
                ("Yaw", "yaw", "°"),
                ("Sensor Status", "mpu_status", "")
            ]
        )

        self.gps_card = self.create_sensor_card(
            "GPS - LOCATION",
            [
                ("Latitude", "latitude", ""),
                ("Longitude", "longitude", ""),
                ("Satellites", "satellites", ""),
                ("GPS Status", "gps_status", "")
            ]
        )

        sensor_layout.addWidget(self.dht11_card, 0, 0)
        sensor_layout.addWidget(self.mpu_card, 0, 1)
        sensor_layout.addWidget(self.gps_card, 0, 2)

        main_layout.addLayout(sensor_layout)

        # =====================================
        # SYSTEM INFORMATION
        # =====================================

        system_group = QGroupBox("SYSTEM INFORMATION")

        system_layout = QGridLayout()

        self.voltage_value = self.create_value_label("N/A")
        self.uptime_value = self.create_value_label("N/A")
        self.device_status = self.create_value_label("IDLE")

        system_layout.addWidget(QLabel("Supply Voltage"), 0, 0)
        system_layout.addWidget(self.voltage_value, 1, 0)

        system_layout.addWidget(QLabel("Uptime"), 0, 1)
        system_layout.addWidget(self.uptime_value, 1, 1)

        system_layout.addWidget(QLabel("Device Status"), 0, 2)
        system_layout.addWidget(self.device_status, 1, 2)

        system_group.setLayout(system_layout)

        main_layout.addWidget(system_group)

        # =====================================
        # COMMUNICATION
        # =====================================

        communication_group = QGroupBox(
            "COMMUNICATION & BUFFER"
        )

        communication_layout = QGridLayout()

        self.communication_status = self.create_value_label("IDLE")
        self.records_collected = self.create_value_label("0")
        self.records_transmitted = self.create_value_label("0")
        self.buffer_occupancy = self.create_value_label("0 / 5")
        self.dropped_records = self.create_value_label("0")

        items = [
            ("Communication", self.communication_status),
            ("Records Collected", self.records_collected),
            ("Records Transmitted", self.records_transmitted),
            ("Buffer Occupancy", self.buffer_occupancy),
            ("Dropped Records", self.dropped_records)
        ]

        for column, (name, value) in enumerate(items):

            communication_layout.addWidget(
                QLabel(name), 0, column
            )

            communication_layout.addWidget(
                value, 1, column
            )

        communication_group.setLayout(communication_layout)

        main_layout.addWidget(communication_group)

        # =====================================
        # EVENT LOG
        # =====================================

        log_group = QGroupBox("EVENT LOG")

        log_layout = QVBoxLayout()

        self.event_log = QTextEdit()

        self.event_log.setReadOnly(True)

        self.event_log.setPlaceholderText(
            "Simulator events will appear here..."
        )

        self.event_log.setMaximumHeight(150)

        log_layout.addWidget(self.event_log)

        log_group.setLayout(log_layout)

        main_layout.addWidget(log_group)

        # INITIAL LOG

        self.add_log("Dashboard initialized.")
        self.add_log("Waiting for simulator.")

    # =====================================
    # SENSOR CARD
    # =====================================

    def create_sensor_card(self, title, fields):

        group = QGroupBox(title)

        layout = QVBoxLayout()

        for field_name, attribute_name, unit in fields:

            row = QHBoxLayout()

            name_label = QLabel(field_name)

            value_label = self.create_value_label("N/A")

            setattr(self, attribute_name, value_label)

            row.addWidget(name_label)
            row.addStretch()
            row.addWidget(value_label)

            if unit:
                row.addWidget(QLabel(unit))

            layout.addLayout(row)

        group.setLayout(layout)

        return group

    # =====================================
    # VALUE LABEL
    # =====================================

    def create_value_label(self, value):

        label = QLabel(value)

        label.setFont(
            QFont("Arial", 13, QFont.Weight.Bold)
        )

        label.setStyleSheet(
            "color: #00D4FF;"
        )

        return label

    # =====================================
    # LOG
    # =====================================

    def add_log(self, message):

        self.event_log.append(message)

    # =====================================
    # START SIMULATOR
    # =====================================

    def start_simulator(self, mode):

        # Prevent multiple processes

        if self.simulator is not None:

            if self.simulator.isRunning():

                self.add_log(
                    "A simulator is already running."
                )

                return

        # Reset parser and dashboard

        self.parser = DataParser()

        self.reset_dashboard()

        # Disable all start buttons

        for button in self.start_buttons:

            button.setEnabled(False)

        self.stop_button.setEnabled(True)

        # MODE INFORMATION

        mode_names = {
            "--normal": "NORMAL TEST",
            "--demo": "DEMO MODE",
            "--error-dht11": "DHT11 FAILURE",
            "--error-mpu6050": "MPU6050 FAILURE",
            "--error-gps": "GPS FAILURE",
            "--error-communication": "COMMUNICATION FAILURE"
        }

        mode_name = mode_names.get(
            mode,
            "UNKNOWN MODE"
        )

        self.system_status.setText(
            f"● {mode_name}"
        )

        if mode == "--normal":

            self.system_status.setStyleSheet(
                "color: #00FF99; font-weight: bold;"
            )

        elif mode == "--demo":

            self.system_status.setStyleSheet(
                "color: #60A5FA; font-weight: bold;"
            )

        else:

            self.system_status.setStyleSheet(
                "color: #FFA500; font-weight: bold;"
            )

        self.add_log(
            f"Starting {mode_name}..."
        )

        # CREATE SIMULATOR PROCESS

        self.simulator = SimulatorProcess(mode)

        self.simulator.output_line.connect(
            self.process_output
        )

        self.simulator.process_finished.connect(
            self.process_finished
        )

        self.simulator.process_error.connect(
            self.process_error
        )

        self.simulator.start()

    # =====================================
    # PROCESS C OUTPUT
    # =====================================

    def process_output(self, line):

        events = self.parser.feed_line(line)

        for event in events:

            event_type = event["type"]

            if event_type == "record":

                self.update_sensor_values(
                    event["data"]
                )

            elif event_type == "collected":

                self.records_collected.setText(
                    str(event["value"])
                )

            elif event_type == "transmitted":

                self.records_transmitted.setText(
                    str(event["value"])
                )

            elif event_type == "buffer":

                self.buffer_occupancy.setText(
                    f"{event['occupancy']} / {event['capacity']}"
                )

            elif event_type == "communication":

                status = event["status"]

                self.communication_status.setText(status)

                if status == "CONNECTED":

                    self.communication_status.setStyleSheet(
                        "color: #00FF99;"
                    )

                else:

                    self.communication_status.setStyleSheet(
                        "color: #FF4444;"
                    )

            elif event_type == "log":

                self.add_log(
                    event["message"]
                )

    # =====================================
    # UPDATE SENSOR VALUES
    # =====================================

    def update_sensor_values(self, data):

        def display(value, unit=""):

            if value is None:
                return "N/A"

            return f"{value}{unit}"

        # DHT11

        self.temperature.setText(
            display(data["temperature"])
        )

        self.humidity.setText(
            display(data["humidity"])
        )

        self.dht_status.setText(
            data["dht_status"]
        )

        # MPU6050

        self.pitch.setText(
            display(data["pitch"])
        )

        self.roll.setText(
            display(data["roll"])
        )

        self.yaw.setText(
            display(data["yaw"])
        )

        self.mpu_status.setText(
            data["mpu_status"]
        )

        # GPS

        self.latitude.setText(
            display(data["latitude"])
        )

        self.longitude.setText(
            display(data["longitude"])
        )

        self.satellites.setText(
            display(data["satellites"])
        )

        self.gps_status.setText(
            data["gps_status"]
        )

        # SYSTEM

        self.voltage_value.setText(
            display(data["voltage"], " V")
        )

        self.uptime_value.setText(
            display(data["uptime"], " sec")
        )

        self.device_status.setText(
            data["device_status"]
        )

    # =====================================
    # RESET DASHBOARD
    # =====================================

    def reset_dashboard(self):

        for attribute in [
            "temperature",
            "humidity",
            "pitch",
            "roll",
            "yaw",
            "latitude",
            "longitude",
            "satellites"
        ]:

            getattr(self, attribute).setText("N/A")

        self.dht_status.setText("UNKNOWN")
        self.mpu_status.setText("UNKNOWN")
        self.gps_status.setText("UNKNOWN")

        self.voltage_value.setText("N/A")
        self.uptime_value.setText("N/A")
        self.device_status.setText("N/A")

        self.communication_status.setText("IDLE")

        self.records_collected.setText("0")
        self.records_transmitted.setText("0")
        self.buffer_occupancy.setText("0 / 5")
        self.dropped_records.setText("0")

        self.event_log.clear()

    # =====================================
    # STOP TEST
    # =====================================

    def stop_test(self):

        if self.simulator is not None:

            if self.simulator.isRunning():

                self.add_log(
                    "Stopping simulator..."
                )

                self.simulator.stop()

        self.system_status.setText(
            "● STOPPING"
        )

        self.system_status.setStyleSheet(
            "color: #FFA500; font-weight: bold;"
        )

        self.stop_button.setEnabled(False)

    # =====================================
    # ENABLE CONTROLS
    # =====================================

    def enable_controls(self):

        for button in self.start_buttons:

            button.setEnabled(True)

        self.stop_button.setEnabled(False)

    # =====================================
    # PROCESS FINISHED
    # =====================================

    def process_finished(self, return_code):

        if return_code == 0:

            self.add_log(
                "Simulator completed successfully."
            )

        else:

            self.add_log(
                f"Simulator stopped or exited with code {return_code}."
            )

        self.system_status.setText(
            "● IDLE"
        )

        self.system_status.setStyleSheet(
            "color: #FFA500; font-weight: bold;"
        )

        self.enable_controls()

        self.simulator = None

    # =====================================
    # PROCESS ERROR
    # =====================================

    def process_error(self, error):

        self.add_log(
            f"ERROR: {error}"
        )

        self.system_status.setText(
            "● ERROR"
        )

        self.system_status.setStyleSheet(
            "color: #FF4444; font-weight: bold;"
        )

        self.enable_controls()

        self.simulator = None

    # =====================================
    # CLOSE WINDOW
    # =====================================

    def closeEvent(self, event):

        if self.simulator is not None:

            if self.simulator.isRunning():

                self.simulator.stop()
                self.simulator.wait(3000)

        event.accept()
