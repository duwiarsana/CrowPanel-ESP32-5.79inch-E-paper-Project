# CrowPanel ESP32 E-Paper 5.79" HMI Display Project

This project provides a boilerplate example and guide to program the **CrowPanel ESP32 E-Paper 5.79-inch Display** (272x792 resolution, SSD1683 controller, ESP32-S3 WROOM) using the Arduino IDE.

## Directory Structure
* `CrowPanel_EPaper_Demo/`
  * [CrowPanel_EPaper_Demo.ino](file:///Users/duwiarsana/.gemini/antigravity-ide/scratch/CrowPanel-ESP32-5.79-EPaper/CrowPanel_EPaper_Demo/CrowPanel_EPaper_Demo.ino): The main Arduino sketch.
  * [crowpanel_pins.h](file:///Users/duwiarsana/.gemini/antigravity-ide/scratch/CrowPanel-ESP32-5.79-EPaper/CrowPanel_EPaper_Demo/crowpanel_pins.h): Defines the pin mapping of the ESP32-S3 HMI board.

---

## Hardware Pin Mappings

| Component | Function | GPIO Pin |
|---|---|---|
| **E-Paper Power** | Power Control (HIGH = On) | **GPIO 7** (Mandatory!) |
| **E-Paper SPI CS** | Chip Select | GPIO 45 |
| **E-Paper DC** | Data/Command | GPIO 46 |
| **E-Paper RST** | Reset | GPIO 47 |
| **E-Paper BUSY** | Busy Status | GPIO 48 |
| **E-Paper SCK** | SPI Clock | GPIO 12 |
| **E-Paper MOSI** | SPI MOSI | GPIO 11 |
| **Rotary Switch** | UP (CW Rotation) | GPIO 6 |
| **Rotary Switch** | DOWN (CCW Rotation) | GPIO 4 |
| **Rotary Switch** | CLICK (Push button) | GPIO 5 |
| **Button** | Menu | GPIO 2 |
| **Button** | Exit | GPIO 1 |
| **TF (SD) Card CS** | SD Chip Select | GPIO 39 |

---

## Arduino IDE Configuration Guide

To compile and upload this project successfully:

### 1. Board Setup
* Install the **esp32** board library by **Espressif Systems** via the Board Manager (v2.x or v3.x).
* Select the board: **ESP32S3 Dev Module**
* Set the options:
  * **USB CDC On Boot:** `Enabled` (to capture `Serial.print` commands on the USB port).
  * **PSRAM:** `OPI PSRAM` (the module uses ESP32-S3-WROOM-1-N8R8 which has 8MB PSRAM).
  * **Partition Scheme:** `Huge App (3MB No OTA/1MB SPIFFS)` to fit graphics libraries.
  * **Flash Size:** `8MB`

### 2. Libraries Required
Install the following libraries via the Arduino Library Manager:
* **GxEPD2** (by Jean-Marc Zingg)
* **Adafruit GFX Library** (by Adafruit)

---

## Important Note
* **E-Paper Power Pin:** The display panel is powered through a transistor switch on **GPIO 7**. Before attempting to initialize or talk to the display, you must set `pinMode(7, OUTPUT)` and `digitalWrite(7, HIGH)`. If you do not do this, the display controller will not receive power, and the code will hang on initialization because the `BUSY` pin will never go low.
