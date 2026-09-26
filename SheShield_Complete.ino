// ============================================================================
//                         SHESHIELD COMPLETE
//              ESP32 WOMEN'S SAFETY WEARABLE PROTOTYPE
// ============================================================================
//
// HARDWARE / FIXED PINS
// ----------------------------------------------------------------------------
// ESP32 DevKit
//
// MAX30102        -> I2C SDA 21, SCL 22
// MPU6050         -> I2C SDA 21, SCL 22
// SH1106 OLED     -> I2C SDA 21, SCL 22
//
// GPS NEO-6M:
// GPS TX -> ESP32 GPIO 4 (RX2)
// GPS RX -> ESP32 GPIO 5 (TX2)
//
// Sound Sensor    -> GPIO 34
// Buzzer          -> GPIO 26
// RED LED         -> GPIO 27
// GREEN LED       -> GPIO 25
// Button          -> GPIO 13 INPUT_PULLUP
//
// ============================================================================
//
// TELEGRAM COMMANDS
// ----------------------------------------------------------------------------
// /start
// /help
// /status
// /location
// /test
// /reset
// /sos
// /on
// /off
//
// ============================================================================
//
// BUTTON LOGIC
// ----------------------------------------------------------------------------
// QUICK PRESS (release before 1.2 sec), works from any state:
//   SYSTEM OFF  -> SYSTEM ON
//   MONITORING  -> SYSTEM OFF
//   COUNTDOWN   -> CANCEL (back to MONITORING)
//   SOS ACTIVE  -> RESET  (back to MONITORING)
//
// LONG HOLD (>= 8 sec), only while MONITORING:
//   Immediately fires MANUAL SOS (starts the 8-sec cancellable countdown)
//
// ============================================================================
//
// AUTOMATIC DISTRESS LOGIC
// ----------------------------------------------------------------------------
// Personal BPM baseline is captured automatically.
//
// HR rise:
//   +15 BPM -> +30 score
//   +25 BPM -> +60 score   (alone crosses SOS threshold)
//
// SOFT VOICE:
//   1 event -> +20 score
//   2 events -> +40 score
//   3 events -> +60 score  (alone crosses SOS threshold, fires INSTANTLY)
//
// Motion (MPU6050):
//   Sustained motion (smoothed signal)      -> +15
//   Sudden impact / fall-like (raw signal)  -> +30
//
// 3 soft voice events within 5 seconds trigger SOS countdown immediately,
// no need to wait for the periodic score scan.
//
// Strong demo patterns:
//   SOFT VOICE + BPM
//   3 SOFT VOICE EVENTS (instant trigger)
//   HR SPIKE alone (instant trigger via score scan)
//   HR SPIKE + motion/impact
//
// When score >= 55 (or 3rd voice event fires directly):
//   8-second cancellation countdown
//
// ============================================================================

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <U8g2lib.h>

#include "MAX30105.h"
#include "heartRate.h"

#include <HardwareSerial.h>
#include <TinyGPS++.h>

#include <UniversalTelegramBot.h>
#include <math.h>


// ============================================================================
// USER CONFIGURATION
// ============================================================================

// IMPORTANT:
// Paste your CURRENT credentials here.
// Do not post them publicly.

#define BOT_TOKEN "I AM SORRY BECUASE I CAN'T DO THIIS "
#define CHAT_ID   "SORRY I CAN'T DO THIS "

const char* WIFI_SSID     = "HARSH__&___Kishori";
const char* WIFI_PASSWORD = "123456789";


// ============================================================================
// FIXED PINS
// ============================================================================

const int SOUND_PIN  = 34;
const int BUZZER_PIN = 26;
const int RED_LED    = 27;
const int GREEN_LED  = 25;
const int BUTTON_PIN = 13;

#define RXD2 4
#define TXD2 5


// ============================================================================
// OBJECTS
// ============================================================================

HardwareSerial neogps(2);
TinyGPSPlus gps;

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);

MAX30105 particleSensor;

bool max30102OK = false;
bool mpuOK = false;


// ============================================================================
// MPU6050
// ============================================================================

#define MPU_ADDR          0x68
#define MPU_PWR_MGMT_1    0x6B
#define MPU_ACCEL_CONFIG  0x1C
#define MPU_ACCEL_XOUT_H  0x3B

float accelX = 0;
float accelY = 0;
float accelZ = 0;

// RAW instantaneous magnitude (updated every read). Used for fast,
// transient events like a sudden impact or a deliberate shake gesture,
// where smoothing would blunt the very spike we're trying to catch.
float gForce = 1.0;

// SMOOTHED magnitude (exponential moving average). Used for sustained
// motion detection, so small sensor noise / hand tremor does not cause
// false positives.
float gForceSmooth = 1.0;

bool motionDetected = false;
bool impactDetected = false;

unsigned long lastMotionTime = 0;

const float MOTION_G = 1.30;
const float IMPACT_G = 2.20;


// ============================================================================
// MPU HAND-GESTURE SYSTEM ON
// ============================================================================
//
// This is a motion gesture, not literal hand recognition.
//
// A strong movement while the system is OFF can arm the prototype.
// Two qualifying spikes, close together in time, are required to reduce
// accidental activation from a single bump or drop.
// ============================================================================

const float HAND_GESTURE_G = 1.55;

const int HAND_GESTURE_REQUIRED = 2;

int handGestureSamples = 0;

unsigned long lastHandGestureSample = 0;

const unsigned long HAND_GESTURE_WINDOW = 900;


// ============================================================================
// TELEGRAM
// ============================================================================

WiFiClientSecure telegramClient;
UniversalTelegramBot bot(BOT_TOKEN, telegramClient);

bool telegramReady = false;

unsigned long lastWiFiAttempt = 0;
unsigned long lastTelegramPoll = 0;

const unsigned long WIFI_RECONNECT_INTERVAL = 10000;
const unsigned long TELEGRAM_POLL_INTERVAL = 1500;

unsigned long telegramMessagesSent = 0;


// ============================================================================
// SYSTEM STATES
// ============================================================================

enum SystemState
{
  SYS_OFF,
  MONITORING,
  COUNTDOWN,
  SOS_ACTIVE
};

SystemState state = SYS_OFF;


// ============================================================================
// GPS
// ============================================================================

double latitude = 0;
double longitude = 0;
double altitude = 0;
double speedKmph = 0;
double hdop = 0;

unsigned long satellites = 0;

bool gpsFix = false;
unsigned long gpsLastFixTime = 0;


// ============================================================================
// HEART RATE
// ============================================================================

const byte RATE_SIZE = 4;

byte rates[RATE_SIZE];
byte rateSpot = 0;

long lastBeat = 0;

float instantBPM = 0;
int averageBPM = 0;

bool fingerDetected = false;


// ============================================================================
// HEART RATE LEVEL
// ============================================================================

enum HRLevel
{
  HR_LOW,
  HR_MEDIUM,
  HR_HIGH,
  HR_WAIT
};

HRLevel hrLevel = HR_WAIT;


// ============================================================================
// PERSONAL BPM BASELINE
// ============================================================================

bool baselineCaptured = false;

float baselineSum = 0;
int baselineSamples = 0;

float bpmBaseline = 0;

const int BASELINE_SAMPLES_NEEDED = 2;


// ============================================================================
// SOUND SENSOR
// ============================================================================
//
// LOWER THRESHOLD FOR SOFT SPEECH
//
// The sensor detects sound intensity rather than recognizing the word
// "Bachaao". Three separate soft voice bursts are treated as three events.
//
// ============================================================================

float soundBaseline = 0;
float soundEnvelope = 0;
float soundDifference = 0;

// Original was much higher.
// Lower values allow quieter speech to be detected.
const float SOUND_SENSITIVITY = 4.0;

const int SOUND_MINIMUM = 2;

const int SOUND_EVENTS_MAX = 3;

const unsigned long SOUND_WINDOW = 5000;

// Prevents one continuous voice sound from becoming multiple events.
const unsigned long SOUND_LOCKOUT = 350;

int soundEvents = 0;

unsigned long soundWindowStart = 0;
unsigned long lastSoundEvent = 0;

bool soundActive = false;


// ============================================================================
// DEMO HR + SOUND CONDITION
// ============================================================================

const int DEMO_HR_THRESHOLD = 45;

bool loudVoiceDetected = false;


// ============================================================================
// DISTRESS SCORE
// ============================================================================

int distressScore = 0;

const int TRIGGER_THRESHOLD = 55;


// ============================================================================
// COUNTDOWN
// ============================================================================

const unsigned long COUNTDOWN_TIME = 8000;

unsigned long countdownStart = 0;

int countdownSecond = 8;

String countdownReason = "";

bool sosTelegramSent = false;


// ============================================================================
// BUTTON
// ============================================================================

bool lastReading = HIGH;
bool stableState = HIGH;

unsigned long lastChangeTime = 0;
unsigned long pressStartTime = 0;

const unsigned long BUTTON_DEBOUNCE = 40;

const unsigned long SHORT_PRESS_MAX = 1200;

// Holding the button for 8 seconds fires an instant manual SOS.
const unsigned long LONG_PRESS_SOS = 8000;

// Prevents the short-press action from also firing once a long-press
// SOS has already been triggered on this same physical press.
bool longPressHandled = false;


// ============================================================================
// OLED
// ============================================================================

bool messageActive = false;

String messageLine1 = "";
String messageLine2 = "";

unsigned long messageStart = 0;

const unsigned long MESSAGE_DURATION = 2000;

byte oledPage = 0;

unsigned long lastOLED = 0;
unsigned long lastPageChange = 0;

const unsigned long OLED_INTERVAL = 100;
const unsigned long PAGE_TIME = 2500;


// ============================================================================
// TIMERS
// ============================================================================

unsigned long lastSerial = 0;

const unsigned long SERIAL_INTERVAL = 1000;

unsigned long lastMotionRead = 0;

const unsigned long MOTION_INTERVAL = 25;

unsigned long lastScoreUpdate = 0;

const unsigned long SCORE_INTERVAL = 100;


// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void connectWiFi();
void handleWiFiReconnect();

void readGPS();

void initMPU();
void readMPU();

void initMAX30102();
void readMAX30102();

void processSound(int raw);
void registerSoundEvent();
int measureSoundLevel();

int computeDistressScore();

String buildTriggerReason();

void handleButton();

void turnSystemOn();
void turnSystemOff();

void manualSOS();
void cancelAlert();
void resetFromSOS();

void startCountdown(String reason);
void handleCountdown();

void backToMonitoring();

void showFullScreenMessage(String l1, String l2);

void updateOLED();
void drawFullScreenMessage();
void drawSystemOffScreen();
void drawCountdownScreen();
void drawSOSScreen();
void drawStatusPage();
void drawSensorsPage();
void drawGPSPage();
void drawMotionPage();

void printSerialDashboard();

bool isAuthorizedChat(String chatID);

void sendTelegramMessage(
  String chatID,
  String message,
  String parseMode = "HTML"
);

void sendTelegramLocationPin(
  String chatID,
  double lat,
  double lon
);

void handleTelegram();

void sendTelegramStartup();
void sendTelegramSOS(String reason);
void sendTelegramStatus(String chatID);
void sendTelegramLocation(String chatID);
void sendTelegramHelp(String chatID);
void sendTelegramTest(String chatID);
void sendTelegramReset(String chatID);
void sendTelegramSystemOn(String chatID);
void sendTelegramSystemOff(String chatID);

String getStateText();
String getHRText();
String getGoogleMapsLink();


// ============================================================================
// SETUP
// ============================================================================

void setup()
{
  Serial.begin(115200);

  delay(500);


  // ----------------------------------------------------------
  // GPIO
  // ----------------------------------------------------------

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin(21, 22);

  Wire.setClock(400000);


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  oled.begin();

  oled.setContrast(180);

  oled.clearBuffer();

  oled.setFont(u8g2_font_6x10_tf);

  oled.drawStr(0, 20, "SheShield");

  oled.drawStr(0, 36, "Booting...");

  oled.sendBuffer();


  // ----------------------------------------------------------
  // SERIAL HEADER
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("================================================");
  Serial.println("             SHESHIELD PROTOTYPE");
  Serial.println("================================================");


  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  neogps.begin(
    9600,
    SERIAL_8N1,
    RXD2,
    TXD2
  );

  Serial.println("[GPS] UART READY");
  Serial.println("[GPS] RX = GPIO4");
  Serial.println("[GPS] TX = GPIO5");


  // ----------------------------------------------------------
  // MAX30102
  // ----------------------------------------------------------

  initMAX30102();


  // ----------------------------------------------------------
  // MPU6050
  // ----------------------------------------------------------

  initMPU();


  // ----------------------------------------------------------
  // SOUND CALIBRATION
  // ----------------------------------------------------------

  Serial.println("[SOUND] Calibrating...");

  long total = 0;

  for (int i = 0; i < 100; i++)
  {
    total += measureSoundLevel();
    delay(10);
  }

  soundBaseline = total / 100.0;

  Serial.print("[SOUND] Baseline = ");
  Serial.println(soundBaseline);


  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  connectWiFi();


  if (WiFi.status() == WL_CONNECTED)
  {
    telegramClient.setInsecure();

    telegramReady = true;

    delay(300);

    sendTelegramStartup();
  }


  // ----------------------------------------------------------
  // START OFF
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("================================================");
  Serial.println(" SYSTEM OFF");
  Serial.println(" Quick press button to turn ON");
  Serial.println(" MPU gesture can also arm prototype");
  Serial.println("================================================");

  showFullScreenMessage(
    "SYSTEM OFF",
    "Quick press = ON"
  );
}


// ============================================================================
// MAIN LOOP
// ============================================================================

void loop()
{
  unsigned long now = millis();


  // ----------------------------------------------------------
  // BUTTON ALWAYS ACTIVE
  // ----------------------------------------------------------

  handleButton();


  // ----------------------------------------------------------
  // WIFI / TELEGRAM
  // ----------------------------------------------------------

  handleWiFiReconnect();

  handleTelegram();


  // ----------------------------------------------------------
  // SENSORS
  // ----------------------------------------------------------

  //
  // MPU is read even when OFF so the prototype can recognize
  // a movement gesture for SYSTEM ON.
  //
  if (now - lastMotionRead >= MOTION_INTERVAL)
  {
    lastMotionRead = now;

    readMPU();
  }


  //
  // GPS is read every loop cycle regardless of system state so the
  // module stays warmed up and acquires a satellite fix as fast as
  // possible, instead of only starting to search once armed.
  //
  readGPS();


  // ----------------------------------------------------------
  // SENSORS ONLY WHEN SYSTEM IS ON
  // ----------------------------------------------------------

  if (state != SYS_OFF)
  {
    readMAX30102();


    // --------------------------------------------------------
    // SOUND
    // --------------------------------------------------------

    int soundLevel = measureSoundLevel();

    processSound(soundLevel);


    // --------------------------------------------------------
    // COUNTDOWN
    // --------------------------------------------------------

    if (state == COUNTDOWN)
    {
      handleCountdown();
    }


    // --------------------------------------------------------
    // SOS ACTIVE
    // --------------------------------------------------------

    else if (state == SOS_ACTIVE)
    {
      digitalWrite(GREEN_LED, LOW);

      digitalWrite(RED_LED, HIGH);

      digitalWrite(BUZZER_PIN, HIGH);
    }


    // --------------------------------------------------------
    // MONITORING
    // --------------------------------------------------------

    else if (state == MONITORING)
    {
      digitalWrite(GREEN_LED, HIGH);

      digitalWrite(RED_LED, LOW);

      digitalWrite(BUZZER_PIN, LOW);


      if (now - lastScoreUpdate >= SCORE_INTERVAL)
      {
        lastScoreUpdate = now;

        distressScore = computeDistressScore();


        if (distressScore >= TRIGGER_THRESHOLD)
        {
          startCountdown(
            buildTriggerReason()
          );
        }
      }
    }
  }


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (now - lastOLED >= OLED_INTERVAL)
  {
    lastOLED = now;

    if (now - lastPageChange >= PAGE_TIME)
    {
      lastPageChange = now;

      oledPage++;

      if (oledPage >= 4)
      {
        oledPage = 0;
      }
    }

    updateOLED();
  }


  // ----------------------------------------------------------
  // SERIAL
  // ----------------------------------------------------------

  if (now - lastSerial >= SERIAL_INTERVAL)
  {
    lastSerial = now;

    printSerialDashboard();
  }
}


// ============================================================================
// BUTTON STATE MACHINE
// ----------------------------------------------------------------------------
// Quick press (release < 1.2s), any state:
//   OFF        -> ON
//   MONITORING -> OFF
//   COUNTDOWN  -> CANCEL
//   SOS ACTIVE -> RESET
//
// Long hold (>= 8s) while held down, only in MONITORING:
//   Fires manual SOS immediately (does not wait for release)
// ============================================================================

void handleButton()
{
  unsigned long now = millis();

  bool reading = digitalRead(BUTTON_PIN);


  if (reading != lastReading)
  {
    lastChangeTime = now;

    lastReading = reading;
  }


  if (now - lastChangeTime < BUTTON_DEBOUNCE)
  {
    return;
  }


  // ----------------------------------------------------------
  // BUTTON JUST CHANGED STATE (PRESS OR RELEASE)
  // ----------------------------------------------------------

  if (reading != stableState)
  {
    stableState = reading;


    if (stableState == LOW)
    {
      // Button just went down.
      pressStartTime = now;

      longPressHandled = false;

      return;
    }


    // --------------------------------------------------------
    // BUTTON JUST RELEASED
    // --------------------------------------------------------

    unsigned long duration =
      now - pressStartTime;


    // If the long-press SOS already fired during this hold,
    // don't also apply a short-press action on release.
    if (longPressHandled)
    {
      longPressHandled = false;

      return;
    }


    if (duration >= SHORT_PRESS_MAX)
    {
      // Held too long for a "quick press" but never reached the
      // 8s SOS threshold (or wasn't in MONITORING) -> ignore.
      return;
    }


    // --------------------------------------------------------
    // QUICK PRESS ACTION (per current state)
    // --------------------------------------------------------

    if (state == SYS_OFF)
    {
      turnSystemOn();
    }
    else if (state == MONITORING)
    {
      turnSystemOff();
    }
    else if (state == COUNTDOWN)
    {
      cancelAlert();
    }
    else if (state == SOS_ACTIVE)
    {
      resetFromSOS();
    }

    return;
  }


  // ----------------------------------------------------------
  // BUTTON STILL HELD DOWN -> WATCH FOR 8s LONG PRESS SOS
  // ----------------------------------------------------------

  if (
    stableState == LOW &&
    !longPressHandled &&
    state == MONITORING
  )
  {
    if (now - pressStartTime >= LONG_PRESS_SOS)
    {
      longPressHandled = true;

      Serial.println();
      Serial.println(">>> 8s LONG PRESS -> MANUAL SOS");

      manualSOS();
    }
  }
}


// ============================================================================
// SYSTEM ON
// ============================================================================

void turnSystemOn()
{
  state = MONITORING;

  backToMonitoring();


  Serial.println();
  Serial.println(">>> SYSTEM ON");


  showFullScreenMessage(
    "SYSTEM ON",
    "Monitoring Active"
  );


  digitalWrite(GREEN_LED, HIGH);

  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER_PIN, LOW);


  if (telegramReady)
  {
    sendTelegramSystemOn(CHAT_ID);
  }
}


// ============================================================================
// SYSTEM OFF
// ============================================================================

void turnSystemOff()
{
  state = SYS_OFF;


  digitalWrite(GREEN_LED, LOW);

  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER_PIN, LOW);


  Serial.println();
  Serial.println(">>> SYSTEM OFF");


  showFullScreenMessage(
    "SYSTEM OFF",
    "Monitoring Paused"
  );


  if (telegramReady)
  {
    sendTelegramSystemOff(CHAT_ID);
  }
}


// ============================================================================
// MANUAL SOS
// ============================================================================

void manualSOS()
{
  Serial.println();
  Serial.println(">>> MANUAL SOS BUTTON");

  startCountdown(
    "MANUAL SOS BUTTON (8s HOLD)"
  );
}


// ============================================================================
// CANCEL
// ============================================================================

void cancelAlert()
{
  Serial.println();
  Serial.println(">>> ALERT CANCELLED");


  state = MONITORING;

  backToMonitoring();


  showFullScreenMessage(
    "ALERT CANCELLED",
    "System Re-Armed"
  );


  if (telegramReady)
  {
    sendTelegramMessage(
      CHAT_ID,

      "🟢 <b>ALERT CANCELLED</b>\n"
      "━━━━━━━━━━━━━━━━━━\n\n"
      "🔘 Cancel button detected\n"
      "🛑 Emergency escalation stopped\n"
      "🛡️ SheShield is re-armed\n"
      "📡 Monitoring continues normally",

      "HTML"
    );
  }
}


// ============================================================================
// RESET FROM SOS
// ============================================================================

void resetFromSOS()
{
  Serial.println();
  Serial.println(">>> SOS RESET");


  state = MONITORING;

  backToMonitoring();


  showFullScreenMessage(
    "RESET",
    "Monitoring Resumed"
  );


  if (telegramReady)
  {
    sendTelegramMessage(
      CHAT_ID,

      "🔄 <b>SHESHIELD RESET</b>\n"
      "━━━━━━━━━━━━━━━━━━\n\n"
      "🟢 Emergency state cleared\n"
      "🛡️ System returned to monitoring mode\n"
      "📡 Sensors active",

      "HTML"
    );
  }
}


// ============================================================================
// RESET MONITORING VARIABLES
// ============================================================================

void backToMonitoring()
{
  soundEvents = 0;

  soundWindowStart = 0;

  soundActive = false;

  loudVoiceDetected = false;


  motionDetected = false;

  impactDetected = false;


  distressScore = 0;

  sosTelegramSent = false;


  countdownSecond = 8;

  countdownReason = "";


  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER_PIN, LOW);

  digitalWrite(GREEN_LED, HIGH);
}


// ============================================================================
// START COUNTDOWN
// ============================================================================

void startCountdown(String reason)
{
  if (state != MONITORING)
  {
    return;
  }


  state = COUNTDOWN;

  countdownStart = millis();

  countdownSecond = 8;

  countdownReason = reason;


  digitalWrite(GREEN_LED, LOW);

  digitalWrite(RED_LED, LOW);

  digitalWrite(BUZZER_PIN, LOW);


  Serial.println();
  Serial.print(">>> DISTRESS DETECTED: ");
  Serial.println(reason);


  if (telegramReady)
  {
    String msg =
      "⚠️ <b>SHESHIELD ALERT DETECTED</b>\n"
      "━━━━━━━━━━━━━━━━━━\n\n"

      "🚨 Trigger: <b>" +
      reason +
      "</b>\n"

      "🎯 Distress Score: <b>" +
      String(distressScore) +
      "/100</b>\n\n"

      "❤️ BPM: <b>" +
      String(
        averageBPM > 0
        ? averageBPM
        : 0
      ) +
      "</b>\n"

      "🎙️ Sound Events: <b>" +
      String(soundEvents) +
      "/3</b>\n"

      "📳 Motion: <b>" +
      String(
        motionDetected ? "DETECTED" : "NORMAL"
      ) +
      "</b>\n\n"

      "⏳ <b>8 SECOND CANCELLATION WINDOW</b>\n"
      "🔘 Short-press the physical button to cancel.\n\n"

      "If not cancelled, SOS will be activated automatically.";

    sendTelegramMessage(
      CHAT_ID,
      msg,
      "HTML"
    );
  }
}


// ============================================================================
// COUNTDOWN
// ============================================================================

void handleCountdown()
{
  unsigned long elapsed =
    millis() - countdownStart;


  bool pulse =
    ((elapsed / 200) % 2);


  digitalWrite(
    RED_LED,
    pulse
  );

  digitalWrite(
    BUZZER_PIN,
    pulse
  );

  digitalWrite(
    GREEN_LED,
    LOW
  );


  int remaining =
    (elapsed >= COUNTDOWN_TIME)
    ? 0
    : (COUNTDOWN_TIME - elapsed + 999) / 1000;


  if (remaining != countdownSecond)
  {
    countdownSecond = remaining;


    Serial.print(">>> SOS IN ");
    Serial.print(remaining);
    Serial.println(" SEC");
  }


  if (elapsed >= COUNTDOWN_TIME)
  {
    state = SOS_ACTIVE;


    digitalWrite(
      RED_LED,
      HIGH
    );

    digitalWrite(
      BUZZER_PIN,
      HIGH
    );

    digitalWrite(
      GREEN_LED,
      LOW
    );


    Serial.println();
    Serial.println("*** SOS ACTIVE ***");


    showFullScreenMessage(
      "SOS SENT",
      "HELP ALERTED"
    );


    sendTelegramSOS(
      countdownReason
    );
  }
}


// ============================================================================
// WIFI
// ============================================================================

void connectWiFi()
{
  Serial.print("[WiFi] Connecting");

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );


  unsigned long start =
    millis();


  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - start < 12000
  )
  {
    delay(300);

    Serial.print(".");
  }


  Serial.println();


  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println(
      "[WiFi] CONNECTED"
    );

    Serial.print(
      "[WiFi] IP: "
    );

    Serial.println(
      WiFi.localIP()
    );

    telegramClient.setInsecure();

    telegramReady = true;
  }
  else
  {
    Serial.println(
      "[WiFi] FAILED"
    );

    telegramReady = false;
  }
}


// ============================================================================
// WIFI RECONNECT
// ============================================================================

void handleWiFiReconnect()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    telegramReady = true;

    return;
  }


  telegramReady = false;


  unsigned long now =
    millis();


  if (
    now - lastWiFiAttempt >=
    WIFI_RECONNECT_INTERVAL
  )
  {
    lastWiFiAttempt = now;


    Serial.println(
      ">>> WiFi reconnecting..."
    );


    WiFi.disconnect();

    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );
  }
}


// ============================================================================
// TELEGRAM AUTH
// ============================================================================

bool isAuthorizedChat(String chatID)
{
  return chatID ==
         String(CHAT_ID);
}


// ============================================================================
// TELEGRAM SEND
// ============================================================================

void sendTelegramMessage(
  String chatID,
  String message,
  String parseMode
)
{
  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    return;
  }


  bool ok =
    bot.sendMessage(
      chatID,
      message,
      parseMode
    );


  if (ok)
  {
    telegramMessagesSent++;
  }
}


// ============================================================================
// TELEGRAM LOCATION PIN
// ============================================================================

void sendTelegramLocationPin(
  String chatID,
  double lat,
  double lon
)
{
  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    return;
  }


  HTTPClient https;


  String url =
    "https://api.telegram.org/bot" +
    String(BOT_TOKEN) +

    "/sendLocation?chat_id=" +
    chatID +

    "&latitude=" +
    String(lat, 6) +

    "&longitude=" +
    String(lon, 6);


  https.begin(
    telegramClient,
    url
  );


  https.GET();


  https.end();
}


// ============================================================================
// TELEGRAM HANDLER
// ============================================================================

void handleTelegram()
{
  if (
    WiFi.status() !=
    WL_CONNECTED
  )
  {
    return;
  }


  unsigned long now =
    millis();


  if (
    now - lastTelegramPoll <
    TELEGRAM_POLL_INTERVAL
  )
  {
    return;
  }


  lastTelegramPoll = now;


  int numNew =
    bot.getUpdates(
      bot.last_message_received + 1
    );


  while (numNew)
  {
    for (int i = 0; i < numNew; i++)
    {
      String chatId =
        bot.messages[i].chat_id;

      String text =
        bot.messages[i].text;


      text.trim();


      if (
        !isAuthorizedChat(chatId)
      )
      {
        bot.sendMessage(
          chatId,

          "⛔ <b>ACCESS DENIED</b>\n\n"
          "This SheShield device is privately configured.",

          "HTML"
        );

        continue;
      }


      if (text == "/start")
      {
        sendTelegramHelp(chatId);
      }

      else if (text == "/help")
      {
        sendTelegramHelp(chatId);
      }

      else if (text == "/status")
      {
        sendTelegramStatus(chatId);
      }

      else if (text == "/location")
      {
        sendTelegramLocation(chatId);
      }

      else if (text == "/test")
      {
        sendTelegramTest(chatId);
      }

      else if (text == "/reset")
      {
        sendTelegramReset(chatId);
      }

      else if (text == "/sos")
      {
        if (state == MONITORING)
        {
          startCountdown(
            "TELEGRAM MANUAL SOS"
          );
        }
        else
        {
          sendTelegramMessage(
            chatId,

            "⚠️ <b>SOS NOT STARTED</b>\n\n"
            "System is not currently in monitoring mode.",

            "HTML"
          );
        }
      }

      else if (text == "/on")
      {
        if (state == SYS_OFF)
        {
          turnSystemOn();
        }
        else
        {
          sendTelegramMessage(
            chatId,

            "🟢 <b>SYSTEM ALREADY ON</b>\n\n"
            "🛡️ Monitoring is active.",

            "HTML"
          );
        }
      }

      else if (text == "/off")
      {
        if (state != SYS_OFF)
        {
          turnSystemOff();
        }
        else
        {
          sendTelegramMessage(
            chatId,

            "🔴 <b>SYSTEM ALREADY OFF</b>\n\n"
            "Monitoring is paused.",

            "HTML"
          );
        }
      }

      else
      {
        sendTelegramMessage(
          chatId,

          "❓ <b>UNKNOWN COMMAND</b>\n\n"
          "Available commands:\n"
          "🟢 /on\n"
          "🔴 /off\n"
          "📊 /status\n"
          "📍 /location\n"
          "🧪 /test\n"
          "🔄 /reset\n"
          "🆘 /sos\n"
          "ℹ️ /help",

          "HTML"
        );
      }
    }


    numNew =
      bot.getUpdates(
        bot.last_message_received + 1
      );
  }
}


// ============================================================================
// TELEGRAM STARTUP
// ============================================================================

void sendTelegramStartup()
{
  String msg =

    "🛡️ <b>SHESHIELD IS ONLINE</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🟢 <b>System Status:</b> OFF\n"
    "📡 <b>Monitoring:</b> STANDBY\n"
    "📶 <b>WiFi:</b> CONNECTED\n\n"

    "🔍 <b>LIVE SENSOR CHECK</b>\n"

    "❤️ MAX30102: " +
    String(
      max30102OK
      ? "✅ READY"
      : "❌ ERROR"
    ) +
    "\n"

    "📳 MPU6050: " +
    String(
      mpuOK
      ? "✅ READY"
      : "❌ ERROR"
    ) +
    "\n"

    "📍 GPS: 🛰️ SEARCHING\n"

    "📺 OLED: ✅ ACTIVE\n"

    "🎙️ Sound: ✅ HIGH SENSITIVITY\n\n"

    "🚨 <b>EMERGENCY LOGIC</b>\n"
    "• Multimodal distress scoring\n"
    "• Loud sound + HR support\n"
    "• 3 sound events / 5 sec -> INSTANT SOS\n"
    "• 8 sec cancellation window\n"
    "• Quick press → TOGGLE ON/OFF\n"
    "• Quick press (COUNTDOWN) → CANCEL\n"
    "• Quick press (SOS ACTIVE) → RESET\n"
    "• 8 sec HOLD (MONITORING) → MANUAL SOS\n"
    "• Timeout → AUTOMATIC SOS\n\n"

    "❤️ <b>HEART RATE</b>\n"
    "LOW: &lt;45 BPM\n"
    "MEDIUM: 45–59 BPM\n"
    "HIGH: ≥60 BPM\n\n"

    "📲 <b>TELEGRAM CONTROL</b>\n"
    "/on /off /status /location\n"
    "/test /reset /sos /help\n\n"

    "🛡️ <b>SheShield is ready for operation.</b>";

  sendTelegramMessage(
    CHAT_ID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM SYSTEM ON
// ============================================================================

void sendTelegramSystemOn(
  String chatID
)
{
  String msg =

    "🟢 <b>SHESHIELD SYSTEM ON</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🛡️ <b>System:</b> ARMED\n"
    "📡 <b>Monitoring:</b> ACTIVE\n"
    "💚 <b>Green LED:</b> ON\n"
    "❤️ <b>Heart Rate:</b> MONITORING\n"
    "📳 <b>Motion:</b> MONITORING\n"
    "🎙️ <b>Sound:</b> MONITORING\n"
    "📍 <b>GPS:</b> ACQUIRING\n\n"

    "🚨 <b>Protection logic active.</b>\n"
    "Multimodal distress detection is now running.";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM SYSTEM OFF
// ============================================================================

void sendTelegramSystemOff(
  String chatID
)
{
  String msg =

    "🔴 <b>SHESHIELD SYSTEM OFF</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🔴 <b>System:</b> OFF\n"
    "📡 <b>Monitoring:</b> PAUSED\n"
    "💡 <b>LEDs:</b> OFF\n"
    "🔊 <b>Buzzer:</b> OFF\n"
    "📳 <b>Motion monitoring:</b> PAUSED\n"
    "🎙️ <b>Sound monitoring:</b> PAUSED\n\n"

    "🛡️ Emergency monitoring is currently disabled.\n\n"

    "▶️ Quick-press the physical button\n"
    "or use <b>/on</b> to resume.";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM SOS
// ============================================================================

void sendTelegramSOS(
  String reason
)
{
  if (sosTelegramSent)
  {
    return;
  }


  sosTelegramSent = true;


  String msg =

    "🚨🚨 <b>SHESHIELD EMERGENCY ALERT</b> 🚨🚨\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🆘 <b>SOS ACTIVATED</b>\n\n"

    "⚠️ <b>Trigger:</b> " +
    reason +
    "\n"

    "🎯 <b>DISTRESS SCORE:</b> " +
    String(distressScore) +
    "/100\n\n"

    "❤️ <b>HEART RATE</b>\n"
    "BPM: " +
    String(
      averageBPM > 0
      ? averageBPM
      : 0
    ) +
    "\n"

    "Level: " +
    getHRText() +
    "\n\n"

    "🎙️ <b>SOUND</b>\n"
    "Events: " +
    String(soundEvents) +
    "/3\n"

    "Envelope: " +
    String(soundEnvelope, 1) +
    "\n\n"

    "📳 <b>MOTION</b>\n"
    "Motion: " +
    String(
      motionDetected
      ? "DETECTED"
      : "NORMAL"
    ) +
    "\n"

    "Impact/Fall-like: " +
    String(
      impactDetected
      ? "DETECTED"
      : "NO"
    ) +
    "\n\n";


  if (gpsFix)
  {
    msg +=

      "📍 <b>EMERGENCY LOCATION</b>\n"

      "Latitude: " +
      String(latitude, 6) +
      "\n"

      "Longitude: " +
      String(longitude, 6) +
      "\n"

      "🛰️ Satellites: " +
      String(satellites) +
      "\n"

      "📡 HDOP: " +
      String(hdop, 2) +
      "\n\n"

      "🗺️ <a href=\"" +
      getGoogleMapsLink() +
      "\">OPEN GOOGLE MAPS</a>\n\n";
  }
  else
  {
    msg +=

      "📍 <b>EMERGENCY LOCATION</b>\n"

      "⚠️ NO VALID GPS FIX\n"

      "GPS location will become available after satellite acquisition.\n\n";
  }


  msg +=

    "🔴 <b>SOS STATE: ACTIVE</b>\n"
    "⚠️ Please check the user immediately.\n\n"

    "🛡️ SheShield emergency protection is active.";


  sendTelegramMessage(
    CHAT_ID,
    msg,
    "HTML"
  );


  if (gpsFix)
  {
    sendTelegramLocationPin(
      CHAT_ID,
      latitude,
      longitude
    );
  }
}


// ============================================================================
// TELEGRAM STATUS
// ============================================================================

void sendTelegramStatus(
  String chatID
)
{
  String msg =

    "📊 <b>SHESHIELD LIVE STATUS</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🛡️ State: <b>" +
    getStateText() +
    "</b>\n"

    "🎯 Distress Score: <b>" +
    String(distressScore) +
    "/100</b>\n\n"

    "❤️ BPM: <b>" +
    String(
      averageBPM > 0
      ? averageBPM
      : 0
    ) +
    "</b>\n"

    "❤️ HR Level: <b>" +
    getHRText() +
    "</b>\n"

    "📈 Baseline: " +
    String(
      baselineCaptured
      ? String(bpmBaseline, 1)
      : "CALIBRATING"
    ) +
    "\n\n"

    "🎙️ Sound Events: " +
    String(soundEvents) +
    "/3\n"

    "🎙️ Voice Activity: " +
    String(
      loudVoiceDetected
      ? "DETECTED"
      : "NORMAL"
    ) +
    "\n\n"

    "📳 G-Force: " +
    String(gForce, 2) +
    " G\n"

    "📳 Motion: " +
    String(
      motionDetected
      ? "YES"
      : "NO"
    ) +
    "\n"

    "💥 Impact: " +
    String(
      impactDetected
      ? "YES"
      : "NO"
    ) +
    "\n\n";


  if (gpsFix)
  {
    msg +=

      "📍 GPS: <b>FIXED</b>\n"

      "🛰️ Satellites: " +
      String(satellites) +
      "\n"

      "Lat: " +
      String(latitude, 6) +
      "\n"

      "Lon: " +
      String(longitude, 6) +
      "\n\n";
  }
  else
  {
    msg +=

      "📍 GPS: <b>SEARCHING</b>\n\n";
  }


  msg +=

    "📶 WiFi: " +
    String(
      WiFi.status() == WL_CONNECTED
      ? "CONNECTED"
      : "OFFLINE"
    ) +
    "\n"

    "📲 Telegram: " +
    String(
      telegramReady
      ? "READY"
      : "OFFLINE"
    ) +
    "\n\n"

    "🛡️ Protection system status available in real time.";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM LOCATION
// ============================================================================

void sendTelegramLocation(
  String chatID
)
{
  if (!gpsFix)
  {
    sendTelegramMessage(
      chatID,

      "📍 <b>GPS LOCATION UNAVAILABLE</b>\n\n"
      "🛰️ No valid satellite fix yet.\n"
      "Move the GPS module to an open-sky area and try again.",

      "HTML"
    );

    return;
  }


  String msg =

    "📍 <b>SHESHIELD CURRENT LOCATION</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "Latitude: " +
    String(latitude, 8) +
    "\n"

    "Longitude: " +
    String(longitude, 8) +
    "\n"

    "🛰️ Satellites: " +
    String(satellites) +
    "\n"

    "📡 HDOP: " +
    String(hdop, 2) +
    "\n\n"

    "🗺️ <a href=\"" +
    getGoogleMapsLink() +
    "\">OPEN GOOGLE MAPS</a>";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );


  sendTelegramLocationPin(
    chatID,
    latitude,
    longitude
  );
}


// ============================================================================
// TELEGRAM HELP
// ============================================================================

void sendTelegramHelp(
  String chatID
)
{
  String msg =

    "🛡️ <b>SHESHIELD COMMAND CENTER</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🎛️ <b>SYSTEM CONTROL</b>\n"

    "🟢 /on — Arm system\n"
    "🔴 /off — Turn monitoring OFF\n\n"

    "📊 <b>MONITORING</b>\n"

    "📊 /status — Live sensor status\n"
    "📍 /location — GPS + Google Maps\n"
    "🧪 /test — Hardware communication test\n\n"

    "🚨 <b>EMERGENCY</b>\n"

    "🆘 /sos — Manual SOS\n"
    "🔄 /reset — Reset emergency state\n\n"

    "ℹ️ /help — Show this command center\n\n"

    "🔘 <b>PHYSICAL BUTTON</b>\n"

    "• Quick press: OFF → SYSTEM ON\n"
    "• Quick press: ON → SYSTEM OFF\n"
    "• 8 sec HOLD (while ON) → MANUAL SOS\n"
    "• Countdown + quick press → CANCEL\n"
    "• SOS + quick press → RESET\n\n"

    "🛡️ <b>AUTOMATIC PROTECTION</b>\n"

    "❤️ Personal HR baseline\n"
    "🎙️ Soft voice detection\n"
    "🎙️ 3 voice events / 5 sec → INSTANT SOS\n"
    "📳 Motion / struggle detection\n"
    "💥 Impact / fall-like detection\n"
    "🎯 Multimodal distress score\n"
    "⏳ 8-second cancellation window\n"
    "📍 GPS emergency location";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM TEST
// ============================================================================

void sendTelegramTest(
  String chatID
)
{
  String msg =

    "🧪 <b>SHESHIELD SYSTEM TEST</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🟢 Telegram: <b>COMMUNICATION OK</b>\n"

    "📶 WiFi: " +
    String(
      WiFi.status() == WL_CONNECTED
      ? "CONNECTED"
      : "OFFLINE"
    ) +
    "\n"

    "❤️ MAX30102: " +
    String(
      max30102OK
      ? "✅ READY"
      : "❌ ERROR"
    ) +
    "\n"

    "📳 MPU6050: " +
    String(
      mpuOK
      ? "✅ READY"
      : "❌ ERROR"
    ) +
    "\n"

    "📍 GPS: " +
    String(
      gpsFix
      ? "✅ FIXED"
      : "🛰️ SEARCHING"
    ) +
    "\n"

    "📺 OLED: ✅ ACTIVE\n"

    "🎙️ Sound: ✅ ACTIVE\n"

    "🔘 Button: ✅ ACTIVE\n"

    "🔴 Red LED: ✅ READY\n"

    "🟢 Green LED: ✅ READY\n"

    "🔊 Buzzer: ✅ READY\n\n"

    "🛡️ SheShield hardware test completed.";

  sendTelegramMessage(
    chatID,
    msg,
    "HTML"
  );
}


// ============================================================================
// TELEGRAM RESET
// ============================================================================

void sendTelegramReset(
  String chatID
)
{
  if (state == SYS_OFF)
  {
    sendTelegramMessage(
      chatID,

      "🔴 <b>SYSTEM IS OFF</b>\n\n"
      "Use /on to start monitoring.",

      "HTML"
    );

    return;
  }


  state = MONITORING;

  backToMonitoring();


  sendTelegramMessage(
    chatID,

    "🔄 <b>SHESHIELD RESET</b>\n"
    "━━━━━━━━━━━━━━━━━━\n\n"

    "🟢 System returned to monitoring mode.\n"
    "🛡️ Emergency state cleared.\n"
    "📡 Sensors active.\n"
    "💚 Green LED active.",

    "HTML"
  );
}


// ============================================================================
// GPS READING
// ============================================================================

void readGPS()
{
  while (neogps.available())
  {
    gps.encode(
      neogps.read()
    );
  }


  if (gps.location.isValid())
  {
    latitude =
      gps.location.lat();

    longitude =
      gps.location.lng();

    gpsFix = true;

    gpsLastFixTime =
      millis();
  }


  if (gps.altitude.isValid())
  {
    altitude =
      gps.altitude.meters();
  }


  if (gps.speed.isValid())
  {
    speedKmph =
      gps.speed.kmph();
  }


  if (gps.hdop.isValid())
  {
    hdop =
      gps.hdop.hdop();
  }


  if (gps.satellites.isValid())
  {
    satellites =
      gps.satellites.value();
  }


  if (
    gpsFix &&
    millis() - gpsLastFixTime > 15000
  )
  {
    gpsFix = false;
  }
}


// ============================================================================
// MAX30102 INIT
// ============================================================================

void initMAX30102()
{
  Serial.println(
    "[MAX30102] Initializing..."
  );


  if (
    particleSensor.begin(
      Wire,
      I2C_SPEED_FAST
    )
  )
  {
    max30102OK = true;


    particleSensor.setup(
      80,
      4,
      2,
      100,
      411,
      4096
    );


    particleSensor.setPulseAmplitudeRed(
      0x1F
    );


    particleSensor.setPulseAmplitudeGreen(
      0
    );


    Serial.println(
      "[MAX30102] READY"
    );
  }
  else
  {
    Serial.println(
      "[MAX30102] NOT FOUND"
    );
  }
}


// ============================================================================
// MAX30102 READ
// ============================================================================

void readMAX30102()
{
  if (!max30102OK)
  {
    return;
  }


  long irValue =
    particleSensor.getIR();


  if (irValue < 20000)
  {
    fingerDetected = false;

    hrLevel = HR_WAIT;

    return;
  }


  if (!fingerDetected)
  {
    fingerDetected = true;

    lastBeat = 0;

    instantBPM = 0;

    averageBPM = 0;

    rateSpot = 0;


    for (
      byte i = 0;
      i < RATE_SIZE;
      i++
    )
    {
      rates[i] = 0;
    }
  }


  if (
    checkForBeat(irValue)
  )
  {
    unsigned long now =
      millis();


    long delta =
      now - lastBeat;


    lastBeat = now;


    if (
      delta >= 250 &&
      delta <= 2000
    )
    {
      instantBPM =
        60.0 /
        (delta / 1000.0);


      if (
        instantBPM >= 30 &&
        instantBPM <= 220
      )
      {
        rates[rateSpot] =
          (byte)instantBPM;


        rateSpot =
          (rateSpot + 1) %
          RATE_SIZE;


        int total = 0;

        int count = 0;


        for (
          byte i = 0;
          i < RATE_SIZE;
          i++
        )
        {
          if (rates[i] > 0)
          {
            total += rates[i];

            count++;
          }
        }


        if (count > 0)
        {
          averageBPM =
            total / count;
        }
      }
    }
  }


  if (averageBPM > 0)
  {
    if (averageBPM < 45)
    {
      hrLevel = HR_LOW;
    }
    else if (averageBPM < 60)
    {
      hrLevel = HR_MEDIUM;
    }
    else
    {
      hrLevel = HR_HIGH;
    }
  }


  if (
    !baselineCaptured &&
    fingerDetected &&
    averageBPM > 0
  )
  {
    baselineSum += averageBPM;

    baselineSamples++;


    if (
      baselineSamples >=
      BASELINE_SAMPLES_NEEDED
    )
    {
      bpmBaseline =
        baselineSum /
        baselineSamples;


      baselineCaptured = true;


      Serial.print(
        ">>> BPM BASELINE: "
      );

      Serial.println(
        bpmBaseline
      );
    }
  }
}


// ============================================================================
// MPU6050 INIT
// ============================================================================

void initMPU()
{
  Wire.beginTransmission(
    MPU_ADDR
  );


  if (
    Wire.endTransmission() != 0
  )
  {
    Serial.println(
      "[MPU6050] NOT FOUND"
    );

    mpuOK = false;

    return;
  }


  Wire.beginTransmission(
    MPU_ADDR
  );

  Wire.write(
    MPU_PWR_MGMT_1
  );

  Wire.write(
    0x00
  );

  Wire.endTransmission();


  delay(20);


  Wire.beginTransmission(
    MPU_ADDR
  );

  Wire.write(
    MPU_ACCEL_CONFIG
  );

  Wire.write(
    0x00
  );

  Wire.endTransmission();


  mpuOK = true;


  Serial.println(
    "[MPU6050] READY"
  );
}


// ============================================================================
// MPU6050 READ
// ============================================================================
//
// STRENGTHENED VERSION:
//  - RAW gForce is used for fast, transient events (impact / gesture),
//    since smoothing would blunt a sudden spike.
//  - SMOOTHED gForce (EMA) is used for sustained motion detection, so
//    normal hand tremor / sensor noise doesn't cause false triggers.
//  - Gesture-arm timing bug fixed: elapsed time since the previous
//    qualifying sample is now captured BEFORE the timestamp is
//    overwritten, so the "close together in time" check is meaningful.
// ============================================================================

void readMPU()
{
  if (!mpuOK)
  {
    return;
  }


  Wire.beginTransmission(
    MPU_ADDR
  );

  Wire.write(
    MPU_ACCEL_XOUT_H
  );

  Wire.endTransmission(false);


  Wire.requestFrom(
    MPU_ADDR,
    6,
    true
  );


  if (Wire.available() < 6)
  {
    return;
  }


  int16_t rawX =
    (Wire.read() << 8) |
    Wire.read();


  int16_t rawY =
    (Wire.read() << 8) |
    Wire.read();


  int16_t rawZ =
    (Wire.read() << 8) |
    Wire.read();


  accelX =
    rawX / 16384.0;


  accelY =
    rawY / 16384.0;


  accelZ =
    rawZ / 16384.0;


  gForce =
    sqrt(
      accelX * accelX +
      accelY * accelY +
      accelZ * accelZ
    );


  // Exponential moving average -> stable signal for sustained motion.
  gForceSmooth =
    gForceSmooth * 0.75 +
    gForce * 0.25;


  unsigned long now =
    millis();


  // ----------------------------------------------------------
  // SYSTEM OFF -> MOTION GESTURE CAN ARM SYSTEM
  // ----------------------------------------------------------
  //
  // Uses RAW gForce (a deliberate shake produces a sharp spike),
  // and correctly measures the gap since the previous qualifying
  // sample so two spikes must land close together in time.
  // ----------------------------------------------------------

  if (state == SYS_OFF)
  {
    if (gForce >= HAND_GESTURE_G)
    {
      // Only sample once every 180ms so a single shake doesn't
      // register as many samples.
      if (now - lastHandGestureSample > 180)
      {
        // Capture elapsed time BEFORE overwriting the timestamp -
        // this is the actual gap since the previous qualifying
        // sample, used to decide if the two spikes are "close
        // together in time".
        unsigned long elapsedSinceLast =
          (lastHandGestureSample == 0)
          ? 0
          : now - lastHandGestureSample;

        lastHandGestureSample = now;


        if (
          handGestureSamples == 0 ||
          elapsedSinceLast <= HAND_GESTURE_WINDOW
        )
        {
          handGestureSamples++;
        }
        else
        {
          // Too much time passed since the last qualifying
          // sample - this is a fresh attempt, start over at 1.
          handGestureSamples = 1;
        }


        Serial.print(
          "[MPU] HAND GESTURE SAMPLE: "
        );

        Serial.print(
          handGestureSamples
        );

        Serial.print(
          " (gap="
        );

        Serial.print(
          elapsedSinceLast
        );

        Serial.println(
          "ms)"
        );


        if (
          handGestureSamples >=
          HAND_GESTURE_REQUIRED
        )
        {
          handGestureSamples = 0;

          Serial.println(
            ">>> MPU GESTURE -> SYSTEM ON"
          );

          turnSystemOn();
        }
      }
    }


    return;
  }


  // ----------------------------------------------------------
  // NORMAL MOTION / IMPACT DETECTION
  // ----------------------------------------------------------
  //
  // Sustained motion uses the SMOOTHED signal (steadier, resists
  // false positives from sensor noise). Impact uses the RAW signal
  // (a real fall/impact is a brief, sharp spike that smoothing
  // would otherwise weaken).
  // ----------------------------------------------------------

  motionDetected = false;

  impactDetected = false;


  bool sustainedMotion =
    gForceSmooth >= MOTION_G;

  bool suddenImpact =
    gForce >= IMPACT_G;


  if (
    (sustainedMotion || suddenImpact) &&
    now - lastMotionTime >= 700
  )
  {
    motionDetected = true;

    lastMotionTime = now;


    if (suddenImpact)
    {
      impactDetected = true;
    }
  }
}


// ============================================================================
// SOUND LEVEL
// ============================================================================
//
// Peak-to-peak burst sampling gives much better sensitivity than a single
// analogRead() for a small voice signal.
// ============================================================================

int measureSoundLevel()
{
  int minVal = 4095;
  int maxVal = 0;

  unsigned long start = micros();

  while (micros() - start < 4000)
  {
    int val = analogRead(SOUND_PIN);

    if (val < minVal)
    {
      minVal = val;
    }

    if (val > maxVal)
    {
      maxVal = val;
    }
  }

  return maxVal - minVal;
}


// ============================================================================
// SOUND PROCESSING
// ============================================================================

void processSound(int raw)
{
  if (
    state != MONITORING
  )
  {
    soundActive = false;

    loudVoiceDetected = false;

    return;
  }


  unsigned long now = millis();


  // ----------------------------------------------------------
  // ADAPTIVE AMBIENT BASELINE
  // ----------------------------------------------------------

  if (!soundActive)
  {
    soundBaseline =
      soundBaseline * 0.97 +
      raw * 0.03;
  }


  soundDifference =
    raw - soundBaseline;


  if (soundDifference < 0)
  {
    soundDifference = 0;
  }


  // ----------------------------------------------------------
  // SMOOTH ENVELOPE
  // ----------------------------------------------------------

  soundEnvelope =
    soundEnvelope * 0.45 +
    soundDifference * 0.55;


  // ----------------------------------------------------------
  // SOFT VOICE DETECTION
  // ----------------------------------------------------------

  bool voiceDetected =
    (
      soundEnvelope >=
      SOUND_SENSITIVITY
    )
    &&
    (
      soundDifference >=
      SOUND_MINIMUM
    );


  // ----------------------------------------------------------
  // HR + VOICE SUPPORT
  // ----------------------------------------------------------

  loudVoiceDetected =
    voiceDetected &&
    averageBPM >=
    DEMO_HR_THRESHOLD;


  // ----------------------------------------------------------
  // EVENT DETECTION
  // ----------------------------------------------------------

  if (voiceDetected)
  {
    if (!soundActive)
    {
      soundActive = true;


      if (
        lastSoundEvent == 0 ||
        now - lastSoundEvent >=
        SOUND_LOCKOUT
      )
      {
        registerSoundEvent();
      }
    }
  }
  else
  {
    soundActive = false;
  }


  // ----------------------------------------------------------
  // DEBUG
  // ----------------------------------------------------------

  static unsigned long lastSoundDebug = 0;


  if (
    now - lastSoundDebug >=
    250
  )
  {
    lastSoundDebug = now;


    Serial.print("[VOICE] raw=");
    Serial.print(raw);

    Serial.print(" base=");
    Serial.print(soundBaseline, 1);

    Serial.print(" diff=");
    Serial.print(soundDifference, 1);

    Serial.print(" env=");
    Serial.print(soundEnvelope, 1);

    Serial.print(" events=");
    Serial.print(soundEvents);

    Serial.println("/3");
  }
}


// ============================================================================
// REGISTER SOUND EVENT
// ============================================================================

void registerSoundEvent()
{
  unsigned long now =
    millis();


  // ----------------------------------------------------------
  // 5 SECOND EVENT WINDOW
  // ----------------------------------------------------------

  if (
    soundEvents == 0 ||
    now - soundWindowStart >=
    SOUND_WINDOW
  )
  {
    soundEvents = 0;

    soundWindowStart = now;
  }


  // ----------------------------------------------------------
  // MAX 3 EVENTS
  // ----------------------------------------------------------

  if (
    soundEvents >=
    SOUND_EVENTS_MAX
  )
  {
    return;
  }


  soundEvents++;

  lastSoundEvent = now;


  Serial.println();

  Serial.print(
    ">>> VOICE EVENT "
  );

  Serial.print(
    soundEvents
  );

  Serial.println(
    "/3"
  );


  if (soundEvents == 1)
  {
    Serial.println(
      ">>> SOFT VOICE DETECTED"
    );
  }
  else if (soundEvents == 2)
  {
    Serial.println(
      ">>> SECOND VOICE EVENT DETECTED"
    );
  }
  else if (soundEvents == 3)
  {
    Serial.println(
      ">>> THIRD VOICE EVENT DETECTED"
    );

    Serial.println(
      ">>> 3/3 REACHED -> FIRING SOS INSTANTLY"
    );


    // --------------------------------------------------------
    // INSTANT TRIGGER — don't wait for the next periodic score
    // scan. 3 soft voice bursts inside the window fire the SOS
    // countdown (and OLED alert screen) immediately.
    // --------------------------------------------------------

    if (state == MONITORING)
    {
      distressScore = 100;

      startCountdown(
        "3x SOFT VOICE EVENTS"
      );
    }
  }
}


// ============================================================================
// DISTRESS SCORE
// ============================================================================

int computeDistressScore()
{
  int score = 0;


  // ----------------------------------------------------------
  // HEART RATE RISE (strongest single trigger)
  // ----------------------------------------------------------
  //
  // A big HR spike alone should be able to win and cross the
  // SOS threshold on its own, same as 3 voice events do.
  // ----------------------------------------------------------

  if (
    baselineCaptured &&
    fingerDetected &&
    averageBPM > 0
  )
  {
    float rise =
      averageBPM -
      bpmBaseline;


    if (rise >= 25)
    {
      score += 60;
    }
    else if (rise >= 15)
    {
      score += 30;
    }
  }


  // ----------------------------------------------------------
  // SOFT VOICE
  // ----------------------------------------------------------
  //
  // 1 event = 20
  // 2 events = 40
  // 3 events = 60 (also fires instantly, see registerSoundEvent)
  // ----------------------------------------------------------

  score +=
    soundEvents * 20;


  // ----------------------------------------------------------
  // LOUD / VOICE + HR
  // ----------------------------------------------------------

  if (
    loudVoiceDetected &&
    averageBPM >=
    DEMO_HR_THRESHOLD
  )
  {
    score += 20;
  }


  // ----------------------------------------------------------
  // MOTION
  // ----------------------------------------------------------

  if (impactDetected)
  {
    score += 30;
  }
  else if (motionDetected)
  {
    score += 15;
  }


  if (score > 100)
  {
    score = 100;
  }


  return score;
}


// ============================================================================
// BUILD TRIGGER REASON
// ============================================================================

String buildTriggerReason()
{
  String parts = "";


  if (
    baselineCaptured &&
    fingerDetected &&
    (
      averageBPM -
      bpmBaseline
    ) >= 25
  )
  {
    parts +=
      "STRONG HR SPIKE";
  }
  else if (
    baselineCaptured &&
    fingerDetected &&
    (
      averageBPM -
      bpmBaseline
    ) >= 15
  )
  {
    parts +=
      "HR SPIKE";
  }


  if (
    loudVoiceDetected &&
    averageBPM >=
    DEMO_HR_THRESHOLD
  )
  {
    if (parts.length() > 0)
    {
      parts += " + ";
    }

    parts +=
      "LOUD SOUND + HR";
  }


  if (
    soundEvents > 0
  )
  {
    if (parts.length() > 0)
    {
      parts += " + ";
    }

    parts +=
      "SOUND";
  }


  if (impactDetected)
  {
    if (parts.length() > 0)
    {
      parts += " + ";
    }

    parts +=
      "IMPACT";
  }
  else if (motionDetected)
  {
    if (parts.length() > 0)
    {
      parts += " + ";
    }

    parts +=
      "MOTION";
  }


  if (parts.length() == 0)
  {
    parts =
      "MULTIMODAL DISTRESS";
  }


  return parts;
}


// ============================================================================
// OLED MESSAGE
// ============================================================================

void showFullScreenMessage(
  String l1,
  String l2
)
{
  messageActive = true;

  messageLine1 = l1;

  messageLine2 = l2;

  messageStart =
    millis();
}


// ============================================================================
// OLED UPDATE
// ============================================================================

void updateOLED()
{
  oled.clearBuffer();


  if (
    state == SYS_OFF &&
    !messageActive
  )
  {
    drawSystemOffScreen();

    oled.sendBuffer();

    return;
  }


  if (messageActive)
  {
    if (
      millis() - messageStart <
      MESSAGE_DURATION
    )
    {
      drawFullScreenMessage();

      oled.sendBuffer();

      return;
    }
    else
    {
      messageActive = false;
    }
  }


  if (
    state == COUNTDOWN
  )
  {
    drawCountdownScreen();
  }
  else if (
    state == SOS_ACTIVE
  )
  {
    drawSOSScreen();
  }
  else if (
    state == MONITORING
  )
  {
    if (oledPage == 0)
    {
      drawStatusPage();
    }
    else if (oledPage == 1)
    {
      drawSensorsPage();
    }
    else if (oledPage == 2)
    {
      drawGPSPage();
    }
    else
    {
      drawMotionPage();
    }
  }
  else
  {
    drawSystemOffScreen();
  }


  oled.sendBuffer();
}


// ============================================================================
// OLED FULL SCREEN
// ============================================================================

void drawFullScreenMessage()
{
  oled.setFont(
    u8g2_font_9x15B_tf
  );


  oled.drawStr(
    2,
    28,
    messageLine1.c_str()
  );


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    2,
    46,
    messageLine2.c_str()
  );
}


// ============================================================================
// OLED OFF
// ============================================================================

void drawSystemOffScreen()
{
  oled.setFont(
    u8g2_font_9x15B_tf
  );


  oled.drawStr(
    10,
    28,
    "SYSTEM OFF"
  );


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    2,
    46,
    "Quick press btn"
  );


  oled.drawStr(
    2,
    58,
    "to arm SheShield"
  );
}


// ============================================================================
// OLED COUNTDOWN
// ============================================================================

void drawCountdownScreen()
{
  oled.setFont(
    u8g2_font_9x15B_tf
  );


  oled.drawStr(
    4,
    20,
    "ALERT!"
  );


  oled.setFont(
    u8g2_font_logisoso24_tf
  );


  char buf[4];


  snprintf(
    buf,
    sizeof(buf),
    "%d",
    countdownSecond
  );


  oled.drawStr(
    52,
    55,
    buf
  );


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    63,
    "Press btn to cancel"
  );
}


// ============================================================================
// OLED SOS
// ============================================================================

void drawSOSScreen()
{
  oled.setFont(
    u8g2_font_9x15B_tf
  );


  oled.drawStr(
    20,
    28,
    "SOS SENT"
  );


  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    44,
    "Score:"
  );


  oled.print(
    distressScore
  );


  oled.drawStr(
    0,
    58,
    "Btn = Reset"
  );
}


// ============================================================================
// OLED STATUS PAGE
// ============================================================================

void drawStatusPage()
{
  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    9,
    "SheShield"
  );


  oled.drawHLine(
    0,
    11,
    128
  );


  oled.setCursor(
    0,
    22
  );

  oled.print(
    "STATE: MONITOR"
  );


  oled.setCursor(
    0,
    34
  );

  oled.print(
    "BPM:"
  );


  oled.print(
    fingerDetected &&
    averageBPM > 0
    ? String(averageBPM)
    : "--"
  );


  oled.setCursor(
    70,
    34
  );

  oled.print(
    "HR:"
  );

  oled.print(
    getHRText()
  );


  oled.setCursor(
    0,
    46
  );

  oled.print(
    "SCORE:"
  );

  oled.print(
    distressScore
  );


  oled.setCursor(
    0,
    59
  );

  oled.print(
    "GPS:"
  );

  oled.print(
    gpsFix
    ? "OK"
    : "..."
  );


  oled.setCursor(
    70,
    59
  );

  oled.print(
    WiFi.status() ==
    WL_CONNECTED
    ? "WiFi OK"
    : "WiFi--"
  );
}


// ============================================================================
// OLED SENSOR PAGE
// ============================================================================

void drawSensorsPage()
{
  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    9,
    "LIVE SENSORS"
  );


  oled.drawHLine(
    0,
    11,
    128
  );


  oled.setCursor(
    0,
    22
  );

  oled.print(
    "MAX:"
  );

  oled.print(
    max30102OK
    ? "OK"
    : "ERR"
  );


  oled.setCursor(
    65,
    22
  );

  oled.print(
    "F:"
  );

  oled.print(
    fingerDetected
    ? "YES"
    : "NO"
  );


  oled.setCursor(
    0,
    34
  );

  oled.print(
    "MPU:"
  );

  oled.print(
    mpuOK
    ? "OK"
    : "ERR"
  );


  oled.setCursor(
    65,
    34
  );

  oled.print(
    "G:"
  );

  oled.print(
    gForce,
    2
  );


  oled.setCursor(
    0,
    46
  );

  oled.print(
    "SND EV:"
  );

  oled.print(
    soundEvents
  );

  oled.print(
    "/3"
  );


  oled.setCursor(
    0,
    59
  );

  oled.print(
    "Telegram:"
  );

  oled.print(
    telegramReady
    ? "OK"
    : "OFF"
  );
}


// ============================================================================
// OLED GPS PAGE
// ============================================================================

void drawGPSPage()
{
  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    9,
    "GPS LOCATION"
  );


  oled.drawHLine(
    0,
    11,
    128
  );


  if (!gpsFix)
  {
    oled.drawStr(
      0,
      24,
      "WAITING FOR FIX"
    );


    oled.setCursor(
      0,
      37
    );


    oled.print(
      "SAT:"
    );


    oled.print(
      satellites
    );


    return;
  }


  oled.setCursor(
    0,
    23
  );


  oled.print(
    "LAT:"
  );


  oled.print(
    latitude,
    5
  );


  oled.setCursor(
    0,
    35
  );


  oled.print(
    "LON:"
  );


  oled.print(
    longitude,
    5
  );


  oled.setCursor(
    0,
    47
  );


  oled.print(
    "SAT:"
  );


  oled.print(
    satellites
  );


  oled.setCursor(
    65,
    47
  );


  oled.print(
    "HD:"
  );


  oled.print(
    hdop,
    1
  );
}


// ============================================================================
// OLED MOTION PAGE
// ============================================================================

void drawMotionPage()
{
  oled.setFont(
    u8g2_font_6x10_tf
  );


  oled.drawStr(
    0,
    9,
    "MPU6050 MOTION"
  );


  oled.drawHLine(
    0,
    11,
    128
  );


  oled.setCursor(
    0,
    23
  );

  oled.print(
    "X:"
  );

  oled.print(
    accelX,
    2
  );


  oled.setCursor(
    65,
    23
  );

  oled.print(
    "Y:"
  );

  oled.print(
    accelY,
    2
  );


  oled.setCursor(
    0,
    35
  );

  oled.print(
    "Z:"
  );

  oled.print(
    accelZ,
    2
  );


  oled.setCursor(
    65,
    35
  );

  oled.print(
    "G:"
  );

  oled.print(
    gForce,
    2
  );


  oled.setCursor(
    0,
    48
  );

  oled.print(
    "MOTION:"
  );

  oled.print(
    motionDetected
    ? "YES"
    : "NO"
  );


  oled.setCursor(
    0,
    60
  );

  oled.print(
    "IMPACT:"
  );

  oled.print(
    impactDetected
    ? "YES"
    : "NO"
  );
}


// ============================================================================
// SERIAL DASHBOARD
// ============================================================================

void printSerialDashboard()
{
  Serial.println();

  Serial.println(
    "---------------- SHESHIELD ----------------"
  );


  Serial.print(
    "STATE      : "
  );

  Serial.println(
    getStateText()
  );


  Serial.print(
    "SCORE      : "
  );

  Serial.print(
    distressScore
  );

  Serial.println(
    "/100"
  );


  Serial.print(
    "BPM        : "
  );

  if (
    averageBPM > 0
  )
  {
    Serial.print(
      averageBPM
    );
  }
  else
  {
    Serial.print(
      "--"
    );
  }


  Serial.print(
    " | HR="
  );


  Serial.println(
    getHRText()
  );


  Serial.print(
    "BASELINE   : "
  );


  if (baselineCaptured)
  {
    Serial.println(
      bpmBaseline
    );
  }
  else
  {
    Serial.println(
      "CALIBRATING"
    );
  }


  Serial.print(
    "SOUND EV   : "
  );

  Serial.print(
    soundEvents
  );

  Serial.println(
    "/3"
  );


  Serial.print(
    "VOICE ENV  : "
  );

  Serial.println(
    soundEnvelope,
    1
  );


  Serial.print(
    "LOUD+HR    : "
  );

  Serial.println(
    loudVoiceDetected
    ? "YES"
    : "NO"
  );


  Serial.print(
    "MOTION     : "
  );

  Serial.print(
    motionDetected
    ? "YES"
    : "NO"
  );


  Serial.print(
    " | IMPACT="
  );

  Serial.println(
    impactDetected
    ? "YES"
    : "NO"
  );


  Serial.print(
    "G-FORCE    : "
  );

  Serial.print(
    gForce,
    2
  );

  Serial.print(
    " (smooth="
  );

  Serial.print(
    gForceSmooth,
    2
  );

  Serial.println(
    ")"
  );


  Serial.print(
    "GPS        : "
  );


  if (gpsFix)
  {
    Serial.print(
      latitude,
      6
    );

    Serial.print(
      ", "
    );

    Serial.println(
      longitude,
      6
    );
  }
  else
  {
    Serial.println(
      "NO FIX"
    );
  }


  Serial.print(
    "SATELLITES : "
  );

  Serial.println(
    satellites
  );


  Serial.print(
    "WiFi       : "
  );

  Serial.println(
    WiFi.status() ==
    WL_CONNECTED
    ? "ONLINE"
    : "OFFLINE"
  );


  Serial.print(
    "Telegram   : "
  );

  Serial.println(
    telegramReady
    ? "READY"
    : "OFFLINE"
  );


  Serial.println(
    "--------------------------------------------"
  );
}


// ============================================================================
// STATE TEXT
// ============================================================================

String getStateText()
{
  if (
    state == SYS_OFF
  )
  {
    return "SYSTEM OFF";
  }


  if (
    state == MONITORING
  )
  {
    return "MONITORING";
  }


  if (
    state == COUNTDOWN
  )
  {
    return "COUNTDOWN";
  }


  return "SOS ACTIVE";
}


// ============================================================================
// HR TEXT
// ============================================================================

String getHRText()
{
  if (
    hrLevel == HR_LOW
  )
  {
    return "LOW";
  }


  if (
    hrLevel == HR_MEDIUM
  )
  {
    return "MEDIUM";
  }


  if (
    hrLevel == HR_HIGH
  )
  {
    return "HIGH";
  }


  return "WAITING";
}


// ============================================================================
// GOOGLE MAPS LINK
// ============================================================================

String getGoogleMapsLink()
{
  return
    "https://maps.google.com/?q=" +
    String(latitude, 8) +
    "," +
    String(longitude, 8);
}


// ============================================================================
// END
// ============================================================================
