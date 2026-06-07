---

# 🔑 Smart Room Access System Documentation

A secure, offline-first IoT access control system powered by an **ESP32** microcontroller. This system features an asynchronous web interface, local hardware button toggles, over-the-air (OTA) updates, an intelligent fail-safe Captive Portal with a background network scanner, and a hardware watchdog timer for high availability.

---

## 📋 Project Overview

The Smart Room Access System replaces standard physical key entries with a secure, digital keypad interface hosted locally on an encrypted web server. It bridges home automation with reliable, physical fallbacks, ensuring you never get locked out—even during power failures, router reboots, or configuration errors.

### Core Features

* **Local Web Keypad Interface:** A light-themed, mobile-responsive numeric keypad utilizing modern asynchronous handling (`fetch` API).
* **Dual Relay Control:** Controls an electric strike or magnetic door lock alongside a main room light toggle.
* **Physical Override Button:** Allows a hardware button on the inside of the room to manually toggle the light via an asynchronous interrupt.
* **Resilient Connectivity (Non-Blocking):** The system boots instantly. It does not hang if your router is offline or rebooting, preserving physical access control locally.
* **Captive Portal & Network Scanner:** If home Wi-Fi is lost for more than 5 minutes, the system switches to AP mode, running an environmental radio snapshot to populate an available network dropdown list.
* **Over-The-Air (OTA) Updates:** Allows flashing updated firmware securely over the local network via the custom port `3232` with back-channeled communication piped over port `10000`.
* **Hardware Watchdog Timer (WDT):** Ensures structural security by forcing a hardware chip reboot within 5 seconds if a memory deadlock or system freeze occurs.

---

## 🛠️ Hardware Architecture

The project relies on minimal components chosen for high reliability and clean execution logic.

![ESP32 WIFI Bluetooth BLE 2-Channel 2-Relay Module 220V AC](<ESP32 WIFI Bluetooth BLE 2-Channel2 Relay Module 220v AC.png>)


### 1. Core Component List & Procurement Links

* **Microcontroller:** [ESP32 Development Board (NodeMCU ESP-WROOM-32)](https://www.aliexpress.com/item/1005007027676026.html?spm=a2g0o.order_list.order_list_main.5.72841802fzU2O9) – The dual-core engine handling Wi-Fi routing, processing, and GPIO control.
* **Relay Isolation:** [2-Channel 5V Relay Module with Optocoupler Isolation](https://www.aliexpress.com/item/1005007027676026.html?spm=a2g0o.order_list.order_list_main.5.72841802fzU2O9) – Isolates high inductive kickbacks from the door strike and light circuits away from the microchip's standard GPIO channels.
* **Tactile Hardware Button:** Onboard button connected to GPIO0 "IO0" – Mounted internally inside the room for manual light toggles.
* **Power Source:** Onboard built-in 220v AC to 5v DC power supply.

### 2. Wiring Pin Map

| Component | ESP32 GPIO Pin | Direction | Function | Active State |
| --- | --- | --- | --- | --- |
| **Door Relay** | Pin defined in `config.h` `GPIO16` | OUTPUT | Triggers Electric Door Strike | Defined by `RELAY_ON` |
| **Light Relay** | Pin defined in `config.h` `GPIO17` | OUTPUT | Switches Overhead Room Light | Defined by `RELAY_ON` |
| **Wall Button** | Pin defined in `config.h` `GPIO0` | INPUT_PULLUP | Physical button inside room | `LOW` (On Press) |

---

## 💻 Software Framework

The software stack utilizes an event-driven paradigm managed over an asynchronous processing loop, keeping user interactions entirely decoupled from background Wi-Fi handling.

### 1. Codebase Breakdown

* **`config.h`**: Stores compile-time constants like structural PIN designations, hardware relays orientations, fallback factory network credentials (`ssid`, `password`), and the secret master unlock sequence (`ACCESS_PIN`).
* **`webpage.h`**: Stores web components compiled inside PROGMEM (Flash memory). Contains `index_html` (the primary keypad) and `wifi_html` (the portal setup interface).
* **`my_smart_room.ino`**: The core execution engine handling FreeRTOS watchdog configurations, network state-machine operations, and route allocations.

### 2. State Machine Logic (How the Code Works)

```
       +---------------------------------------------+
       |                 SYSTEM BOOT                 |
       |  - Mount Pins, Interrupts, & Watchdog Core  |
       |  - Fetch Network Profile from Preference NV |
       +---------------------------------------------+
                              |
                              v
       +---------------------------------------------+
       |               STATION MODE (STA)            |
       |  - Connects & starts Web/OTA server instantly|
       |  - Keypad & Wall Button function offline     |
       +---------------------------------------------+
             |                                 |
     (Wi-Fi Connected)                (Wi-Fi Disconnected)
             |                                 |
             v                                 v
   +-------------------+             +-----------------------+
   | Normal Operations |             | Check Timeout: 5 min  |
   | Bound to home net |             +-----------------------+
   +-------------------+                       |
                                       (Timeout Expired)
                                               |
                                               v
                                     +-----------------------+
                                     |  PRE-FLIGHT SCANNER   |
                                     | Disconnects and snaps |
                                     | local 2.4GHz bands    |
                                     +-----------------------+
                                               |
                                               v
                                     +-----------------------+
                                     |   ACCESS POINT (AP)   |
                                     | - Drops Station Loop  |
                                     | - Broadcasts Network  |
                                     | - Opens /wifi Portal  |
                                     +-----------------------+
                                               |
                                       (5 mins Idle Limit)
                                               |
                                               v
                                     [ Recycle / Restart Loop ]

```

### 3. Non-Volatile Flash Persistence

When updated configurations are saved via the custom captive portal, they are sent to the ESP32's onboard EEPROM/Flash sector using the `<Preferences.h>` namespace interface.

* **Saves:** `ssid` and `pass`.
* **Execution:** On subsequent boot phases, the code pulls values directly from this sector. If the sector is completely clean (e.g., after a full flash erase), it seamlessly reads compilation constants specified in `config.h` as a recovery boundary.

---

## 🛜 Fail-Safe Captive Portal Operation

If your home router performs a scheduled weekly reboot or experiences a sudden black-out, the system prevents accidental setup lockouts through a timed, structural fallback procedure:

1. **The Search Window:** The lock operates normally while attempting to locate your home network for 5 straight minutes.
2. **The Pre-Flight Snapshot:** At minute 5, the ESP32 briefly halts its wireless stack and runs a passive environmental scan, capturing available SSIDs in the area and saving them directly into memory as a JSON array.
3. **The Broadcast:** The system launches its recovery network:
* **SSID:** `Smart Room Access`
* **Password:** *Your default Wi-Fi password written inside `config.h*`


4. **Configuration Page:** When connected, navigate directly to `http://192.168.4.1/wifi` (or click **Network Settings** on the keypad screen).
* Select your target network directly from the **Detected Networks** dropdown menu (populated via the pre-flight snapshot data).
* Type a password, hit save, and the ESP32 automatically commits the text to permanent storage and reboots.



---

## 🔄 Firmware Updates (OTA Deployment)

You can flash updated firmware over your network without removing the lock from the door or plugging in a USB cable.

### 1. Prerequisites (Ubuntu Linux Workspace)

Ensure your environment firewall (`UFW`) is configured to handle back-channeled communication streams routed from the flashing tools:

```bash
sudo ufw allow 10000/tcp

```

### 2. Overriding the Arduino IDE 2.x Variable Bug

Due to an expansion macro bug inside Arduino IDE 2.x, variables parsed across remote target profiles sometimes fail to evaluate properly. To ensure the IDE button works without relying on the command line terminal, patch your core configuration file:

1. Target the local configuration layout file:
```bash
nano /home/qudor/.arduino15/packages/esp32/hardware/esp32/3.0.7/platform.txt

```


2. Locate the deployment line profile: `tools.espota.upload.pattern`
3. Locate the variable definition block: `-p "{upload.port.properties.port}"`
4. Replace that block explicitly with hardcoded target and firewall routing assignments:
```text
-p 3232 -P 10000

```


5. Save the document (`Ctrl+O`, `Enter`, `Ctrl+X`) and restart the Arduino IDE application workspace.

### 3. Triggering the Network Flash

1. Open the Arduino IDE.
2. Click the **Boards / Ports** selection dropdown.
3. Select your network-connected device under **Network Ports** (e.g., `door at 192.168.123.7`).
4. Click the standard **Upload** (Arrow) icon. The system will flash over the network automatically.

---

## 🛡️ Security & Reliability Integrations

### 1. Hardware Watchdog Supervisor

The system integrates an active **FreeRTOS Hardware Watchdog Timer (WDT)** configured to a strict 5-second boundary.

```cpp
esp_task_wdt_reset();

```

This execution instruction is positioned at the start of the `loop()` function. If the core code hangs, deadlocks on a bad web socket transaction, or encounters a memory exception during an open transaction loop, it stops feeding the watchdog. Within 5 seconds, the hardware timer will force a hard reset on the CPU chip, clearing the memory cache and restoring access controls immediately.

### 2. Network Endpoint Isolation

## The network configuration paths (`/scan`, `/wifi`, `/save-wifi`) are explicitly blocked during normal operations. If an external user on your home network tries to access these configuration endpoints, the server returns an absolute `403 Forbidden` response. These endpoints unlock **only** when the ESP32 drops its client mode entirely and switches over to an isolated Access Point configuration.


---

## 💖 Support & Donation

If you find this project useful and would like to support my work, feel free to make a donation via PayPal! Any support is highly appreciated and helps me build more open-source projects.

[![PayPal](https://img.shields.io/badge/Donate-PayPal-blue.svg)](https://paypal.me/KHUDHURALFARHAN)

👉 **[Support me on PayPal](https://paypal.me/KHUDHURALFARHAN)**

---

## ⚠️ Disclaimer

> [!WARNING]
> **HIGH VOLTAGE DANGER:** This project involves working with **220V AC mains electricity**. Mains voltage can cause **severe injury, electrical shock, fire, or death**. Do not attempt to wire, install, or modify this system unless you are fully qualified, know how to handle high-voltage electricity safely, and have completely disconnected all power sources before working.

This project is provided **"as is"** and **without any warranty** of any kind, express or implied. By using this software and hardware design, you agree that you are doing so entirely at your own risk and responsibility. 

The author(s) assume no liability or responsibility for:
* Any damage to property, electrical components, or locks.
* Accidental lockouts or unauthorized physical or digital access.
* Any other mistakes, malfunctions, or safety issues resulting from the use or modification of this open-source project.

**Project Status & Updates:**
Please note that this project is shared for educational/personal use and **may not be actively updated or maintained**. If you encounter any bugs, connectivity issues, or wish to add new features, you are highly encouraged to **fork this repository** and make your own adjustments.

