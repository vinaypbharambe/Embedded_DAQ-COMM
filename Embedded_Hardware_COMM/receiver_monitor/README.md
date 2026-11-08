# Receiver Telemetry Monitor

A standalone PyQt6 dashboard for the Receiver Arduino. It reads the reconstructed
`DATA,...` line over USB serial at **9600 baud**.

## 1. Install dependencies (Ubuntu)

```bash
cd receiver_monitor
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

If PyQt6 installation reports missing system packages, install the Ubuntu Python
Qt dependencies available for your release, or use a Python version supported by
the installed PyQt6 wheel.

## 2. Serial permissions

Check the receiver Arduino port:

```bash
ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
```

If permission is denied, add your user to the `dialout` group:

```bash
sudo usermod -aG dialout "$USER"
```

Log out and log back in for the group change to take effect.

Close Arduino IDE Serial Monitor or any other application using the receiver port.

## 3. Run

```bash
python3 main.py
```

Select the receiver Arduino port and click **Connect**.

## DATA record

The parser expects exactly 24 comma-separated fields:

```text
DATA,sequence,timestamp,dht_valid,temp,humidity,mpu_valid,pitch,roll,yaw,gps_valid,utc,latitude,lat_dir,longitude,lon_dir,fix,satellites,hdop,altitude,geoid,voltage,uptime,status
```

Only complete lines beginning with `DATA,` are processed. Receiver startup messages
are ignored. Communication turns green after a valid DATA record and turns red
when no valid DATA record has arrived for more than five seconds.

Sensor values are shown only when their corresponding validity flag is `1`.
Operational status and uptime are displayed as raw values; their meanings depend
on the transmitter-side definitions.
