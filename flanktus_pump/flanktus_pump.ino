/*
 * FLANKTUS v5.0 - Hydroponic Auto Pump Controller + Sensor Logger
 *
 * Wiring:
 *   Pin 2  → DS18B20 #1 water temp DATA  (4.7k pull-up to 5V)
 *   Pin 4  → DS18B20 #2 air temp DATA    (4.7k pull-up to 5V)
 *   Pin 5  → Button (auto mode ON/OFF) → GND (internal pullup)
 *   Pin 7  → Relay IN
 *   5V     → Relay VCC, both DS18B20 VCC
 *   GND    → Relay GND, both DS18B20 GND, Button
 *
 * Auto mode:
 *   Pump cycles ON/OFF automatically. Timing adjusts by air temp:
 *     < 10 C  → pump off (too cold)
 *     10-25 C → 1 min ON / 10 min OFF
 *     25-30 C → 1 min ON /  5 min OFF
 *     > 30 C  → 2 min ON /  2 min OFF
 *
 * LED:
 *   Solid ON  = auto mode active (pump cycling)
 *   OFF       = auto mode disabled (pump off)
 *   2 blinks  = sensor log written
 *
 * Logging:
 *   Sensors read every 60 min, stored in EEPROM (170 entries, ~7 days).
 *   Serial commands (9600 baud):
 *     d = dump log as CSV
 *     c = clear log
 *     s = current status
 */

#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "flanktus_logic.h"

// ── Pins ──
const int WATER_TEMP_PIN = 2;
const int AIR_TEMP_PIN   = 4;
const int BTN_TOGGLE     = 5;
const int RELAY_PIN      = 7;

// ── Sensors ──
OneWire owWater(WATER_TEMP_PIN);
OneWire owAir(AIR_TEMP_PIN);
DallasTemperature waterSensor(&owWater);
DallasTemperature airSensor(&owAir);

// ── State ──
bool autoMode = false;
bool pumpOn = false;
unsigned long cycleStart = 0;
unsigned long lastDebounce = 0;

// ── Cached air temp (re-read every 30s, not every loop) ──
float cachedAirTemp = 20.0;
unsigned long lastTempRead = 0;
const unsigned long TEMP_READ_INTERVAL = 30UL * 1000UL;

// ── Logging ──
const unsigned long LOG_INTERVAL = 60UL * 60UL * 1000UL;
unsigned long lastLogTime = 0;
bool firstLog = true;

// ── Debug logging (every 5s to serial) ──
const unsigned long DEBUG_INTERVAL = 5UL * 1000UL;
unsigned long lastDebugTime = 0;

// ── Helpers ──
uint16_t getEntryCount() {
  uint16_t c;
  EEPROM.get(0, c);
  return (c > MAX_ENTRIES) ? 0 : c;
}

void setEntryCount(uint16_t c) {
  EEPROM.put(0, c);
}

void setPump(bool on) {
  pumpOn = on;
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);  // active-LOW relay
}

void updateLED() {
  digitalWrite(LED_BUILTIN, autoMode ? HIGH : LOW);
}

float readWaterTemp() {
  waterSensor.requestTemperatures();
  float t = waterSensor.getTempCByIndex(0);
  return (t == DEVICE_DISCONNECTED_C) ? -99.9 : t;
}

float readAirTemp() {
  airSensor.requestTemperatures();
  float t = airSensor.getTempCByIndex(0);
  return (t == DEVICE_DISCONNECTED_C) ? -99.9 : t;
}

void refreshAirTemp(unsigned long now) {
  if (now - lastTempRead >= TEMP_READ_INTERVAL) {
    lastTempRead = now;
    float t = readAirTemp();
    if (t > -99.0) cachedAirTemp = t;
  }
}

void logSensorData() {
  float w = readWaterTemp();
  float a = readAirTemp();

  // Skip logging if both sensors are disconnected
  if (w < -99.0 && a < -99.0) return;

  uint16_t count = getEntryCount();
  int idx = ringIndex(count);

  uint16_t mins = (uint16_t)(millis() / 60000UL);
  int16_t wt = (int16_t)(w * 10);
  int16_t at = (int16_t)(a * 10);

  int addr = entryAddress(idx);
  EEPROM.put(addr, mins);
  EEPROM.put(addr + 2, wt);
  EEPROM.put(addr + 4, at);

  if (count < MAX_ENTRIES) setEntryCount(count + 1);

  // Blink LED twice to confirm log
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_BUILTIN, HIGH); delay(100);
    digitalWrite(LED_BUILTIN, LOW);  delay(100);
  }
  updateLED();
}

void dumpLog() {
  uint16_t count = getEntryCount();
  Serial.println(F("minutes,water_c,air_c"));
  for (uint16_t i = 0; i < count; i++) {
    int addr = entryAddress(i);
    uint16_t mins; int16_t wt, at;
    EEPROM.get(addr, mins);
    EEPROM.get(addr + 2, wt);
    EEPROM.get(addr + 4, at);
    Serial.print(mins); Serial.print(',');
    Serial.print(wt / 10); Serial.print('.'); Serial.print(abs(wt) % 10); Serial.print(',');
    Serial.print(at / 10); Serial.print('.'); Serial.println(abs(at) % 10);
  }
  Serial.print(F("--- ")); Serial.print(count);
  Serial.print('/'); Serial.print(MAX_ENTRIES);
  Serial.println(F(" entries ---"));
}

void printStatus() {
  float wt = readWaterTemp();
  float at = readAirTemp();
  Serial.println(F("=== FLANKTUS v5.0 ==="));
  Serial.print(F("Auto:  ")); Serial.println(autoMode ? F("ON (cycling)") : F("OFF"));
  Serial.print(F("Pump:  ")); Serial.println(pumpOn ? F("ON") : F("OFF"));
  Serial.print(F("Water: ")); Serial.print(wt, 1); Serial.println(F(" C"));
  Serial.print(F("Air:   ")); Serial.print(at, 1); Serial.println(F(" C"));
  if (autoMode) {
    if (!shouldPumpRun(at)) {
      Serial.println(F("Paused: too cold (<10 C)"));
    } else {
      Serial.print(F("ON t:  ")); Serial.print(getOnTime(at) / 60000UL); Serial.println(F(" min"));
      Serial.print(F("OFF t: ")); Serial.print(getOffTime(at) / 60000UL); Serial.println(F(" min"));
    }
  }
  Serial.print(F("Up:    ")); Serial.print(millis() / 60000UL); Serial.println(F(" min"));
  Serial.print(F("Log:   ")); Serial.print(getEntryCount());
  Serial.print('/'); Serial.println(MAX_ENTRIES);
  Serial.println(F("====================="));
}

void setup() {
  Serial.begin(9600);
  digitalWrite(RELAY_PIN, HIGH);   // deactivate relay BEFORE setting as output (active-LOW)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BTN_TOGGLE, INPUT_PULLUP);
  waterSensor.begin();
  airSensor.begin();
  setPump(false);
  updateLED();
  cachedAirTemp = readAirTemp();
  if (cachedAirTemp < -99.0) cachedAirTemp = 20.0;
  Serial.println(F("FLANKTUS v5.0 | d=dump c=clear s=status"));
}

void loop() {
  unsigned long now = millis();

  // ── Refresh cached air temp every 30s ──
  refreshAirTemp(now);

  // ── Button: toggle auto mode ──
  if (digitalRead(BTN_TOGGLE) == LOW && (now - lastDebounce) > 500) {
    lastDebounce = now;
    autoMode = !autoMode;

    if (autoMode) {
      pumpOn = true;
      setPump(true);
      cycleStart = now;
      // 3 fast blinks = auto ON
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH); delay(50);
        digitalWrite(LED_BUILTIN, LOW);  delay(50);
      }
    } else {
      setPump(false);
      // 1 long blink = auto OFF
      digitalWrite(LED_BUILTIN, HIGH); delay(300);
      digitalWrite(LED_BUILTIN, LOW);  delay(100);
    }
    updateLED();
  }

  // ── Auto pump cycling ──
  if (autoMode) {
    if (!shouldPumpRun(cachedAirTemp)) {
      if (pumpOn) setPump(false);
    } else {
      unsigned long elapsed = now - cycleStart;
      if (shouldTogglePump(pumpOn, cachedAirTemp, elapsed)) {
        setPump(!pumpOn);
        cycleStart = now;
      }
    }
  }

  // ── Log every 30 min ──
  if (firstLog || (now - lastLogTime >= LOG_INTERVAL)) {
    lastLogTime = now;
    firstLog = false;
    logSensorData();
  }

  // ── Debug: print sensor data every 5s ──
  if (now - lastDebugTime >= DEBUG_INTERVAL) {
    lastDebugTime = now;
    float wt = readWaterTemp();
    float at = readAirTemp();
    unsigned long upMin = now / 60000UL;
    unsigned long upSec = (now / 1000UL) % 60;
    Serial.print(F("[DBG "));
    Serial.print(upMin); Serial.print(':');
    if (upSec < 10) Serial.print('0');
    Serial.print(upSec);
    Serial.print(F("] water="));
    Serial.print(wt, 1);
    Serial.print(F("C air="));
    Serial.print(at, 1);
    Serial.print(F("C pump="));
    Serial.print(pumpOn ? F("ON") : F("OFF"));
    Serial.print(F(" auto="));
    Serial.print(autoMode ? F("ON") : F("OFF"));
    if (autoMode && pumpOn) {
      Serial.print(F(" cycle_left="));
      unsigned long remain = 0;
      unsigned long target = getOnTime(cachedAirTemp);
      unsigned long elapsed = now - cycleStart;
      if (elapsed < target) remain = (target - elapsed) / 1000UL;
      Serial.print(remain); Serial.print(F("s"));
    } else if (autoMode && !pumpOn) {
      Serial.print(F(" cycle_left="));
      unsigned long remain = 0;
      unsigned long target = getOffTime(cachedAirTemp);
      unsigned long elapsed = now - cycleStart;
      if (elapsed < target) remain = (target - elapsed) / 1000UL;
      Serial.print(remain); Serial.print(F("s"));
    }
    Serial.println();
  }

  // ── Serial commands ──
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'd' || cmd == 'D') dumpLog();
    else if (cmd == 'c' || cmd == 'C') {
      for (int i = 0; i < (int)EEPROM.length(); i++) EEPROM.update(i, 0);
      Serial.println(F("EEPROM wiped."));
    }
    else if (cmd == 's' || cmd == 'S') printStatus();
  }
}
