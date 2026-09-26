<div align="center">

# 🛡️ SheShield

### **When she cannot reach her phone, SheShield acts.**

<p>
<b>Multimodal • Explainable • Human-Cancellable • Location-Aware</b>
</p>

<p>
❤️ Heart Rate &nbsp; • &nbsp;
🎙️ Sound &nbsp; • &nbsp;
📳 Motion &nbsp; • &nbsp;
💥 Impact &nbsp; • &nbsp;
📍 GPS &nbsp; • &nbsp;
🚨 SOS
</p>

<br>

<img src="https://img.shields.io/badge/ESP32-IoT-0078D4?style=for-the-badge&logo=espressif&logoColor=white">
<img src="https://img.shields.io/badge/Arduino-Firmware-00979D?style=for-the-badge&logo=arduino&logoColor=white">
<img src="https://img.shields.io/badge/IEEE-WIE%20ILS-2026-6A1B9A?style=for-the-badge">
<img src="https://img.shields.io/badge/Track-SheLeads-E67E22?style=for-the-badge">
<img src="https://img.shields.io/badge/Problem-P6-C0392B?style=for-the-badge">
<img src="https://img.shields.io/badge/Prototype-WORKING-2ECC71?style=for-the-badge">

<br><br>

<a href="#-live-prototype">
<img src="https://img.shields.io/badge/🚀_LIVE_PROTOTYPE-2ECC71?style=for-the-badge">
</a>
&nbsp;
<a href="#-how-it-works">
<img src="https://img.shields.io/badge/🧠_HOW_IT_WORKS-3498DB?style=for-the-badge">
</a>
&nbsp;
<a href="#-demo">
<img src="https://img.shields.io/badge/🎥_DEMO-E74C3C?style=for-the-badge">
</a>

</div>

---

<div align="center">

## 🏆 IEEE WIE ILS 2026

### **Track 3 — SheLeads**

### **Problem Statement P6 — SheShield Wearable**

</div>

---

# 📸 Meet SheShield

<div align="center">

<!-- REPLACE WITH YOUR BEST PROTOTYPE PHOTO -->

<img src="./assets/images/sheshield-prototype.jpg" width="850">

### 🛡️ **A working multimodal safety prototype**

</div>

> **The idea is simple:**
> If the wearer cannot reach the phone, the wearable should still be able to recognize a possible distress pattern and begin an emergency-response process.

---

# 🚨 The Problem

Traditional emergency systems often assume:

```text
📱 Reach Phone
      ↓
🔓 Unlock
      ↓
📲 Open App
      ↓
🔘 Press SOS
      ↓
🚨 Help
```

But during a real emergency, **the first step itself may be impossible.**

The user may be:

* 🤕 Physically restrained
* 📱 Unable to reach the phone
* 😨 Under extreme stress
* 🚫 Unable to perform precise interactions
* ⚠️ In a situation where using a phone is not practical

### 💡 SheShield targets this interaction gap.

---

# 🛡️ Our Core Idea

<div align="center">

### **Don't wait for the button.**

### **Observe the situation.**

<br>

❤️ + 🎙️ + 📳 + 💥 <br>
⬇️ <br>
🧠 **DISTRESS ANALYSIS** <br>
⬇️ <br>
🎯 **EXPLAINABLE SCORE** <br>
⬇️ <br>
⏳ **8-SECOND SAFETY WINDOW** <br>
⬇️ <br>
🚨 **SOS** <br>
⬇️ <br>
📲 **TELEGRAM** + 📍 **GPS**

</div>

---

# 🧠 How It Works

<div align="center">

<img src="./assets/diagrams/system-architecture.png" width="900">

</div>

### 🔄 Detection Pipeline

```text
       ❤️ HEART RATE
              │
              │
       🎙️ SOUND EVENTS
              │
              │
       📳 MOTION / IMPACT
              │
              ▼
    ┌─────────────────────┐
    │   🧠 SENSOR FUSION  │
    └──────────┬──────────┘
               │
               ▼
    ┌─────────────────────┐
    │ 🎯 DISTRESS SCORE   │
    │       0 — 100       │
    └──────────┬──────────┘
               │
          SCORE ≥ 55
               │
               ▼
    ┌─────────────────────┐
    │   ⏳ 8 SEC WINDOW    │
    │                     │
    │  🛑 CANCEL   🚨 SOS │
    └──────────┬──────────┘
               │
          NOT CANCELLED
               │
               ▼
       ┌───────────────┐
       │  🚨 SOS ACTIVE│
       └───────┬───────┘
               │
       ┌───────┴────────┐
       ▼                ▼
   📲 TELEGRAM       📍 GPS
```

---

# ⭐ What Makes SheShield Different?

<div align="center">

|         🧩 Feature        | 🛡️ SheShield |
| :-----------------------: | :-----------: |
|   ❤️ Heart-rate context   |       ✅       |
| 🎙️ Sound-event detection |       ✅       |
|     📳 Motion analysis    |       ✅       |
|    💥 Impact detection    |       ✅       |
|    🧠 Explainable score   |       ✅       |
|  👤 Personal HR baseline  |       ✅       |
|    ⏳ Human cancellation   |       ✅       |
|       🔘 Manual SOS       |       ✅       |
|      📍 GPS location      |       ✅       |
|  📲 Remote Telegram alert |       ✅       |
|      📺 OLED feedback     |       ✅       |
|  🔊 Local emergency alert |       ✅       |

</div>

---

# 🧮 Explainable Distress Engine

Instead of an unexplained **AI = YES/NO**, the prototype exposes a visible score.

### 🎯 Current Prototype Parameters

| Signal               | Contribution |
| :------------------- | -----------: |
| ❤️ HR rise ≥ +15 BPM |      **+30** |
| ❤️ HR rise ≥ +25 BPM |      **+60** |
| 🎙️ 1 sound event    |      **+20** |
| 🎙️ 2 sound events   |      **+40** |
| 🎙️ 3 sound events   |      **+60** |
| 📳 Sustained motion  |      **+15** |
| 💥 Sudden impact     |      **+30** |

### 🚨 Trigger Threshold

<div align="center">

# **55 / 100**

`███████████░░░░░░░░░`

### 🎯 DISTRESS THRESHOLD

</div>

> These values are prototype parameters and require further calibration and validation.

---

# 🛡️ False-Alarm Protection by Design

SheShield is designed with multiple safety layers.

### 1️⃣ Multimodal Context

Multiple signals contribute to the decision.

### 2️⃣ Explainable Scoring

Every contribution can be inspected.

### 3️⃣ Human Cancellation

The system provides an **8-second cancellation window**.

### 4️⃣ Manual Override

The wearer can manually initiate SOS.

---

## ⏳ Human-in-the-Loop Safety Layer

<div align="center">

```text
             🚨 POSSIBLE DISTRESS
                      │
                      ▼
                 🎯 SCORE ≥ 55
                      │
                      ▼
               ⏳ 8 SECOND WINDOW
                      │
             ┌────────┴────────┐
             │                 │
        🔘 CANCEL          NO ACTION
             │                 │
             ▼                 ▼
       🟢 CANCELLED        🚨 SOS
                              │
                      ┌───────┴───────┐
                      ▼               ▼
                  📲 TELEGRAM       📍 GPS
```

</div>

---

# 🎙️ Soft-Voice Demonstration

### **Designed for quieter voice/sound events**

<div align="center">

```text
🎙️ EVENT 1
      ↓
🎙️ EVENT 2
      ↓
🎙️ EVENT 3
      ↓
🎯 DISTRESS PATTERN
      ↓
⏳ COUNTDOWN
      ↓
🚨 SOS
```

</div>

### ⚠️ Technical Honesty

The current prototype detects **sound intensity/events**.

It does **not** perform speech recognition or identify the word `"bachaao"`.

---

# ❤️ Personal Heart-Rate Baseline

<div align="center">

```text
       ❤️ INITIAL READINGS
              ↓
       📊 PERSONAL BASELINE
              ↓
       ❤️ CURRENT BPM
              ↓
          📈 HR RISE
              ↓
       🎯 SCORE CONTRIBUTION
```

</div>

The system therefore considers the **change relative to the baseline**, rather than relying only on one universal BPM value.

---

# 📳 Motion + 💥 Impact

The MPU6050 provides complementary movement information.

```text
📳 SUSTAINED MOTION
          +
💥 SUDDEN IMPACT
          ↓
🧠 DISTRESS CONTRIBUTION
```

This enables the prototype to demonstrate different physical distress scenarios.

---

# 🚨 SOS State Machine

<div align="center">

```text
       🔴 SYSTEM OFF
              │
              │ 🔘 / Motion
              ▼
      🟢 MONITORING
              │
              │ 🎯 Threshold
              ▼
       🟡 COUNTDOWN
              │
       ┌──────┴──────┐
       │             │
   🔘 CANCEL      ⏳ TIMEOUT
       │             │
       ▼             ▼
    🟢 RESET      🚨 SOS ACTIVE
                     │
             ┌───────┴───────┐
             ▼               ▼
         📲 TELEGRAM       📍 GPS
```

</div>

---

# 🔘 Physical Controls

| User Action                 | Result        |
| --------------------------- | ------------- |
| 🔘 Quick press — OFF        | 🟢 Turn ON    |
| 🔘 Quick press — Monitoring | 🔴 Turn OFF   |
| 🔘 Quick press — Countdown  | 🛑 Cancel     |
| 🔘 Quick press — SOS        | 🔄 Reset      |
| 🔘 Hold 8 seconds           | 🆘 Manual SOS |
| 📳 Motion gesture — OFF     | 🟢 Arm        |

---

# 📲 Telegram Command Center

<div align="center">

### **Your emergency control room**

</div>

|   Command   | Function              |
| :---------: | --------------------- |
|   `/start`  | 🛡️ Initialize        |
|   `/help`   | 📖 Commands           |
|  `/status`  | 📊 Live status        |
| `/location` | 📍 GPS location       |
|   `/test`   | 🧪 Communication test |
|   `/reset`  | 🔄 Reset              |
|    `/sos`   | 🆘 Manual SOS         |
|    `/on`    | 🟢 Arm                |
|    `/off`   | 🔴 Disarm             |

---

# 📍 Emergency Location

When a valid GPS fix is available:

```text
🚨 SOS ACTIVE

❤️ Heart Rate
🎙️ Sound Events
📳 Motion
🎯 Distress Score

📍 Latitude
📍 Longitude
🛰️ Satellites
📡 HDOP

🗺️ Google Maps
📌 Telegram Location
```

---

# 📺 Live OLED Dashboard

<div align="center">

<!-- ADD OLED PHOTO -->

<img src="./assets/images/oled-dashboard.jpg" width="600">

### **Live system visibility**

</div>

The OLED provides local feedback for:

❤️ BPM
🎙️ Sound Events
🎯 Distress Score
📳 Motion
📍 GPS
⏳ Countdown
🚨 SOS State

---

# 📸 Hardware Prototype

<div align="center">

<!-- ADD 2–4 REAL PHOTOS -->

<img src="./assets/images/prototype-front.jpg" width="400">
<img src="./assets/images/prototype-side.jpg" width="400">

<br><br>

<img src="./assets/images/prototype-wiring.jpg" width="800">

### 🔧 Working Breadboard Prototype

</div>

---

# 🔌 Hardware Architecture

<div align="center">

<img src="./assets/diagrams/hardware-block-diagram.png" width="900">

</div>

---

# 💰 Hardware & Cost

| Component                    | Role            |     Approx. Cost |
| ---------------------------- | --------------- | ---------------: |
| 🧠 ESP32 DevKit              | Main controller |         ₹300–450 |
| ❤️ MAX30102                  | Heart rate      |         ₹120–200 |
| 📳 MPU6050                   | Motion / impact |          ₹80–130 |
| 📍 NEO-6M                    | GPS             |         ₹250–400 |
| 🎙️ Sound Sensor             | Sound events    |          ₹50–100 |
| 📺 SH1106 1.3"               | OLED            |         ₹150–200 |
| 🔊 Buzzer + LEDs             | Alerts          |              ₹50 |
| 🔘 Push Button               | Control         |           ₹10–25 |
| 🔌 Breadboard + wires + case | Prototype       |         ₹150–350 |
| **💰 TOTAL**                 |                 | **≈ ₹900–1,400** |

---

# 🔌 Pin Map

| Hardware     | ESP32           |
| ------------ | --------------- |
| ❤️ MAX30102  | SDA 21 / SCL 22 |
| 📳 MPU6050   | SDA 21 / SCL 22 |
| 📺 OLED      | SDA 21 / SCL 22 |
| 📍 GPS TX    | GPIO 4          |
| 📍 GPS RX    | GPIO 5          |
| 🎙️ Sound    | GPIO 34         |
| 🔊 Buzzer    | GPIO 26         |
| 🔴 Red LED   | GPIO 27         |
| 🟢 Green LED | GPIO 25         |
| 🔘 Button    | GPIO 13         |

---

# 🧰 Technology Stack

<div align="center">

<img src="https://img.shields.io/badge/ESP32-000000?style=flat-square&logo=espressif">
<img src="https://img.shields.io/badge/Arduino-00979D?style=flat-square&logo=arduino">
<img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus">
<img src="https://img.shields.io/badge/WiFi-0078D4?style=flat-square&logo=wifi">
<img src="https://img.shields.io/badge/GPS-NEO--6M-green?style=flat-square">
<img src="https://img.shields.io/badge/Telegram-Bot_API-26A5E4?style=flat-square&logo=telegram">

</div>

---

# 🎥 LIVE PROTOTYPE

<div align="center">

## 🚀 See SheShield in Action

<!-- OPTION 1: GIF -->

<img src="./assets/demo/sheshield-demo.gif" width="850">

<br><br>

### ▶️ Full Demonstration

<a href="YOUR_YOUTUBE_LINK">
<img src="https://img.shields.io/badge/▶️_WATCH_FULL_DEMO-FF0000?style=for-the-badge&logo=youtube&logoColor=white">
</a>

</div>

### 🎬 Recommended Demo Flow

```text
01  🔴 System OFF
02  🟢 System ON
03  📺 Live OLED
04  ❤️ Heart Rate
05  🎙️ Soft Voice
06  🎯 Distress Score
07  ⏳ Countdown
08  🛑 Cancellation
09  🚨 SOS
10  📲 Telegram
11  📍 GPS
12  🆘 Manual SOS
```

---

# 🧪 Judge Demonstration Matrix

| Scenario             | Expected Result            |
| -------------------- | -------------------------- |
| 🟢 Normal monitoring | Continue monitoring        |
| 🎙️ Sound event      | Score contribution         |
| 🎙️ Repeated events  | Increased score            |
| ❤️ HR rise           | Physiological contribution |
| 📳 Motion            | Motion contribution        |
| 💥 Impact            | Impact contribution        |
| 🎯 Score ≥ threshold | Countdown                  |
| 🔘 Cancel            | Alert cancelled            |
| ⏳ No cancellation    | 🚨 SOS                     |
| 📍 GPS available     | Location sent              |
| 🆘 Manual SOS        | Emergency sequence         |

---

# 📁 Repository

```text
SheShield/
│
├── 📂 assets/
│   ├── 📂 images/
│   │   ├── prototype-front.jpg
│   │   ├── prototype-side.jpg
│   │   ├── prototype-wiring.jpg
│   │   └── oled-dashboard.jpg
│   │
│   ├── 📂 diagrams/
│   │   ├── system-architecture.png
│   │   └── hardware-block-diagram.png
│   │
│   └── 📂 demo/
│       └── sheshield-demo.gif
│
├── 📂 firmware/
│   └── SheShield.ino
│
├── 📂 hardware/
│   ├── wiring/
│   └── circuit/
│
├── 📂 docs/
│   ├── architecture/
│   ├── testing/
│   └── presentation/
│
├── 📂 demo/
│   └── demo-video-link.md
│
└── 📄 README.md
```

---

# 📈 Prototype Status

<div align="center">

| Module              |   Status   |
| ------------------- | :--------: |
| ❤️ Heart Rate       | 🟢 WORKING |
| 👤 HR Baseline      | 🟢 WORKING |
| 🎙️ Sound Detection | 🟢 WORKING |
| 📳 Motion           | 🟢 WORKING |
| 💥 Impact           | 🟢 WORKING |
| 🧠 Distress Score   | 🟢 WORKING |
| ⏳ 8s Countdown      | 🟢 WORKING |
| 🛑 Cancellation     | 🟢 WORKING |
| 🆘 Manual SOS       | 🟢 WORKING |
| 📍 GPS              | 🟢 WORKING |
| 📺 OLED             | 🟢 WORKING |
| 📲 Telegram         | 🟢 WORKING |

</div>

---

# 🚀 Roadmap

```text
                 🧪 TODAY
                    │
                    ▼
          🛡️ WORKING PROTOTYPE
                    │
                    ▼
             🔵 PHASE 2
          Custom PCB + Case
                    │
                    ▼
             🟣 PHASE 3
           GSM / LTE + SMS
                    │
                    ▼
             🟠 PHASE 4
        Adaptive Personalization
                    │
                    ▼
             🔴 PHASE 5
        Controlled Validation
                    │
                    ▼
             🌍 REAL PRODUCT
```

---

# 🔮 Future Product Vision

### Current Prototype

🧪 Breadboard
🧠 ESP32
📡 Wi-Fi
📲 Telegram
🔌 External wiring

### Future Wearable

⌚ Compact form factor
⚡ Custom PCB
🔋 Low-power design
📡 GSM/LTE
🧠 Adaptive sensing
🛡️ Wearable enclosure

---

# ⚠️ Honest Limitations

We believe strong engineering means clearly separating **what works today** from **what comes next**.

* 🎙️ Sound detection is based on intensity/events, not speech recognition.
* 📡 Telegram currently requires Wi-Fi/internet.
* 📍 GPS requires satellite visibility.
* ❤️ HR accuracy depends on proper sensor contact.
* 🧠 Thresholds are prototype parameters.
* 🧪 Large-scale real-world validation is still required.
* 🏥 This is not a certified medical or emergency-response device.

### **Prototype ≠ Product**

The current goal is to validate the **multimodal safety concept and emergency-response pipeline**.

---

# 🔐 Security

### 🚨 NEVER upload real credentials to GitHub.

Use:

```cpp
#define BOT_TOKEN "YOUR_BOT_TOKEN"
#define CHAT_ID   "YOUR_CHAT_ID"

const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
```

Keep real credentials outside the public repository.

---

# 👥 Team SheShield

<div align="center">

## 🛡️ Team

| 👤 Member            | 🧩 Role                            | 🎓 Institution             |
| -------------------- | ---------------------------------- | -------------------------- |
| **Harsh Parmar**     | 🧠 Team Lead · Hardware & Firmware | Marwadi University, Rajkot |
| **Mirali Sankaliya** | 📲 SOS Integration · Documentation | Atmiya University          |

</div>

---

# 🏆 Why SheShield?

<div align="center">

### **The problem is not always that the user doesn't want to ask for help.**

### **Sometimes the problem is that they cannot.**

<br>

❤️ Sense

↓

🧠 Interpret

↓

🎯 Score

↓

⚠️ Warn

↓

⏳ Allow Cancellation

↓

🚨 Escalate

↓

📲 Communicate

↓

📍 Locate

</div>

---

# 🌟 Vision

<div align="center">

## **From button-dependent safety**

## **to context-aware wearable protection.**

<br>

⌚ **Compact**
🧠 **Intelligent**
📡 **Connected**
🎯 **Context-Aware**
🛡️ **User-Centric**

<br>

# 🛡️ SheShield

### **Sense. Protect. Respond.**

❤️ 🎙️ 📳 💥 📍 📺 📲 🚨

<br>

**IEEE WIE ILS 2026 — Track 3: SheLeads**

</div>

---

<div align="center">

### ⭐ If you find this project interesting, consider giving it a star!

<img src="https://img.shields.io/github/stars/YOUR_USERNAME/YOUR_REPOSITORY?style=for-the-badge&logo=github">

</div>

---

# 📜 License

Released under the **MIT License** for educational, research and hackathon purposes.
