# 🌬️ ORLARO — Smart Air Purification System

### Complete User Guide & Manual

> **Version:** 1.0.0  
> **Device Model:** Orlaro PM2.5 Monitor  
> **Manufacturer:** Areonex  

---

## 📖 Table of Contents

1. [What is Orlaro?](#-what-is-orlaro)
2. [What's in the Box](#-whats-in-the-box)
3. [Know Your Device — Parts Explained](#-know-your-device--parts-explained)
4. [Hardware Setup — Wiring Guide](#-hardware-setup--wiring-guide)
5. [Software Setup — Installing the Firmware](#-software-setup--installing-the-firmware)
6. [Connecting to WiFi — First Time Setup](#-connecting-to-wifi--first-time-setup)
7. [Understanding the Status LED](#-understanding-the-status-led)
8. [How the Device Works — Daily Operation](#-how-the-device-works--daily-operation)
9. [Reading Air Quality Values](#-reading-air-quality-values)
10. [Fan / Purifier Control](#-fan--purifier-control)
11. [Cloud Dashboard & Remote Control](#-cloud-dashboard--remote-control)
12. [Serial Monitor — Viewing Live Data](#-serial-monitor--viewing-live-data)
13. [Troubleshooting](#-troubleshooting)
14. [Frequently Asked Questions (FAQ)](#-frequently-asked-questions-faq)
15. [Safety & Maintenance](#-safety--maintenance)
16. [Technical Specifications](#-technical-specifications)
17. [Support & Contact](#-support--contact)

---

## 🌍 What is Orlaro?

**Orlaro** is a smart air quality monitoring and automatic air purification system. It continuously measures the tiny invisible particles floating in your air — known as **PM2.5** (particulate matter) — and automatically turns on a fan or air purifier when the air quality drops below safe levels.

### What does it do?

| Feature | Description |
|---------|-------------|
| 📊 **Monitors Air Quality** | Measures PM1.0, PM2.5, and PM10 particles in real-time |
| 🌐 **Sends Data to Cloud** | Uploads readings to a web dashboard every 10 seconds |
| 🔄 **Automatic Fan Control** | Turns your air purifier ON/OFF based on air quality |
| 📱 **Remote Control** | Control the fan from anywhere via the cloud dashboard |
| 💡 **Visual Status** | LED indicator shows system health at a glance |
| 🔒 **Easy WiFi Setup** | No coding needed — configure WiFi from your phone |

### Why does air quality matter?

- **PM2.5** particles are 30 times smaller than a human hair
- They can enter your lungs and bloodstream
- Long-term exposure causes respiratory and heart problems
- Indoor air can be **2-5 times worse** than outdoor air
- Cooking, candles, dust, and smoke all produce PM2.5

> **Orlaro helps you breathe cleaner air by automatically keeping your purifier running when it's needed most.**

---

## 📦 What's in the Box

Before you begin, make sure you have all the required components:

| # | Component | Quantity | What It Looks Like |
|---|-----------|----------|-------------------|
| 1 | ESP32 Dev Module | 1 | Small blue/black circuit board with USB port |
| 2 | PMS5003 Air Quality Sensor | 1 | Small metal box with a fan inside and a ribbon cable |
| 3 | PMS5003 Breakout Cable | 1 | Ribbon cable with colored wires at one end |
| 4 | Relay Module | 1 | Small board with a blue cube (the relay) on it |
| 5 | USB Cable (Micro-USB or Type-C) | 1 | For powering the ESP32 and uploading code |
| 6 | Jumper Wires | Several | Colored wires for connecting components |

### You will also need (not included):

- A computer (Windows, Mac, or Linux)
- A WiFi network (2.4GHz — **not 5GHz**)
- A smartphone or tablet (for WiFi setup)
- A USB power adapter (5V, 1A minimum) for permanent installation
- An air purifier or fan to control (plugged into the relay)

---

## 🔍 Know Your Device — Parts Explained

### ESP32 Dev Module (The Brain)

This is the main controller. Think of it as a tiny computer that:
- Reads the air quality sensor
- Connects to your WiFi
- Sends data to the internet
- Controls the fan relay
- Shows status via the LED

**Key parts:**
- **USB Port** — For power and programming
- **GPIO Pins** — The numbered holes along the edges (for connecting wires)
- **Built-in LED** — The small light on the board (GPIO2)
- **Reset Button** — Small button labeled "EN" or "RST"

### PMS5003 Sensor (The Nose)

This sensor literally "sniffs" the air. It has a tiny laser and fan inside that:
- Sucks in air through a small opening
- Shines a laser through the air
- Counts particles by detecting laser reflections
- Reports PM1.0, PM2.5, and PM10 concentrations

**Important:** Don't block the air intake holes on the sensor!

### Relay Module (The Switch)

The relay is like a remote-controlled light switch. When the ESP32 sends a signal:
- **Signal ON** → Relay clicks → Your fan/purifier turns ON
- **Signal OFF** → Relay clicks → Your fan/purifier turns OFF

**Important:** The relay can handle mains voltage (AC 220V/110V). If you are connecting mains-powered devices, **have a qualified electrician do the high-voltage wiring**.

---

## 🔌 Hardware Setup — Wiring Guide

### Step-by-Step Wiring

Connect the components using jumper wires. Follow the table below **carefully**. Each row is one wire.

### PMS5003 Sensor → ESP32

| Wire # | PMS5003 Pin | Connect To | ESP32 Pin | Wire Color (typical) |
|--------|-------------|-----------|-----------|---------------------|
| 1 | VCC (Power) | → | **5V** (or VIN) | 🔴 Red |
| 2 | GND (Ground) | → | **GND** | ⚫ Black |
| 3 | TXD (Data Out) | → | **GPIO 16** | 🟢 Green |
| 4 | RXD (Data In) | → | **GPIO 17** | 🟡 Yellow |

> **⚠️ Important:** TXD on the sensor goes to GPIO 16 (RX2) on the ESP32. This is a **cross-connection** — TX to RX. This is correct!

### Relay Module → ESP32

| Wire # | Relay Pin | Connect To | ESP32 Pin | Wire Color (typical) |
|--------|-----------|-----------|-----------|---------------------|
| 5 | VCC | → | **5V** (or VIN) | 🔴 Red |
| 6 | GND | → | **GND** | ⚫ Black |
| 7 | IN (Signal) | → | **GPIO 23** | 🔵 Blue |

### Status LED

Most ESP32 boards have a **built-in LED on GPIO 2**. No extra wiring is needed!

If your board doesn't have one, connect an LED:

| Wire # | LED Part | Connect To | Notes |
|--------|----------|-----------|-------|
| 8 | Long leg (+) Anode | → **GPIO 2** | Through a 220Ω resistor |
| 9 | Short leg (-) Cathode | → **GND** | Direct connection |

### Wiring Diagram (Text)

```
                    ┌─────────────────────┐
                    │    ESP32 Dev Board   │
                    │                     │
   PMS5003          │   5V ──────────┐    │
   ┌────────┐       │   GND ─────────┤    │
   │ VCC ───│───────│── 5V           │    │
   │ GND ───│───────│── GND          │    │
   │ TXD ───│───────│── GPIO 16 (RX) │    │
   │ RXD ───│───────│── GPIO 17 (TX) │    │
   └────────┘       │                │    │
                    │                │    │
   Relay Module     │                │    │
   ┌────────┐       │                │    │
   │ VCC ───│───────│── 5V ──────────┘    │
   │ GND ───│───────│── GND              │
   │ IN  ───│───────│── GPIO 23          │
   └────────┘       │                     │
                    │   GPIO 2 = LED 💡   │
                    └─────────────────────┘
```

### ✅ Wiring Checklist

Before powering on, verify:

- [ ] All VCC wires go to 5V
- [ ] All GND wires go to GND
- [ ] PMS5003 TXD → GPIO 16 (not the other way!)
- [ ] PMS5003 RXD → GPIO 17
- [ ] Relay IN → GPIO 23
- [ ] No loose or touching wires
- [ ] USB cable connected to ESP32

---

## 💻 Software Setup — Installing the Firmware

### Step 1: Install Arduino IDE

1. Go to [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download **Arduino IDE 2.x** for your operating system
3. Install and open it

### Step 2: Add ESP32 Board Support

1. Open Arduino IDE
2. Go to **File** → **Preferences**
3. In the **"Additional Board Manager URLs"** field, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Click **OK**
5. Go to **Tools** → **Board** → **Boards Manager**
6. Search for **"esp32"**
7. Install **"esp32 by Espressif Systems"** (latest version)
8. Wait for installation to complete

### Step 3: Install Required Libraries

Go to **Sketch** → **Include Library** → **Manage Libraries** and install:

| Library Name | Author | How to Find It |
|-------------|--------|----------------|
| **WiFiManager** | tzapu | Search "WiFiManager" — install the one by **tablatronix/tzapu** |
| **ArduinoJson** | Benoit Blanchon | Search "ArduinoJson" — install version **7.x** |

### Step 4: Open the Orlaro Code

1. Open Arduino IDE
2. Go to **File** → **Open**
3. Navigate to the `Orlaro` folder
4. Select **`Orlaro.ino`**
5. The code will open in the editor

### Step 5: Select Your Board

1. Go to **Tools** → **Board** → **ESP32 Arduino** → **ESP32 Dev Module**
2. Go to **Tools** → **Port** → Select the COM port where your ESP32 is connected
   - **Windows:** It will show as `COM3`, `COM4`, etc.
   - **Mac:** It will show as `/dev/cu.usbserial-...`
   - **Linux:** It will show as `/dev/ttyUSB0`

> **💡 Tip:** If no port appears, you may need to install a USB driver. Search for "CP2102 driver" or "CH340 driver" depending on your ESP32 board.

### Step 6: Upload the Code

1. Click the **Upload** button (→ arrow icon) in the top-left
2. Wait for it to compile (this takes 1-2 minutes the first time)
3. You'll see "Connecting..." — the code is being uploaded
4. When you see **"Hard resetting via RTS pin..."** — the upload is complete!
5. The device will automatically restart

### Step 7: Open Serial Monitor (Optional)

1. Go to **Tools** → **Serial Monitor**
2. Set baud rate to **115200** (bottom-right dropdown)
3. You'll see live output from the device

---

## 📶 Connecting to WiFi — First Time Setup

When you first power on Orlaro (or if it can't find a saved WiFi network), it will create its own WiFi hotspot so you can configure it.

### Step-by-Step WiFi Setup

#### 1️⃣ Power On the Device

- Plug in the USB cable
- The LED will start **blinking slowly** (once per second)
- This means it's waiting for WiFi configuration

#### 2️⃣ Connect to the Orlaro Hotspot

On your **phone or laptop**:

1. Open your WiFi settings
2. Look for a network called: **`Orlaro_Setup`**
3. Connect to it
4. Enter the password: **`orlaro123`**

> **📱 Note:** Your phone may warn you that this network has "no internet." That's normal! Stay connected.

#### 3️⃣ Configure Your WiFi

After connecting to `Orlaro_Setup`:

1. A **configuration page** should automatically pop up
   - If it doesn't, open a browser and go to: **`http://192.168.4.1`**
2. You'll see the Orlaro WiFi setup portal
3. Tap **"Configure WiFi"**
4. Select your **home WiFi network** from the list
5. Enter your **WiFi password**
6. Tap **"Save"**

#### 4️⃣ Wait for Connection

- The device will restart and connect to your WiFi
- The LED will stop blinking and turn **solid ON** ✅
- Your phone will automatically reconnect to your normal WiFi

#### 5️⃣ Done!

The device is now connected and will remember your WiFi. Even after power outages, it will reconnect automatically.

### If Something Goes Wrong

| Problem | Solution |
|---------|----------|
| Can't find `Orlaro_Setup` network | Wait 30 seconds and refresh your WiFi list |
| Configuration page doesn't open | Open browser and go to `http://192.168.4.1` |
| Wrong WiFi password entered | The device will restart the hotspot — try again |
| Connected but LED still blinks | Your router might be on 5GHz — switch to 2.4GHz |

---

## 💡 Understanding the Status LED

The LED on the device tells you what's happening at a glance. **No need to connect to a computer!**

### LED Status Table

| What You See | What It Means | Is It Normal? | What To Do |
|-------------|---------------|---------------|-----------|
| 🟢 **Solid ON** (always lit) | Everything is working perfectly! WiFi connected, sensor healthy. | ✅ Yes! | Nothing — enjoy clean air! |
| 🟡 **Slow Blink** (on-off every 1 second) | Connecting to WiFi or WiFi setup needed. | ⚡ During startup, yes | If it continues for more than 2 minutes, [set up WiFi](#-connecting-to-wifi--first-time-setup) |
| 🔴 **Fast Blink** (on-off very quickly, 5 times/sec) | Air quality sensor has a problem. | ❌ No | Check sensor wiring. See [Troubleshooting](#-troubleshooting) |
| 🔴🔴 **Double Blink** (two quick flashes, then pause) | Can't communicate with the cloud server. | ⚠️ Temporary | Check your internet connection. Device still works locally! |
| ⚫ **LED Off** | Device is not powered on. | — | Check USB cable and power supply |

### Quick LED Reference Card

```
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  
   ●●●●●●●●●●   = SOLID ON    → All OK ✅
  
   ●●●○○○●●●○○○ = SLOW BLINK  → WiFi Setup 📶
  
   ●○●○●○●○●○   = FAST BLINK  → Sensor Error ⚠️
  
   ●●○●●○○○○○   = DOUBLE      → Server Error 🌐
  
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 🔄 How the Device Works — Daily Operation

Once set up, **Orlaro runs completely automatically**. Here's what happens every second:

### The Automatic Cycle

```
  Every 1 Second:
  ┌─────────────────────────────────────┐
  │  1. Read air quality from sensor    │
  │  2. Validate the reading            │
  │  3. Update internal values          │
  └─────────────────────────────────────┘

  Every 10 Seconds:
  ┌─────────────────────────────────────┐
  │  4. Send data to cloud server       │
  │  5. Receive fan control commands    │
  │  6. Turn fan ON or OFF accordingly  │
  └─────────────────────────────────────┘

  Continuously:
  ┌─────────────────────────────────────┐
  │  7. Monitor WiFi connection         │
  │  8. Update status LED               │
  │  9. Print diagnostics (if USB)      │
  └─────────────────────────────────────┘
```

### What You Need To Do

**Nothing!** Just keep it powered on. The device will:

- ✅ Automatically read air quality
- ✅ Automatically upload data
- ✅ Automatically control the fan
- ✅ Automatically reconnect WiFi if it drops
- ✅ Automatically resume after power outages

---

## 📊 Reading Air Quality Values

### What Are PM1.0, PM2.5, and PM10?

These numbers tell you how many tiny particles are in the air:

| Measurement | Particle Size | What Produces It | Why It Matters |
|------------|---------------|-------------------|---------------|
| **PM1.0** | Ultra-fine (< 1µm) | Smoke, viruses, combustion | Penetrates deep into lungs |
| **PM2.5** | Fine (< 2.5µm) | Cooking, candles, traffic, fires | Most dangerous — enters bloodstream |
| **PM10** | Coarse (< 10µm) | Dust, pollen, mold | Causes irritation and allergies |

> **µg/m³** = micrograms per cubic meter. This is the standard unit for air quality.

### Air Quality Index — What's Safe?

| PM2.5 Value (µg/m³) | Air Quality | Color | Health Advice |
|---------------------|-------------|-------|---------------|
| 0 – 12 | 🟢 **Good** | Green | Air is clean. No action needed. |
| 13 – 35 | 🟡 **Moderate** | Yellow | Acceptable. Sensitive people may notice effects. |
| 36 – 55 | 🟠 **Unhealthy for Sensitive Groups** | Orange | Children, elderly, and asthma patients should limit exposure. |
| 56 – 150 | 🔴 **Unhealthy** | Red | Everyone may experience health effects. Use purifier! |
| 151 – 250 | 🟣 **Very Unhealthy** | Purple | Health alert. Everyone should reduce outdoor activity. |
| 251+ | 🟤 **Hazardous** | Maroon | Emergency conditions. Stay indoors with purifier ON. |

### Where to See the Values

| Method | How |
|--------|-----|
| **Serial Monitor** | Connect USB, open Arduino Serial Monitor at 115200 baud |
| **Cloud Dashboard** | Visit the web dashboard (configured by your administrator) |
| **Diagnostics Output** | Printed every 5 seconds on Serial Monitor |

---

## 🔌 Fan / Purifier Control

### How the Fan is Controlled

The fan (connected through the relay) is controlled by the **cloud server**. Here's how it works:

```
  Cloud Server Decision:
  
  ┌─────────────────────┐      ┌──────────┐      ┌──────────────┐
  │  Server says         │      │  Relay   │      │  Your Fan /  │
  │  fan_on = true   ────│─────▶│  CLICKS  │─────▶│  Purifier    │
  │                      │      │  ON ⚡    │      │  TURNS ON 🌀 │
  └─────────────────────┘      └──────────┘      └──────────────┘
  
  ┌─────────────────────┐      ┌──────────┐      ┌──────────────┐
  │  Server says         │      │  Relay   │      │  Your Fan /  │
  │  fan_on = false  ────│─────▶│  CLICKS  │─────▶│  Purifier    │
  │                      │      │  OFF     │      │  TURNS OFF   │
  └─────────────────────┘      └──────────┘      └──────────────┘
```

### Control Modes

| Mode | How It Works | Who Controls It |
|------|-------------|-----------------|
| **Auto Mode** | Fan turns ON when PM2.5 exceeds the threshold (e.g., 35 µg/m³) | Server decides automatically |
| **Manual Mode** | You manually turn the fan ON or OFF from the dashboard | You control it remotely |

### Connecting Your Purifier to the Relay

> **⚠️ WARNING: If your air purifier runs on mains power (AC 110V/220V), you MUST have a qualified electrician wire the relay. Incorrect wiring can cause electric shock, fire, or death.**

**For low-voltage fans (5V/12V DC):**

1. Connect the fan's positive wire to the relay's **COM** (Common) terminal
2. Connect the relay's **NO** (Normally Open) terminal to your power supply's positive
3. Connect the fan's negative wire directly to the power supply's negative

**For mains-powered purifiers:**
- Consult a licensed electrician
- The relay acts as a switch in the live/hot wire

---

## 🌐 Cloud Dashboard & Remote Control

### What Data is Sent to the Cloud?

Every 10 seconds, Orlaro sends:

```
{
  "device_id":     "orlaro_001",
  "pm25":          current PM2.5 reading,
  "pm10":          current PM10 reading,
  "pm1":           current PM1.0 reading,
  "sensor_status": "online" or "offline",
  "firmware":      "1.0.0"
}
```

### What the Server Sends Back

The server responds with control instructions:

```
{
  "status":        "success",
  "fan_on":        true or false,
  "auto_mode":     true or false,
  "threshold":     35,
  "manual_fan_on": true or false,
  "poll_interval": 10
}
```

### Privacy & Data

- ✅ Only air quality data and device ID are sent
- ✅ No personal information is collected
- ✅ No camera, microphone, or location data
- ✅ Data is sent over encrypted HTTPS

---

## 🖥️ Serial Monitor — Viewing Live Data

For advanced users and debugging, you can connect the device to a computer and view real-time data.

### How to Open the Serial Monitor

1. Connect the ESP32 to your computer via USB
2. Open **Arduino IDE**
3. Go to **Tools** → **Serial Monitor**
4. Set baud rate to **`115200`** (bottom-right corner)

### What You'll See

#### Startup Messages
```
============================================
  ORLARO Air Purification System
  Firmware: 1.0.0
  Device:   orlaro_001
============================================

[WIFI] Initializing WiFi Manager...
[WIFI] Connected successfully!
[WIFI] IP Address: 192.168.1.105
[WIFI] RSSI: -45 dBm
[PMS] Sensor initialized.
[INIT] All subsystems initialized.
```

#### Diagnostics Output (Every 5 Seconds)
```
┌──────────────────────────────────────────┐
│         ORLARO SYSTEM DIAGNOSTICS        │
├──────────────────────────────────────────┤
│ WiFi Status  : CONNECTED                │
│ WiFi RSSI    : -42 dBm                  │
│ IP Address   : 192.168.1.105            │
├──────────────────────────────────────────┤
│ Sensor       : ONLINE                   │
│ Packet Valid : YES                      │
│ PM1.0        : 8 µg/m³                  │
│ PM2.5        : 15 µg/m³                 │
│ PM10         : 22 µg/m³                 │
├──────────────────────────────────────────┤
│ Fan State    : OFF (Relay LOW)          │
│ Server Ctrl  : ACTIVE                   │
├──────────────────────────────────────────┤
│ Free Heap    : 198432 bytes             │
│ Uptime       : 01:23:45                 │
│ Server Error : NO                       │
└──────────────────────────────────────────┘
```

#### Fan State Change Alert
```
╔══════════════════════════════════════╗
║  FAN STATE CHANGED -> ON             ║
╚══════════════════════════════════════╝
```

#### Cloud Upload Log
```
[CLOUD] ---- Telemetry Upload ----
[CLOUD] JSON Sent: {"device_id":"orlaro_001","pm25":32,"pm10":45,"pm1":12,"sensor_status":"online","firmware":"1.0.0"}
[CLOUD] HTTP Response Code: 200
[CLOUD] JSON Received: {"status":"success","fan_on":false,"auto_mode":true,"threshold":35,"manual_fan_on":false,"poll_interval":10}
[CLOUD] ---- Upload Complete ----
```

### Understanding RSSI (WiFi Signal Strength)

| RSSI Value | Signal Quality | Recommendation |
|-----------|---------------|----------------|
| -30 to -50 | 🟢 Excellent | Perfect — no action needed |
| -51 to -60 | 🟢 Good | Works well |
| -61 to -70 | 🟡 Fair | May experience occasional drops |
| -71 to -80 | 🟠 Weak | Consider moving closer to router |
| -81 or worse | 🔴 Very Weak | Unstable — move device or add WiFi extender |

---

## 🔧 Troubleshooting

### Problem: LED Won't Stop Blinking Slowly

**Meaning:** The device can't connect to WiFi.

**Solutions:**
1. Make sure your router is turned on
2. Check that you're using a **2.4GHz** WiFi network (ESP32 does NOT support 5GHz)
3. Re-do the WiFi setup:
   - Look for the `Orlaro_Setup` WiFi network on your phone
   - Connect and reconfigure
4. Make sure the WiFi password is correct
5. Try restarting the device (unplug and replug USB)

---

### Problem: LED is Blinking Very Fast

**Meaning:** The air quality sensor is not working.

**Solutions:**
1. Check all 4 sensor wires are properly connected
2. Make sure **TXD goes to GPIO 16** and **RXD goes to GPIO 17**
3. Make sure the sensor is getting **5V power** (not 3.3V)
4. Gently blow air into the sensor intake to test it
5. The sensor needs **30 seconds to warm up** after powering on
6. If wires are correct, the sensor may be damaged — try replacing it

---

### Problem: LED Does a Double-Blink Pattern

**Meaning:** Can't reach the cloud server.

**Solutions:**
1. Check your internet connection (can your phone browse the web?)
2. The server might be temporarily down — wait a few minutes
3. Check if your router's firewall is blocking outgoing HTTPS connections
4. **The device will continue monitoring and controlling the fan with its last known settings** — no data is lost

---

### Problem: Fan Won't Turn On

**Solutions:**
1. Check the relay wiring (IN → GPIO 23, VCC → 5V, GND → GND)
2. Listen for a "click" from the relay — if you hear it, the relay is working but check your fan wiring
3. Make sure the fan/purifier is plugged in and has power
4. Check the cloud dashboard — is `fan_on` set to `true`?
5. Test the relay manually: in the cloud dashboard, set manual mode and turn the fan on

---

### Problem: All Values Show 0

**Solutions:**
1. The sensor needs **30 seconds to warm up** after power-on — wait and check again
2. Check the sensor wiring
3. Make sure air can flow through the sensor (don't block the intake holes)
4. The sensor's internal fan should be spinning — listen for a faint hum

---

### Problem: Can't Upload the Code

**Solutions:**
1. Make sure you selected **"ESP32 Dev Module"** as the board
2. Select the correct **COM port** under Tools → Port
3. Install the USB driver for your board:
   - **CP2102** chip → Download driver from Silicon Labs
   - **CH340** chip → Download driver from WCH
4. Try a different USB cable (some cables are charge-only and don't carry data)
5. Try holding the **BOOT** button on the ESP32 while uploading

---

### Problem: WiFi Keeps Disconnecting

**Solutions:**
1. Move the device closer to your WiFi router
2. Check the RSSI value in the Serial Monitor (should be better than -70)
3. Make sure the router isn't overloaded with too many devices
4. Try assigning a static IP address to the ESP32 in your router settings
5. Add a WiFi extender near the device

---

### Problem: Device Restarts Randomly

**Solutions:**
1. Use a **quality USB cable** — cheap cables cause voltage drops
2. Use a **5V 2A power adapter** — insufficient power causes brownout resets
3. Don't power the relay and sensor from the ESP32's 3.3V pin — use the **5V/VIN** pin
4. If using a long USB cable, try a shorter one

---

### Emergency Reset — Erasing All Settings

If the device gets into a bad state and you want to start completely fresh:

1. Install **esptool** on your computer:
   ```
   pip install esptool
   ```
2. Find your COM port (check Device Manager on Windows)
3. Run:
   ```
   esptool.py --port COM3 erase_flash
   ```
4. Re-upload the Orlaro firmware
5. The device will start in WiFi setup mode as if brand new

---

## ❓ Frequently Asked Questions (FAQ)

### General

**Q: Does Orlaro work without internet?**  
A: The sensor reads air quality regardless of internet. However, cloud uploading and remote fan control require internet. The fan will maintain its last known state if internet drops.

**Q: Can I use Orlaro outdoors?**  
A: The electronics are not waterproof. For outdoor use, you must place them in a weatherproof enclosure with ventilation for the sensor.

**Q: How accurate is the PMS5003 sensor?**  
A: The PMS5003 has a measurement range of 0-500 µg/m³ with ±10% accuracy for PM2.5. It's accurate enough for home and office use, though not laboratory-grade.

**Q: Does the device store data locally?**  
A: Currently, no. Data is sent directly to the cloud. If the cloud is unavailable, readings are not stored. See [Future Improvements](#future-improvements) for plans to add local buffering.

### WiFi

**Q: Does Orlaro work with 5GHz WiFi?**  
A: **No.** The ESP32 only supports **2.4GHz** WiFi. Most routers broadcast both — make sure you connect to the 2.4GHz network.

**Q: Can I change the WiFi network later?**  
A: Yes! If the device can't connect to the saved network, it will automatically start the `Orlaro_Setup` hotspot. You can also trigger this by erasing the flash (see Emergency Reset).

**Q: What's the WiFi range?**  
A: Same as any WiFi device — typically 10-30 meters indoors depending on walls and interference.

### Sensor

**Q: How often should I clean the sensor?**  
A: Clean the sensor intake with compressed air every 6-12 months, or more often in dusty environments.

**Q: How long does the PMS5003 sensor last?**  
A: The PMS5003 is rated for approximately **8,000 hours** of continuous operation (about 1 year). For longer life, the sensor can be put to sleep periodically (future firmware feature).

**Q: Why do readings fluctuate?**  
A: This is normal. Air quality changes constantly. Readings may spike when cooking, cleaning, or opening windows. The sensor reads every second to provide real-time data.

### Power

**Q: How much power does Orlaro use?**  
A: The full system draws approximately 300-500mA at 5V (1.5-2.5 watts). A standard phone charger (5V, 1A) is sufficient.

**Q: Can I run it on a battery?**  
A: Technically yes, but the continuous WiFi and sensor operation will drain batteries quickly. A 10,000mAh power bank would last approximately 10-15 hours.

---

## 🛡️ Safety & Maintenance

### Safety Warnings

> **⚠️ ELECTRICAL SAFETY**
> - Never touch the relay terminals while the device is powered on
> - Have a qualified electrician connect mains-powered devices to the relay
> - Do not expose the device to water or moisture
> - Do not operate the device near flammable gases

> **⚠️ SENSOR CARE**
> - Do not insert objects into the sensor air intake
> - Do not use the sensor in environments with oil mist or corrosive gases
> - Keep the sensor away from direct sunlight and extreme heat

### Maintenance Schedule

| Task | Frequency | How To Do It |
|------|-----------|-------------|
| Visual inspection of wiring | Monthly | Check for loose or corroded connections |
| Clean sensor air intake | Every 6 months | Use compressed air (short bursts, don't shake the sensor) |
| Check USB cable condition | Every 6 months | Look for frayed or bent connectors |
| Verify readings against known source | Yearly | Compare with a reference monitor or outdoor AQI station |
| Replace PMS5003 sensor | Every 1-2 years | Order a new PMS5003, swap wiring |

### Optimal Placement

**DO place the device:**
- ✅ In the room you spend the most time in
- ✅ At breathing height (table or shelf level)
- ✅ Away from direct airflow (fans, AC vents, windows)
- ✅ In a central location for representative readings

**DON'T place the device:**
- ❌ Directly next to the air purifier (readings will be artificially low)
- ❌ In enclosed spaces (cabinets, drawers)
- ❌ Near cooking surfaces (sensor will quickly degrade)
- ❌ In direct sunlight or near heat sources
- ❌ In bathrooms or high-humidity areas

---

## 📋 Technical Specifications

| Specification | Value |
|--------------|-------|
| **Microcontroller** | ESP32-WROOM-32 |
| **CPU** | Dual-core Xtensa LX6, 240 MHz |
| **RAM** | 520 KB SRAM |
| **Flash** | 4 MB |
| **WiFi** | 802.11 b/g/n, 2.4 GHz |
| **Sensor** | Plantower PMS5003 |
| **Measurement Range** | 0–500 µg/m³ |
| **Measurement Resolution** | 1 µg/m³ |
| **Sensor Accuracy** | ±10% (PM2.5) |
| **Sensor Response Time** | < 10 seconds |
| **Communication** | UART (9600 baud) + HTTPS |
| **Relay Rating** | 10A @ 250VAC / 10A @ 30VDC |
| **Power Input** | 5V DC via USB |
| **Power Consumption** | ~2W typical |
| **Operating Temperature** | -10°C to 60°C |
| **Firmware Version** | 1.0.0 |
| **Cloud Protocol** | HTTPS POST (JSON) |
| **Upload Interval** | 10 seconds |
| **Sensor Read Interval** | 1 second |

---

## 📞 Support & Contact

If you need help with your Orlaro device:

| Channel | Details |
|---------|---------|
| **Email** | support@areonex.com |
| **GitHub** | Report issues on the project repository |
| **Documentation** | This README file |
| **Serial Monitor** | Connect via USB for live debugging |

### Before Contacting Support

Please have the following information ready:

1. **LED Status** — What pattern is the LED showing?
2. **Serial Output** — If possible, copy the diagnostics output
3. **WiFi Network** — Are you using 2.4GHz?
4. **Power Source** — What USB adapter/cable are you using?
5. **Wiring Photo** — A clear photo of your wiring

---

## 🔮 Future Improvements

Planned features for upcoming firmware updates:

- **📲 OTA Updates** — Update firmware wirelessly without USB
- **💾 Local Data Storage** — Buffer readings when offline
- **🌡️ Temperature & Humidity** — Support for BME280/DHT22 sensors
- **📊 Local Web Dashboard** — View data directly from the device's IP address
- **🔋 Battery Mode** — Deep sleep for battery-powered operation
- **📡 MQTT Support** — Real-time communication protocol
- **🔵 Bluetooth Setup** — Alternative WiFi configuration via BLE

---

<div align="center">

**Made with ❤️ by Areonex**

*Breathe clean. Live healthy.*

</div>
