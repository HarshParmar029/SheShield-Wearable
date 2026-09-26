<p align="center">

# 🛡️ SheShield

### **When she cannot reach her phone, SheShield acts.**

**A multimodal wearable safety prototype that detects distress through ❤️ heart rate, 🎙️ sound, 📳 motion and 💥 impact — then provides a human-cancellable path to 🚨 SOS with 📍 location.**

<br>

<img src="https://img.shields.io/badge/ESP32-IoT-blue?style=for-the-badge&logo=espressif">
<img src="https://img.shields.io/badge/Arduino-Firmware-00979D?style=for-the-badge&logo=arduino">
<img src="https://img.shields.io/badge/IEEE-WIE%20ILS%202026-purple?style=for-the-badge">
<img src="https://img.shields.io/badge/Track-3%20%7C%20SheLeads-orange?style=for-the-badge">
<img src="https://img.shields.io/badge/Problem-P6-red?style=for-the-badge">
<img src="https://img.shields.io/badge/Prototype-Working-success?style=for-the-badge">

<br><br>

### ❤️ 🎙️ 📳 💥 📍 📺 📲 🚨

**Sense → Understand → Warn → Cancel → Respond**

</p>

---

# 🏆 IEEE WIE ILS 2026

### **National Hackathon**

**Track 3 — SheLeads (WIE Special Track)**
**Problem Statement P6 — SheShield Wearable**

---

# 🚨 The Problem

Most safety applications and devices assume one critical thing:

> **The user can act.**

The conventional emergency flow is:

```text
📱 Take Phone
      ↓
🔓 Unlock
      ↓
📲 Open Application
      ↓
🔘 Press SOS
      ↓
🚨 Emergency Alert
```

But during a real emergency, that interaction may not be possible.

The person may be:

* 🤕 Physically restrained
* 📱 Unable to reach the phone
* 😨 Under extreme stress
* 🚫 Unable to perform precise actions
* ⚠️ In a situation where taking out a phone may not be practical

### **SheShield explores the gap between distress beginning and the user being able to manually ask for help.**

---

# 💡 Our Solution

SheShield is an **ESP32-based multimodal wearable prototype** that continuously observes multiple physical signals and converts them into an **explainable distress score**.

Instead of depending on one button:

```text
❤️ Heart Rate
      +
🎙️ Sound
      +
📳 Motion
      +
💥 Impact
      ↓
🧠 DISTRESS ANALYSIS
      ↓
🎯 DISTRESS SCORE
      ↓
⏳ 8-SECOND SAFETY WINDOW
      ↓
      ├── 🛑 CANCEL
      │
      └── 🚨 SOS
             ↓
       📲 Telegram
             +
       📍 GPS Location
```

## 🛡️ Design Philosophy

> **The system should be able to detect a possible emergency without requiring the wearer to manually initiate the first alert.**

---

# ⚡ At a Glance

| Capability                     | SheShield |
| ------------------------------ | :-------: |
| ❤️ Heart-rate monitoring       |     ✅     |
| 🎙️ Sound-event detection      |     ✅     |
| 📳 Motion detection            |     ✅     |
| 💥 Impact detection            |     ✅     |
| 🧠 Explainable distress score  |     ✅     |
| 👤 Personal HR baseline        |     ✅     |
| ⏳ 8-second cancellation window |     ✅     |
| 🔘 Manual SOS                  |     ✅     |
| 📍 GPS location                |     ✅     |
| 📺 OLED interface              |     ✅     |
| 📲 Telegram SOS                |     ✅     |
| 🤖 Telegram command center     |     ✅     |
| 🟢🔴 Local status indicators   |     ✅     |
| 🔊 Emergency buzzer            |     ✅     |
| 📡 Wi-Fi communication         |     ✅     |

---

# 🧠 Multimodal Detection Engine

SheShield does not depend on a single sensor.

It combines multiple signals into a single decision layer.

| Signal              | Hardware         | What it contributes             |
| ------------------- | ---------------- | ------------------------------- |
| ❤️ **Heart Rate**   | MAX30102         | Personal baseline + HR rise     |
| 🎙️ **Sound**       | Sound Sensor     | Separate sound / voice events   |
| 📳 **Motion**       | MPU6050          | Sustained movement              |
| 💥 **Impact**       | MPU6050          | Sudden impact / fall-like event |
| 📍 **Location**     | NEO-6M           | Emergency coordinates           |
| 📺 **Feedback**     | 1.3" SH1106 OLED | Live system information         |
| 🚨 **Local Alert**  | Buzzer + LEDs    | Immediate on-device indication  |
| 📲 **Remote Alert** | Wi-Fi + Telegram | Emergency escalation            |

---

# 🎯 The Core Innovation

## **Multimodal + Explainable + Human-Cancellable**

### 1️⃣ Multimodal

Different physical signals contribute to the same emergency decision.

### 2️⃣ Explainable

The system produces a **0–100 distress score** instead of hiding the decision inside an unexplained black box.

### 3️⃣ Human-Cancellable

Before automatic escalation, the user receives an **8-second cancellation window**.

This creates:

```text
         🧠 DETECTION
              ↓
        🎯 SCORE
              ↓
       ⚠️ POSSIBLE DISTRESS
              ↓
       ⏳ 8 SECOND WINDOW
          ↙         ↘
     🛑 CANCEL      🚨 SOS
```

---

# 🧮 Explainable Distress Score

The current prototype firmware uses the following scoring logic:

| Detected condition                 | Contribution |
| ---------------------------------- | -----------: |
| ❤️ HR rise ≥ +15 BPM               |      **+30** |
| ❤️ HR rise ≥ +25 BPM               |      **+60** |
| 🎙️ 1 sound event                  |      **+20** |
| 🎙️ 2 sound events                 |      **+40** |
| 🎙️ 3 sound events                 |      **+60** |
| 📳 Sustained motion                |      **+15** |
| 💥 Sudden impact / fall-like event |      **+30** |

### 🎯 Automatic escalation threshold

# **55 / 100**

The score makes the system's decision easier to inspect, debug and demonstrate.

> ⚠️ These are **prototype parameters**, not clinically validated thresholds.

---

# 🎙️ Soft-Voice Detection

One of the prototype's key demonstration goals is detecting **quieter voice/sound events** instead of requiring an extremely loud shout.

The current firmware treats separate sound bursts as events.

### Example demonstration

```text
🎙️ "bachaao"
      ↓
🟡 EVENT 1

🎙️ "bachaao"
      ↓
🟡 EVENT 2

🎙️ "bachaao"
      ↓
🔴 EVENT 3

      ↓

🚨 DISTRESS PATTERN
      ↓
⏳ 8-SECOND COUNTDOWN
```

### Important technical clarification

The prototype detects **sound intensity/events**.

It does **not** perform speech recognition and does not identify the actual word `"bachaao"`.

---

# ❤️ Personal Heart-Rate Baseline

Instead of depending only on one universal BPM number, SheShield uses a **personal baseline concept**.

```text
❤️ Initial HR
      ↓
📊 Baseline
      ↓
❤️ Current HR
      ↓
📈 HR Rise
      ↓
🧠 Score Contribution
```

This allows the prototype to reason about **change relative to the wearer** rather than treating every person's heart rate as identical.

---

# 📳 Motion + 💥 Impact Detection

The MPU6050 provides two complementary signals.

### 📳 Sustained Motion

Used for detecting significant movement while reducing sensitivity to small fluctuations.

### 💥 Sudden Impact

Used for fast transient events such as:

* Strong movement
* Sudden impact
* Fall-like acceleration

```text
📳 MOTION
    +
💥 IMPACT
    ↓
🧠 DISTRESS CONTRIBUTION
```

---

# 🛡️ False-Alarm Protection by Design

A safety device must balance **sensitivity** with **controlled escalation**.

SheShield therefore uses multiple layers:

### 🔹 Layer 1 — Multimodal Signals

No single sensor is intended to represent the complete context.

### 🔹 Layer 2 — Explainable Scoring

Signals contribute to a visible score.

### 🔹 Layer 3 — Cancellation Window

A possible emergency enters an **8-second human-in-the-loop window** before automatic escalation.

### 🔹 Layer 4 — Manual Override

The wearer can manually initiate emergency handling through the button.

---

## 🚦 Emergency Decision Flow

```text
          🚨 POSSIBLE DISTRESS
                   │
                   ▼
             🎯 SCORE CHECK
                   │
            Score ≥ 55?
              /       \
            NO         YES
            │           │
            ▼           ▼
       🟢 MONITOR   ⏳ COUNTDOWN
                         │
                  ┌──────┴──────┐
                  │             │
               🛑 CANCEL      NO CANCEL
                  │             │
                  ▼             ▼
             🟢 RETURN       🚨 SOS
                              │
                    ┌─────────┴─────────┐
                    ▼                   ▼
                 📲 Telegram          📍 GPS
```

---

# ⏳ 8-Second Human Safety Window

Automatic escalation is preceded by an **8-second countdown**.

```text
🚨 DISTRESS DETECTED

⏳ 8
⏳ 7
⏳ 6
⏳ 5
⏳ 4
⏳ 3
⏳ 2
⏳ 1

          ↓

       🚨 SOS
```

### 🛑 Short button press

Cancels the pending alert.

### 🚨 No cancellation

SOS becomes active.

---

# 🔘 Physical Control System

| User Action                     | System Behaviour |
| ------------------------------- | ---------------- |
| 🔘 Quick press while OFF        | 🟢 System ON     |
| 🔘 Quick press while monitoring | 🔴 System OFF    |
| 🔘 Quick press during countdown | 🛑 Cancel        |
| 🔘 Quick press during SOS       | 🔄 Reset         |
| 🔘 Hold for 8 seconds           | 🆘 Manual SOS    |
| 📳 Motion gesture while OFF     | 🟢 Arm prototype |

The button therefore acts as both a **control interface** and an **emergency override**.

---

# 🆘 Manual SOS

Automatic detection is not the only path.

The wearer can also initiate a manual emergency sequence.

```text
🔘 HOLD BUTTON
      ↓
⏱️ 8 SECONDS
      ↓
🆘 MANUAL SOS
      ↓
📲 TELEGRAM
      +
📍 GPS
```

This provides a direct fallback when the user is able to manually activate the system.

---

# 📲 Telegram Emergency Command Center

SheShield integrates a Telegram-based remote monitoring layer.

### Available commands

| Command     | Function                      |
| ----------- | ----------------------------- |
| `/start`    | 🛡️ Initialize command center |
| `/help`     | 📖 Show commands              |
| `/status`   | 📊 Live system status         |
| `/location` | 📍 Current GPS location       |
| `/test`     | 🧪 Communication test         |
| `/reset`    | 🔄 Reset emergency state      |
| `/sos`      | 🆘 Manual SOS                 |
| `/on`       | 🟢 Arm system                 |
| `/off`      | 🔴 Disarm system              |

---

# 📍 Emergency Location Intelligence

When GPS has a valid fix, the SOS message can include:

* 📍 Latitude
* 📍 Longitude
* 🛰️ Satellite count
* 📡 HDOP
* 🗺️ Google Maps link
* 📌 Telegram location pin

### Emergency communication flow

```text
🚨 SOS ACTIVE
      ↓
📲 Telegram Alert
      ↓
📍 GPS Coordinates
      ↓
🗺️ Google Maps
      ↓
👤 Remote recipient can identify location
```

---

# 📺 OLED Dashboard

The 1.3" SH1106 OLED provides real-time local feedback.

### Display information includes:

🛡️ System State
❤️ Heart Rate
🎙️ Sound Events
🎯 Distress Score
📳 Motion
💥 Impact
📍 GPS
⏳ Countdown
🚨 SOS State

The display allows judges to see the prototype's internal state during live demonstration.

---

# 🔊 Local Emergency Feedback

SheShield also provides immediate physical feedback through:

🟢 **Green LED** → Monitoring / active state

🔴 **Red LED** → Emergency / alert state

🔊 **Buzzer** → Countdown / emergency indication

📺 **OLED** → Detailed system information

---

# 🔧 Hardware Architecture

```text
                    🛡️ SHESHIELD
                         │
                     ESP32 🧠
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        ▼                ▼                ▼
   ❤️ MAX30102       🎙️ SOUND        📳 MPU6050
   Heart Rate        Sensor          Motion/Impact
        │                │                │
        └────────────────┼────────────────┘
                         │
                         ▼
                  🧠 SCORE ENGINE
                         │
                         ▼
                    ⏳ COUNTDOWN
                         │
                         ▼
                      🚨 SOS
                    /         \
                   /           \
              📲 Telegram     📍 GPS
                              NEO-6M
```

---

# 💰 Hardware & Prototype Cost

| Component                    | Role               |     Approx. Cost |
| ---------------------------- | ------------------ | ---------------: |
| 🧠 ESP32 DevKit              | Main controller    |         ₹300–450 |
| ❤️ MAX30102                  | Heart-rate sensing |         ₹120–200 |
| 📳 MPU6050                   | Motion / impact    |          ₹80–130 |
| 📍 NEO-6M GPS                | Location           |         ₹250–400 |
| 🎙️ Sound Sensor             | Sound events       |          ₹50–100 |
| 📺 1.3" SH1106 OLED          | Live display       |         ₹150–200 |
| 🔊 Buzzer + LEDs             | Local alert        |              ₹50 |
| 🔘 Push Button               | Manual control     |           ₹10–25 |
| 🔌 Breadboard + wires + case | Prototype build    |         ₹150–350 |
| **💰 Total Prototype**       |                    | **≈ ₹900–1,400** |

> Cost represents the current prototype configuration and may vary by supplier.

---

# 🔌 Pin Map

| Component        | ESP32 Connection |
| ---------------- | ---------------- |
| ❤️ MAX30102      | SDA 21 / SCL 22  |
| 📳 MPU6050       | SDA 21 / SCL 22  |
| 📺 SH1106 OLED   | SDA 21 / SCL 22  |
| 📍 GPS TX →      | GPIO 4 / RX2     |
| 📍 GPS RX ←      | GPIO 5 / TX2     |
| 🎙️ Sound Sensor | GPIO 34          |
| 🔊 Buzzer        | GPIO 26          |
| 🔴 Red LED       | GPIO 27          |
| 🟢 Green LED     | GPIO 25          |
| 🔘 Button        | GPIO 13          |

---

# 💻 Technology Stack

```text
🧠 ESP32
⚙️ Arduino Framework
❤️ MAX30105 / MAX30102 Library
📳 MPU6050 — Raw I²C
📍 TinyGPS++
📺 U8g2
📲 UniversalTelegramBot
📡 Wi-Fi
💬 Telegram Bot API
```

---

# 🧪 Prototype Demonstration

## 🎬 Recommended Judge Demo

### 01 — System OFF

🔴 System inactive

↓

### 02 — Arm System

🟢 Monitoring active

↓

### 03 — Show Live Dashboard

❤️ BPM
🎙️ Sound
📳 Motion
📍 GPS
🎯 Score

↓

### 04 — Demonstrate Soft Voice

🎙️ Event 1
🎙️ Event 2
🎙️ Event 3

↓

### 05 — Show Distress Score

🎯 Score crosses trigger threshold

↓

### 06 — Show Countdown

⏳ 8-second cancellation window

↓

### 07 — Demonstrate Cancellation

🔘 Short press

↓

### 08 — Trigger Again

🚨 Countdown

↓

### 09 — Allow SOS

🚨 SOS ACTIVE

↓

### 10 — Show Telegram

📲 Emergency message

↓

### 11 — Show GPS

📍 Coordinates + Map

↓

### 12 — Demonstrate Manual SOS

🔘 8-second hold

---

# 📊 Judge-Facing Demo Matrix

| Scenario                   | Expected System Response       |
| -------------------------- | ------------------------------ |
| 🟢 Normal monitoring       | Continue monitoring            |
| 🎙️ Single sound event     | Add score / continue           |
| 🎙️ Multiple sound events  | Increase distress score        |
| ❤️ HR rise                 | Add physiological contribution |
| 📳 Motion                  | Add movement contribution      |
| 💥 Impact                  | Add impact contribution        |
| 🎯 Threshold reached       | Start countdown                |
| 🔘 Button during countdown | Cancel                         |
| ⏳ Countdown expires        | 🚨 SOS                         |
| 📍 GPS available           | Send location                  |
| 🔘 Manual SOS              | Emergency sequence             |

---

# 📁 Repository Structure

```text
SheShield/
│
├── 📂 firmware/
│   └── 🧠 SheShield.ino
│
├── 📂 hardware/
│   ├── 🔌 wiring/
│   ├── ⚡ circuit/
│   └── 📸 prototype-images/
│
├── 📂 docs/
│   ├── 🧠 architecture/
│   ├── 🧪 testing/
│   └── 📊 presentation/
│
├── 📂 demo/
│   └── 🎥 demo-video-link.md
│
└── 📄 README.md
```

---

# 📈 Prototype Status

## ✅ Working Prototype

```text
❤️ Heart-rate sensing              ✅
👤 Personal HR baseline             ✅
🎙️ Sound-event detection            ✅
📳 Motion detection                 ✅
💥 Impact detection                 ✅
🧠 Explainable distress score       ✅
⏳ 8-second cancellation            ✅
🔘 Manual SOS                       ✅
📍 GPS acquisition                  ✅
📺 OLED dashboard                   ✅
📲 Telegram alerts                  ✅
🤖 Telegram commands                ✅
🟢🔴 LED indication                 ✅
🔊 Buzzer feedback                  ✅
```

---

# 🚀 Product Roadmap

| Phase                                  | Focus               | Goal                             |
| -------------------------------------- | ------------------- | -------------------------------- |
| 🟢 **1 — Prototype**                   | Current system      | Validate multimodal concept      |
| 🔵 **2 — Wearable Engineering**        | PCB + enclosure     | Compact wearable form            |
| 🟣 **3 — Independent Communication**   | GSM/LTE + SMS       | Reduce Wi-Fi dependency          |
| 🟠 **4 — Intelligent Personalization** | Adaptive thresholds | Improve contextual detection     |
| 🔴 **5 — Validation**                  | Controlled testing  | Evaluate reliability & usability |

---

# 🔮 From Prototype → Product

### Current

```text
🧪 Breadboard Prototype
        ↓
ESP32 + Sensors
        ↓
Wi-Fi + Telegram
```

### Future

```text
⌚ Compact Wearable
        ↓
Custom PCB
        ↓
Low-Power Embedded System
        ↓
GSM / LTE
        ↓
Independent SOS Communication
```

---

# ⚠️ Honest Technical Limitations

Strong engineering means being transparent about what the prototype **does and does not** do.

### 🎙️ Sound

The sound sensor detects **sound intensity/events**, not speech semantics.

It does not recognize specific words.

### 📡 Communication

Current Telegram escalation requires Wi-Fi/internet connectivity.

GSM/LTE is part of the future roadmap.

### 📍 GPS

GPS requires satellite visibility and may take time to obtain a fix, especially indoors.

### ❤️ Heart Rate

Reliable HR measurement depends on proper sensor contact and signal quality.

### 🧠 Thresholds

Current thresholds are **prototype parameters** and require broader controlled testing and calibration.

### 🏥 Safety Classification

SheShield is a **student research/hackathon prototype**.

It is not a certified medical device or guaranteed emergency-response system.

---

# 🔐 Security Note

### 🚨 Never commit credentials to GitHub.

Use placeholders such as:

```cpp
#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID   "YOUR_CHAT_ID"

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
```

Use environment/secrets management or a local configuration file for real credentials.

---

# 👥 Team SheShield

## 🛡️ Built by

| Member               | Role                               | Institution                |
| -------------------- | ---------------------------------- | -------------------------- |
| **Harsh Parmar**     | 🧠 Team Lead · Hardware & Firmware | Marwadi University, Rajkot |
| **Mirali Sankaliya** | 📲 SOS Integration · Documentation | Atmiya University          |

---

# 🎥 Prototype Demo

### **Coming here: Full working prototype demonstration**

```text
🎬 DEMO

System ON
    ↓
❤️ HR
    ↓
🎙️ Soft Voice
    ↓
📳 Motion
    ↓
🎯 Distress Score
    ↓
⏳ 8s Countdown
    ↓
🚨 SOS
    ↓
📲 Telegram
    ↓
📍 GPS
```

> **[ INSERT FINAL DEMO VIDEO LINK ]**

---

# 🏆 Why SheShield?

The project is built around a simple but important engineering question:

> ## **What happens when the person who needs help cannot press the button?**

SheShield explores a wearable safety architecture where:

```text
❤️ SENSE
   ↓
🧠 INTERPRET
   ↓
🎯 SCORE
   ↓
⚠️ WARN
   ↓
⏳ ALLOW CANCELLATION
   ↓
🚨 ESCALATE
   ↓
📲 COMMUNICATE
   ↓
📍 LOCATE
```

The goal is not simply to add more sensors.

### The goal is to create a **coordinated emergency-response pipeline**.

---

# 🌟 Vision

### **From button-dependent safety → toward context-aware wearable protection.**

A future SheShield aims to be:

⌚ **Compact**
🔋 **Low-power**
🧠 **Adaptive**
📡 **Independent**
🎯 **Context-aware**
🛡️ **User-centric**

---

<p align="center">

# 🛡️ SheShield

### **Sense. Protect. Respond.**

❤️ 🎙️ 📳 💥 📍 📺 📲 🚨

### **Built with purpose. Engineered for impact.**

**IEEE WIE ILS 2026 — Track 3: SheLeads**

</p>

---

## 📜 License

Released under the **MIT License** for educational, research, and hackathon purposes.
