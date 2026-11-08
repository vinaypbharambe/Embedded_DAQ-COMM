"""Parse the 24-field DATA record emitted by the NRF24 receiver Arduino."""

from dataclasses import dataclass
from datetime import datetime, timezone


class DataRecordError(ValueError):
    """Raised when an incoming DATA line is malformed."""


@dataclass
class DataRecord:
    sequence: int
    unix_timestamp: int
    dht_valid: bool
    temperature: float
    humidity: float
    mpu_valid: bool
    pitch: float
    roll: float
    yaw: float
    gps_valid: bool
    utc: str
    latitude: str
    longitude: str
    gps_fix: int
    satellites: int
    hdop: float
    altitude: float
    geoid_separation: float
    voltage: float
    uptime: float
    operational_status: int

    @property
    def timestamp_text(self):
        try:
            return datetime.fromtimestamp(
                self.unix_timestamp, tz=timezone.utc
            ).astimezone().strftime("%Y-%m-%d %H:%M:%S")
        except (OverflowError, OSError, ValueError):
            return str(self.unix_timestamp)


def _int(value, field):
    try:
        return int(value)
    except (TypeError, ValueError):
        raise DataRecordError(f"Invalid integer in {field}: {value!r}")


def _float(value, field):
    try:
        return float(value)
    except (TypeError, ValueError):
        raise DataRecordError(f"Invalid number in {field}: {value!r}")


def _validity(value, field):
    result = _int(value, field)
    if result not in (0, 1):
        raise DataRecordError(f"{field} must be 0 or 1")
    return bool(result)


def _coordinate(value, direction, field, allowed):
    value = value.strip()
    direction = direction.strip().upper()
    if not value or direction not in allowed:
        raise DataRecordError(f"Invalid {field} coordinate/direction")
    _float(value, field)
    return f"{value}° {direction}"


def _utc_text(value):
    value = value.strip()
    if len(value) < 6 or not value[:6].isdigit():
        return value or "—"
    return f"{value[0:2]}:{value[2:4]}:{value[4:6]} UTC"


def parse_data_line(line: str) -> DataRecord:
    """Parse one complete DATA line. Exactly 24 comma-separated fields expected."""
    fields = line.strip().split(",")

    if not fields or fields[0] != "DATA":
        raise DataRecordError("Line does not begin with DATA")
    if len(fields) != 24:
        raise DataRecordError(
            f"Expected 24 fields, received {len(fields)}"
        )

    return DataRecord(
        sequence=_int(fields[1], "sequence"),
        unix_timestamp=_int(fields[2], "timestamp"),
        dht_valid=_validity(fields[3], "DHT validity"),
        temperature=_float(fields[4], "temperature"),
        humidity=_float(fields[5], "humidity"),
        mpu_valid=_validity(fields[6], "MPU validity"),
        pitch=_float(fields[7], "pitch"),
        roll=_float(fields[8], "roll"),
        yaw=_float(fields[9], "yaw"),
        gps_valid=_validity(fields[10], "GPS validity"),
        utc=_utc_text(fields[11]),
        latitude=_coordinate(fields[12], fields[13], "latitude", {"N", "S"}),
        longitude=_coordinate(fields[14], fields[15], "longitude", {"E", "W"}),
        gps_fix=_int(fields[16], "GPS fix"),
        satellites=_int(fields[17], "satellites"),
        hdop=_float(fields[18], "HDOP"),
        altitude=_float(fields[19], "altitude"),
        geoid_separation=_float(fields[20], "geoid separation"),
        voltage=_float(fields[21], "voltage"),
        uptime=_float(fields[22], "uptime"),
        operational_status=_int(fields[23], "operational status"),
    )
