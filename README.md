# 🚀 Automation 4.0: AR-Enabled Fault Detection & Digital Twin Sync

**Authors:** Vedant, Abhishek, Amey

Welcome to the internet's first end-to-end tutorial on **Automation 4.0**. Unlike standard IoT projects, this system bridges the gap between physical hardware, edge intelligence, and immersive visualization. We go beyond basic robotics to build an industrial-grade ecosystem featuring **Digital Twins** and **Augmented Reality (AR)**.


![WhatsApp Image 2025-11-30 at 04 53 52_0c020797](https://github.com/user-attachments/assets/75c3f658-fd43-4cdd-999a-de7a63e3b1ad)



## 📖 Project Overview

This project enables you to monitor a 6 DoF robotic arm using a real-time virtual counterpart (Digital Twin) and diagnostic overlays (AR).

* **Digital Twin:** A live simulation receiving continuous data streams (position, velocity, torque) to monitor performance in a safe virtual environment.
* **Augmented Reality:** Overlays live diagnostics (heat maps, error codes) directly onto the physical robot via a mobile/tablet view.
* **Edge Intelligence:** Instead of sending raw data to the cloud, we process faults locally on the chip for real-time alerts.

---

## 🏗️ Architecture Phases

The project is divided into three distinct phases:

### Phase 1: The Hardware (6 DoF Robotic Arm)
Development of a physical 6 Degrees of Freedom arm using 3D printed components.
* **Key Innovation:** We "hack" standard hobby servos to expose their internal potentiometers, converting open-loop systems into **closed-loop feedback systems**. This allows us to know the *exact* physical position of the arm, not just the target position.

### Phase 2: The Edge Layer (ESP32)
The intelligence layer. The ESP32 taps into the robot's communication line to "sniff" data packets.
* **Features:** Real-time Edge Fault Detection (Stall, Noise, Range limits), Binary Data Ingestion, and high-speed HTTP/UDP sync to Firebase.

### Phase 3: The Application Layer (Unity)
The user interface and backend.
* **Features:** A robust Unity system that synchronizes a 3D model with the physical robot via Firebase and visualizes internal data using AR.

---

## 🛠️ Phase 1: Hardware Setup & Wiring

### Prerequisite: Servo Modification
Standard servos do not provide position feedback. You must modify your MG995/SG90 servos:
1.  Open the servo case.
2.  Solder a feedback wire to the **middle pin (wiper)** of the internal potentiometer.
3.  Route the wire out and reassemble.



### Wiring Schematic
Connect the components as follows using an Arduino Nano, PCA9685 Driver, and Logic Level Converter (LLC).

**1. Power Supply (PCA9685)**
| Component | Connection |
| :--- | :--- |
| Battery (+) Red | PCA9685 Terminal (+) |
| Battery (-) Black | PCA9685 Terminal (-) |

**2. Logic Level Converter (LLC)**
*Voltage Reference:*
* ESP32 3.3V → LLC LV
* Arduino 5V → LLC HV
* GND → LLC GND

*Data Transmission:*
* ESP32 TX → LLC LV1 → LLC HV1 → Arduino RX (D0)
* Arduino TX (D1) → LLC HV2 → LLC LV2 → ESP32 RX

**3. Servo Driver (PCA9685)**
| Arduino Pin | PCA9685 Pin |
| :--- | :--- |
| 5V | VCC |
| GND | GND |
| A4 | SDA |
| A5 | SCL |

**4. Feedback Wires (Analog Inputs)**
| Servo | Arduino Pin |
| :--- | :--- |
| Base | A0 |
| Shoulder | A1 |
| Elbow | A2 |
| Wrist Pitch | A3 |
| Wrist Roll | A6 |
| Gripper | A7 |

---

## 💻 Phase 2: Firmware & Edge Intelligence

### 1. Arduino Nano (Transmitter)
Upload `nano_transmit_code.ino`.
* **Function:** Runs pick-and-place logic, captures target vs. actual servo positions, and sends data arrays (`[s0...s6]`) via UART/RS485.

### 2. ESP32 (Receiver & Gateway)
Upload `esp32_recieve_code.ino`.
* **Prerequisite:** Create a Firebase Project, enable Email/Password Auth, and Realtime Database.
* **Function:**
    * **Binary Ingestion:** Reads 50 bytes of raw binary data (Header `0xAA`).
    * **Fault Detection:**
        * `ERR_STALL (2)`: Motor powered but stationary > 400ms.
        * `ERR_NOISE (4)`: Impossible position jumps.
        * `ERR_RANGE (5)`: Values outside physical limits.
    * **Cloud Sync:** Uploads JSON strings to Firebase every 500ms.



---

## 📱 Phase 3: Unity & Digital Twin

### Requirements
* Unity 2021.3 LTS (or newer)
* Firebase Unity SDK (`FirebaseDatabase.unitypackage`)
* `google-services-desktop.json` (placed in `Assets/StreamingAssets`)

### Setup Steps
1.  **Import Model:** Drag the Robot Arm FBX into `Assets/Models`.
2.  **Hierarchy:** Create a nested hierarchy (Base → Joint1 → Joint2...).
3.  **Scripts:**
    * `FirebaseReader.cs`: Connects to Firebase and deserializes JSON into a public float array.
    * `RobotJointController.cs`: Reads the array from the reader and applies `Quaternion.Euler` rotations to the 3D joints.

**Expected Database Structure:**
```json
{
  "robot": {
    "values": [30, 45, 10, 90, 0, 0]
  }
}
