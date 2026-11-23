# ⚡ IoT Smart Prepaid Electricity Meter

A real-time, cloud-connected prepaid electricity metering system built using an **Arduino Uno**, **ESP32**, **INA219 Current Sensor**, and the **Blynk IoT** platform. 

Unlike basic timer-based simulations, this system measures actual real-time voltage, current, and power draw from a physical load (5 LED "households") and deducts prepaid credit units proportionally over time. When the balance hits zero—or if the admin toggles the remote master switch—the relay cuts power to the load until a new credit reload is submitted via the mobile app.

---

## ✨ Key Features

* **Real-Time Power Sensing:** Uses an INA219 DC current sensor to measure live bus voltage ($V$), current draw ($A$), real-time wattage ($W$), and accumulated Watt-hours ($Wh$).
* **Usage-Based Credit Deduction:** Dynamically calculates energy consumption ($Power \times Time$) to deplete prepaid balance units accurately based on active electrical load.
* **Zero-Balance Security Lockout:** Powers on with a `0.0` unit balance, requiring an administrator to load credits via the Blynk app before electricity is enabled.
* **Automated Cutoff & Remote Override:** Controls a 5V relay module to physically cut power when credits run out or when toggled off remotely via the Blynk app.
* **Visual & Mobile Alerts:** 
  * Local **16x2 I²C LCD** displays remaining units and system status (`Usage: Normal`, `Balance LOW!`, `EMPTY UNITS`).
  * **Red Warning LED** blinks when balance drops below the warning threshold (`<= 1.0` units) and stays solid when power is cut.
  * **Blynk Push Notifications** alert the user when the balance reaches zero.

---

## 🛠️ Hardware Components

* **Microcontrollers:** 1x Arduino Uno, 1x ESP32 DevKit V1
* **Sensors & Display:** 1x INA219 High-Side DC Current Sensor, 1x 16x2 LCD with I²C Backpack
* **Actuators & Indicators:** 1x 5V Relay Module, 5x House Load LEDs, 1x Red Warning LED
* **Passive Components:** 6x $220\ \Omega$ Resistors
* **Power & Prototyping:** 1x 9V Wall Adapter, 1x Breadboard Power Supply Module (set to 5V), 2x Breadboards, Jumper Wires

---

## 🔌 Wiring & Pinout Guide

### 1. Power & Ground Distribution
* Set the Breadboard Power Supply Module to **5V** and power both the Arduino Uno (`5V`) and ESP32 (`VIN`).
* **Common Ground:** Connect the `GND` rails of both breadboards, the Arduino Uno, and the ESP32 together. Serial communication and sensor readings will fail without a shared ground.

### 2. Microcontroller Serial Bridge (Cross-Wired)
| Arduino Uno Pin | ESP32 Pin | Function |
| :--- | :--- | :--- |
| **Pin 3 (TX)** | **GPIO 16 (RX2)** | Sends sensor telemetry to ESP32 |
| **Pin 2 (RX)** | **GPIO 17 (TX2)** | Receives reload & switch commands |

### 3. Shared I²C Bus (LCD & INA219)
Both the LCD (`0x27`) and INA219 (`0x40`) share the Arduino's I²C pins via a breadboard junction:
* **Arduino A4 (SDA)** $\rightarrow$ LCD `SDA` & INA219 `SDA`
* **Arduino A5 (SCL)** $\rightarrow$ LCD `SCL` & INA219 `SCL`

### 4. Load Sensing & Switched-Ground LEDs
* **Relay Module:** `IN` $\rightarrow$ Arduino **Pin 7**
* **INA219 High-Side Load:** Connect `5V Rail` $\rightarrow$ `INA219 V+`. Connect `INA219 V-` to the **Anodes (+)** of all 6 LEDs.
* **LED Cathodes (-):** Wire each LED's cathode through a **$220\ \Omega$ resistor** to its respective Arduino pin (Inverted Logic: `LOW` = ON, `HIGH` = OFF):
  * **Red Warning LED:** Pin 8
  * **House LEDs 1–5:** Pins 9, 10, 11, 12, and 13

---

## 📱 Blynk IoT Cloud Setup

1. Create a new template in [Blynk.Cloud](https://blynk.cloud/) for an **ESP32** over **WiFi**.
2. Configure the following **Virtual Pin Datastreams** (Data Type: `Double` or `Integer` for switches/buttons):

| Virtual Pin | Name | Widget Type | Function |
| :--- | :--- | :--- | :--- |
| **V1 / V2** | Remaining Balance | Gauge / Value | Displays remaining prepaid units |
| **V3** | Reload Input | Numeric Input | Admin input for top-up amount |
| **V4** | System Status | LED | Green (`ACTIVE`) or Red (`OFF`) |
| **V5** | Master Switch | Switch (`0`/`1`) | Remote power override |
| **V6** | Submit Reload | Button (`Push`) | Triggers the `ADD:` serial command |
| **V7** | Remaining Pesos | Value Display | Simulated currency equivalent |
| **V8** | Bus Voltage | Gauge / Value | Real-time voltage ($V$) from INA219 |
| **V9** | Current Draw | Gauge / Value | Real-time current ($A$) from INA219 |
| **V10** | Energy Consumed | Gauge / Value | Accumulated Watt-hours ($Wh$) |
| **V11** | Active Power | Gauge / Value | Real-time wattage ($W$) from INA219 |

3. Create an Event named `zero_balance` under the **Events** tab to enable push notifications when power is cut.

---

## 🚀 Installation & Usage

1. **Install Required Libraries** in the Arduino IDE:
   * `Blynk` by Volodymyr Shymanskyy
   * `Adafruit INA219` by Adafruit
   * `LiquidCrystal I2C` by Frank de Brabander
2. **Upload Arduino Code:** Disconnect wires from Arduino Pins 2 and 3, upload the `Arduino_Meter.ino` sketch, then reconnect the wires.
3. **Configure & Upload ESP32 Code:** Update `BLYNK_TEMPLATE_ID`, `BLYNK_AUTH_TOKEN`, `ssid`, and `pass` in `ESP32_Blynk_Bridge.ino` and upload it to the ESP32.
4. **Operating the Meter:**
   * Power on the system. The LCD will display `EMPTY UNITS / PLEASE RELOAD`.
   * Open the Blynk app, enter a top-up amount into **V3** (confirm/close the keyboard), and tap **Submit (V6)**.
   * The relay will engage, the 5 house LEDs will illuminate, and live sensor telemetry will populate your dashboard.
