# AR-Enabled Fault Detection and Digital Twin Sync for Industrial IoT Systems

## 📌 Project Overview
This project implements an Industrial IoT (IIoT) system designed to monitor machinery health, detect faults in real-time, and synchronize this data with a Digital Twin. The system leverages Augmented Reality (AR) to visualize sensor data and error states directly onto physical equipment, facilitating easier maintenance and monitoring.

The architecture consists of a sensor node (Arduino Nano) collecting physical data and a gateway node (ESP32) that processes faults and synchronizes with the AR interface.

## 📂 File Structure

| File Name | Description |
| :--- | :--- |
| **`nano_transmit_code.ino`** | **Transmitter Node (Sensor Edge):** Runs on the Arduino Nano. Captures raw data from connected sensors (e.g., vibration, temperature) and transmits it wirelessly to the central unit. |
| **`esp32_receive_code.ino`** | **Receiver/Gateway Node:** Runs on the ESP32. Receives telemetry from the Nano, processes data for the Digital Twin, and handles connectivity (Wi-Fi/Bluetooth) to the AR application. |
| **`error_code.ino`** | **Fault Logic:** Helper module that defines specific error thresholds. It analyzes incoming data to trigger specific error codes when anomalies are detected. |
| **`demo_data_code_v2.zip`** | **Test Data:** Archive containing sample datasets or previous code versions used for testing the Digital Twin synchronization without live hardware. |

## ⚙️ Hardware Requirements
* **Microcontrollers:**
  * Arduino Nano (Sensor Node)
  * ESP32 Development Board (Gateway & Cloud Sync)
* **Communication Modules:**
  * Wireless Transceivers (e.g., nRF24L01, HC-12, or LoRa) between Nano and ESP32.
* **Sensors (Typical):**
  * IMU/Accelerometers (for vibration analysis)
  * Temperature Sensors (for overheating detection)

## 🚀 Getting Started

### 1. Hardware Setup
* Connect your industrial sensors to the **Arduino Nano**.
* Establish the wireless link between the Nano and the **ESP32**.
* Ensure the ESP32 has network access if the Digital Twin is cloud-hosted.

### 2. Installation & Upload
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/vedantjadhav304/AR-Enabled-Fault-Detection-and-Digital-Twin-Sync-for-Industrial-IoT-Systems.git](https://github.com/vedantjadhav304/AR-Enabled-Fault-Detection-and-Digital-Twin-Sync-for-Industrial-IoT-Systems.git)
