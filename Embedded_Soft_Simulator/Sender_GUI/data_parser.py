import re


class DataParser:

    def __init__(self):

        self.current_record = None

        self.records_collected = 0
        self.records_transmitted = 0
        self.dropped_records = 0

    def feed_line(self, line):

        line = line.strip()

        events = []

        if not line:
            return events

        # =====================================
        # COMMUNICATION EVENTS
        # =====================================

        if "[COMMUNICATION]" in line:

            if "Receiver available" in line:
                events.append({
                    "type": "communication",
                    "status": "CONNECTED"
                })

            elif "Receiver unavailable" in line:
                events.append({
                    "type": "communication",
                    "status": "DISCONNECTED"
                })

            elif "Pending records delivered" in line:
                events.append({
                    "type": "log",
                    "message": "Pending records delivered."
                })

            return events

        # =====================================
        # RECORD COLLECTION
        # =====================================

        match = re.search(
            r"Record\s+(\d+)\s+collected and buffered",
            line
        )

        if match:
            self.records_collected = int(match.group(1))

            events.append({
                "type": "collected",
                "value": self.records_collected
            })

            events.append({
                "type": "log",
                "message": f"Record {self.records_collected} collected."
            })

            return events

        # =====================================
        # BUFFER OCCUPANCY
        # =====================================

        match = re.search(
            r"Current buffer occupancy:\s*(\d+)/(\d+)",
            line
        )

        if match:
            events.append({
                "type": "buffer",
                "occupancy": int(match.group(1)),
                "capacity": int(match.group(2))
            })

            return events

        # =====================================
        # RECORD TRANSMISSION
        # =====================================

        if "MOCK RECEIVER: DATA RECORD DELIVERED" in line:

            self.current_record = {
                "record_number": 0,
                "timestamp": "N/A",

                "temperature": None,
                "humidity": None,
                "dht_status": "UNKNOWN",

                "pitch": None,
                "roll": None,
                "yaw": None,
                "mpu_status": "UNKNOWN",

                "latitude": None,
                "longitude": None,
                "satellites": None,
                "fix_quality": None,
                "hdop": None,
                "altitude": None,
                "gps_status": "UNKNOWN",

                "voltage": None,
                "uptime": None,
                "device_status": "UNKNOWN"
            }

            return events

        # Ignore sensor details if no record is being processed
        if self.current_record is None:
            return events

        record = self.current_record

        # =====================================
        # GENERAL RECORD INFORMATION
        # =====================================

        match = re.search(r"Record Number:\s*(\d+)", line)

        if match:
            record["record_number"] = int(match.group(1))

        if line.startswith("Timestamp:"):
            record["timestamp"] = line.split(":", 1)[1].strip()

        # =====================================
        # DHT11 SENSOR
        # =====================================

        match = re.search(
            r"Temperature\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["temperature"] = float(match.group(1))

        match = re.search(
            r"Humidity\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["humidity"] = float(match.group(1))

        if "Sensor Status: VALID" in line:
            if record["dht_status"] == "UNKNOWN":
                record["dht_status"] = "VALID"

            elif record["mpu_status"] == "UNKNOWN":
                record["mpu_status"] = "VALID"

        if "Sensor Status: INVALID / UNAVAILABLE" in line:

            if record["dht_status"] == "UNKNOWN":
                record["dht_status"] = "ERROR"

                record["temperature"] = None
                record["humidity"] = None

            elif record["mpu_status"] == "UNKNOWN":
                record["mpu_status"] = "ERROR"

                record["pitch"] = None
                record["roll"] = None
                record["yaw"] = None

        # =====================================
        # MPU6050 SENSOR
        # =====================================

        match = re.search(
            r"Pitch\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["pitch"] = float(match.group(1))

        match = re.search(
            r"Roll\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["roll"] = float(match.group(1))

        match = re.search(
            r"Yaw\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["yaw"] = float(match.group(1))

        # =====================================
        # GPS
        # =====================================

        match = re.search(
            r"Latitude\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["latitude"] = float(match.group(1))

        match = re.search(
            r"Longitude\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["longitude"] = float(match.group(1))

        match = re.search(r"Satellites\s*:\s*(\d+)", line)

        if match:
            record["satellites"] = int(match.group(1))

        match = re.search(r"Fix Quality:\s*(\d+)", line)

        if match:
            record["fix_quality"] = int(match.group(1))

        match = re.search(
            r"HDOP\s*:\s*(\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["hdop"] = float(match.group(1))

        match = re.search(
            r"Altitude\s*:\s*(-?\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["altitude"] = float(match.group(1))

        if "GPS Status: VALID FIX" in line:
            record["gps_status"] = "VALID FIX"

        elif "GPS Status: NO VALID FIX" in line:
            record["gps_status"] = "NO FIX"

            record["latitude"] = None
            record["longitude"] = None
            record["satellites"] = None

        # =====================================
        # SYSTEM INFORMATION
        # =====================================

        match = re.search(
            r"Supply Voltage\s*:\s*(\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["voltage"] = float(match.group(1))

        match = re.search(
            r"Uptime\s*:\s*(\d+(?:\.\d+)?)",
            line
        )

        if match:
            record["uptime"] = float(match.group(1))

        if line.startswith("Device Status"):
            record["device_status"] = line.split(":", 1)[1].strip()

        # =====================================
        # RECORD COMPLETED
        # =====================================

        if line.startswith("Device Status"):

            self.records_transmitted += 1

            events.append({
                "type": "record",
                "data": record.copy()
            })

            events.append({
                "type": "transmitted",
                "value": self.records_transmitted
            })

            self.current_record = None

            events.append({
                "type": "log",
                "message": (
                    f"Record {record['record_number']} delivered "
                    f"to receiver."
                )
            })

        return events
