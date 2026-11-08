# Embedded_DAQ-COMM

Embedded Simulator and Comm system based on Standard C and POSIX libraries.

The Project is divided into two main parts:

### 1. Embedded Simulator
* **a)** Simulating Mock data of DHT11, MPU6050 and GPS Receiver as sensors and Use System info mock data as inputs to Simulate an Embedded DAQ system.
* **b)** Simulating Errors in Each individual Sensors, pseudo serial data stream and data buffer and handling it using data parser.
* **c) Bonus:** Graphical User Interface (GUI) to Monitor the Data acquired at Sender. A PyQT6 based simple GUI along with normal test and Simulating Errors.

### 2. Bonus: Actual DAQ and Communication Systems with Telematics
* **a)** Use of 3 sensors/dev boards connected to 3 COM ports emulating the communication interface while still generating random data.
* **b)** Interfacing heterogeneous architecture in devboards namely AVR, ARM and Rockchip.
* **c)** Ultimately, Communicating the data stream actually over NRF24 Module from one terminal to another upto 1 km.
* **d)** A simple GUI on receiver side to Visualize the communication and data.

---

## System Architecture

### 1. Embedded Simulator Architecture
![System Architecture Diagram](https://github.com/vinaypbharambe/Embedded_DAQ-COMM/blob/main/embedded%20simulator.png)

### 2. Hardware based DAQ and COMM
![System Architecture Diagram](https://github.com/vinaypbharambe/Embedded_DAQ-COMM/blob/main/hardware%20daq.png)



## Run Information

### A. Embedded Simulator

1. **Operating System Requirement**  
   It is important to have a Linux-based system for this project. I have setup an Ubuntu 24.04-based Virtual Machine to run the required program.

2. **Toolchain Setup**  
   It is important to have the required tools such as `build-essential`, `cmake`, and other make tools for building the C applications. I use my standard installation packages given in `install.x`:
   ```bash
   chmod +x install.x
   ./install.x
   ```

3. **Python & GUI Dependencies**  
   It is important to have a Python 3 and PyQt6 installation to setup the GUI for both the Sender and Receiver side. Use `pip` to install the required packages onto your base or Anaconda Python environments:
   ```bash
   pip install -r requirements.txt
   ```

4. **Compiling the Embedded Simulator**  
   Once everything is set, navigate to the emulator directory and compile the program:
   ```bash
   cd Embedded_Soft_Simulator/just_emulator
   make clean
   make
   ```

5. **Executable Verification**  
   Notice that a new executable will be created in the same directory named: `embedded_simulator`.

6. **Running the Simulator**  
   Run the application from your terminal:
   ```bash
   ./embedded_simulator
   ```
   Observe that the output of the terminal will show readings for the DHT11, MPU6050, GPS, and the System info. By default, this mode runs in `--normal` mode.

7. **Error Inducing Options**  
   We also have error-inducing options available:
   * `--normal`
   * `--demo`
   * `--error-dht11`
   * `--error-mpu6050`
   * `--error-gps`
   * `--error-communication`

   Use them accordingly to introduce a fault. For example:
   ```bash
   ./embedded_simulator --error-dht11
   ```

   ![System Architecture Diagram](just_emulator.png)

8. **Running the Sender GUI**  
   In order to observe the same data stream through the graphical interface, run the following commands:
   ```bash
   cd Sender_GUI
   python main.py
   ```

   ![System Architecture Diagram](SenderGUI.png)

     
### B. Hardware DAQ and COMM

1. **Hardware Connection**  
   Connect all three physical devices to your system's USB ports.

2. **Virtual Machine Configuration (Optional)**  
   If you are running this inside a VirtualBox environment, make sure to pass through and activate the USB devices via the **Devices ➔ USB** menu tab at the top.

3. **Compiling the Hardware DAQ Application**  
   Navigate to the total sensor integration directory and compile the program:
   ```bash
   cd Embedded_Hardware_COMM/total_sensor_integration
   make clean
   make
   ```

4. **Executable Verification**  
   Notice that a new executable will be created in the same directory named: `embedded_simulator`.

5. **Identify Device Addresses**  
   Find the serial port addresses assigned to your connected hardware by running:
   ```bash
   ls /dev/tty*
   ```

6. **Device Ordering Constraint**  
   Ensure you organize and pass the addresses in the exact order of the sensor boards: **DHT11 (Arduino Uno)**, **MPU6050 (STM32)**, and **GPS (Vicharak Shrike Lite)**.

7. **Running the Application**  
   Run the compiled binary while providing your exact system device addresses as arguments:
   ```bash
   ./embedded_simulator /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0
   ```
   *(Be sure to swap out `/dev/ttyUSBX` with the matching device ports you discovered in Step 5)*

8. **Verifying Terminal Output**  
   Observe that the output of the terminal will show live readings for the DHT11, MPU6050, GPS, and System info. By default, this mode runs in `--normal` mode, and there is no manual error simulation mode here.

   ![System Architecture Diagram](just_emulator.png)

9. **Hardware Status Check**  
   Observe the serial communication (TX/RX) LEDs blinking on the Arduino Uno as new telemetry data is successfully received at 1-second intervals.

10. **Setting up the Receiver Monitor**  
    On a second computer, connect your Receiver NRF24 hardware module and fire up the graphical receiver monitor:
    ```bash
    cd "receiver monitor"
    python main.py
    ```

11. **Connecting via the Interface**  
    Select the correct Serial/COM port connected to your Arduino from the GUI dropdown menu and click **Connect**.

12. **Live Data Visualization**  
    You will see the incoming sensor stream start to populate live on your interface dashboard.

    ![Receiver Monitor](receiver_monitor.png)

---

### Hardware Reference Diagrams

#### NRF24 Circuit Schematic
![NRF24 Circuit Diagram](NRF24.png)

#### Final Hardware Setup & Connections
![Hardware Setup Connections](hardware.png)


