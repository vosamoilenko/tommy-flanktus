/*
 * FLANKTUS v6.0 - Hydroponic Auto Pump Controller + SD Logger + LCD Display
 *
 * Wiring:
 *   Pin 2  → DS18B20 #1 water temp DATA  (4.7k pull-up to 5V)
 *   Pin 4  → DS18B20 #2 air temp DATA    (4.7k pull-up to 5V)
 *   Pin 5  → Button (auto mode ON/OFF) → GND (internal pullup)
 *   Pin 7  → Relay IN
 *   Pin 10 → SD card CS
 *   Pin 11 → SD card MOSI  (fixed SPI)
 *   Pin 12 → SD card MISO  (fixed SPI)
 *   Pin 13 → SD card SCK   (fixed SPI)
 *   A4     → LCD SDA       (I2C)
 *   A5     → LCD SCL       (I2C)
 *   5V     → Relay VCC, both DS18B20 VCC, SD VCC, LCD VCC
 *   GND    → Relay GND, both DS18B20 GND, SD GND, LCD GND, Button
 *
 * Auto mode:
 *   Pump cycles ON/OFF automatically. Timing adjusts by air temp:
 *     ≤ 1 C   → pump off (too cold)
 *     1-25 C  → 1 min ON / 10 min OFF
 *     25-30 C → 1 min ON /  5 min OFF
 *     > 30 C  → 2 min ON /  2 min OFF
 *
 * LCD (20x4):
 *   Line 0: Water temp + Air temp
 *   Line 1: Pump state + auto mode
 *   Line 2: Cycle timer (countdown to next toggle)
 *   Line 3: SD card status + uptime
 *
 * SD card:
 *   Logs sensor data every 60 min to FLANKTUS.CSV
 *   Format: minutes,water_c,air_c,pump,auto
 *
 * Serial commands (9600 baud):
 *   d = dump EEPROM log as CSV
 *   c = clear EEPROM log
 *   s = current status
 */

#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "flanktus_logic.h"

// ── Pins ──
const int WATER_TEMP_PIN = 2;
const int AIR_TEMP_PIN   = 4;
const int BTN_TOGGLE     = 5;
const int RELAY_PIN      = 7;
const int SD_CS_PIN      = 10;

// ── LCD (I2C address 0x27, 20 columns, 4 rows) ──
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ── Sensors ──
OneWire owWater(WATER_TEMP_PIN);
OneWire owAir(AIR_TEMP_PIN);
DallasTemperature waterSensor(&owWater);
DallasTemperature airSensor(&owAir);

// ── State ──
bool autoMode = true;
bool pumpOn = false;
unsigned long cycleStart = 0;
unsigned long lastDebounce = 0;

// ── SD card state ──
bool sdReady = false;
const char* SD_FILENAME = "FLANKTUS.CSV";

// ── Cached temps (re-read every 30s, not every loop) ──
float cachedAirTemp = 20.0;
float cachedWaterTemp = 20.0;
unsigned long lastTempRead = 0;
const unsigned long TEMP_READ_INTERVAL = 30UL * 1000UL;

// ── Logging ──
const unsigned long LOG_INTERVAL = 60UL * 60UL * 1000UL;
unsigned long lastLogTime = 0;
bool firstLog = true;
unsigned long lastSDWriteTime = 0;
bool sdWriteFlash = false;

// ── Debug logging (every 5s to serial) ──
const unsigned long DEBUG_INTERVAL = 5UL * 1000UL;
unsigned long lastDebugTime = 0;

// ── LCD update (every 1s) ──
const unsigned long LCD_INTERVAL = 1UL * 1000UL;
unsigned long lastLCDUpdate = 0;

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

void refreshTemps(unsigned long now) {
  if (now - lastTempRead >= TEMP_READ_INTERVAL) {
    lastTempRead = now;
    float wt = readWaterTemp();
    float at = readAirTemp();
    if (wt > -99.0) cachedWaterTemp = wt;
    if (at > -99.0) cachedAirTemp = at;
  }
}

// ── SD card logging ──
void initSD() {
  if (SD.begin(SD_CS_PIN)) {
    sdReady = true;
    // Write header if file doesn't exist
    if (!SD.exists(SD_FILENAME)) {
      File f = SD.open(SD_FILENAME, FILE_WRITE);
      if (f) {
        f.println(F("minutes,water_c,air_c,pump,auto"));
        f.close();
      }
    }
  } else {
    sdReady = false;
  }
}

void logToSD(float waterC, float airC) {
  if (!sdReady) return;
  File f = SD.open(SD_FILENAME, FILE_WRITE);
  if (f) {
    uint16_t mins = (uint16_t)(millis() / 60000UL);
    f.print(mins); f.print(',');
    f.print(waterC, 1); f.print(',');
    f.print(airC, 1); f.print(',');
    f.print(pumpOn ? 1 : 0); f.print(',');
    f.println(autoMode ? 1 : 0);
    f.close();
    lastSDWriteTime = millis();
    sdWriteFlash = true;
  }
}

// ── EEPROM + SD logging combined ──
void logSensorData() {
  float w = readWaterTemp();
  float a = readAirTemp();

  // Skip logging if both sensors are disconnected
  if (w < -99.0 && a < -99.0) return;

  // EEPROM log
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

  // SD card log
  logToSD(w, a);
}

// ── EEPROM dump ──
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
  Serial.println(F("=== FLANKTUS v6.0 ==="));
  Serial.print(F("Auto:  ")); Serial.println(autoMode ? F("ON (cycling)") : F("OFF"));
  Serial.print(F("Pump:  ")); Serial.println(pumpOn ? F("ON") : F("OFF"));
  Serial.print(F("Water: ")); Serial.print(wt, 1); Serial.println(F(" C"));
  Serial.print(F("Air:   ")); Serial.print(at, 1); Serial.println(F(" C"));
  if (autoMode) {
    if (!shouldPumpRun(at)) {
      Serial.println(F("Paused: too cold (<=1 C)"));
    } else {
      Serial.print(F("ON t:  ")); Serial.print(getOnTime(at) / 60000UL); Serial.println(F(" min"));
      Serial.print(F("OFF t: ")); Serial.print(getOffTime(at) / 60000UL); Serial.println(F(" min"));
    }
  }
  Serial.print(F("Up:    ")); Serial.print(millis() / 60000UL); Serial.println(F(" min"));
  Serial.print(F("Log:   ")); Serial.print(getEntryCount());
  Serial.print('/'); Serial.println(MAX_ENTRIES);
  Serial.print(F("SD:    ")); Serial.println(sdReady ? F("OK") : F("NOT FOUND"));
  Serial.println(F("====================="));
}

// ── LCD display ──
// Helper: format mm:ss from milliseconds remaining
void formatTime(char* buf, unsigned long ms) {
  unsigned long totalSec = ms / 1000UL;
  unsigned long m = totalSec / 60;
  unsigned long s = totalSec % 60;
  sprintf(buf, "%02lu:%02lu", m, s);
}

void updateLCD(unsigned long now) {
  char line[21];  // 20 chars + null

  // ── Line 0: Temperatures ──
  // "W:23.5C     A:28.1C"
  lcd.setCursor(0, 0);
  if (cachedWaterTemp < -99.0) {
    sprintf(line, "W:--.-C     A:");
  } else {
    int wWhole = (int)cachedWaterTemp;
    int wFrac = abs((int)(cachedWaterTemp * 10) % 10);
    sprintf(line, "W:%d.%dC", wWhole, wFrac);
  }
  lcd.print(line);

  // Pad to column 12 for air temp
  int len = strlen(line);
  for (int i = len; i < 12; i++) lcd.print(' ');

  if (cachedAirTemp < -99.0) {
    lcd.print(F("A:--.-C "));
  } else {
    int aWhole = (int)cachedAirTemp;
    int aFrac = abs((int)(cachedAirTemp * 10) % 10);
    sprintf(line, "A:%d.%dC", aWhole, aFrac);
    lcd.print(line);
    // Pad to end
    len = strlen(line);
    for (int i = len; i < 8; i++) lcd.print(' ');
  }

  // ── Line 1: Pump state + mode ──
  lcd.setCursor(0, 1);
  if (!autoMode) {
    lcd.print(F("PUMP:OFF  AUTO:OFF   "));
  } else if (!shouldPumpRun(cachedAirTemp)) {
    lcd.print(F("PUMP:OFF  COLD PAUSE "));
  } else if (pumpOn) {
    lcd.print(F("PUMP:ON   AUTO:ON    "));
  } else {
    lcd.print(F("PUMP:OFF  AUTO:ON    "));
  }

  // ── Line 2: Cycle timer ──
  lcd.setCursor(0, 2);
  if (!autoMode) {
    lcd.print(F("Manual mode         "));
  } else if (!shouldPumpRun(cachedAirTemp)) {
    lcd.print(F("Too cold - waiting  "));
  } else {
    unsigned long elapsed = now - cycleStart;
    unsigned long target;
    if (pumpOn) {
      target = getOnTime(cachedAirTemp);
    } else {
      target = getOffTime(cachedAirTemp);
    }
    unsigned long remain = 0;
    if (elapsed < target) remain = target - elapsed;
    char timeBuf[6];
    formatTime(timeBuf, remain);

    // "ON  ends 00:45      " or "OFF next 09:32      "
    if (pumpOn) {
      sprintf(line, "ON  ends %s", timeBuf);
    } else {
      sprintf(line, "OFF next %s", timeBuf);
    }
    lcd.print(line);
    len = strlen(line);
    for (int i = len; i < 20; i++) lcd.print(' ');
  }

  // ── Line 3: SD status + uptime ──
  lcd.setCursor(0, 3);
  unsigned long upMin = now / 60000UL;
  unsigned long upHr = upMin / 60;
  unsigned long upM = upMin % 60;

  // Show "SD:OK" or "SD:--" and flash "SD:WR" for 2 seconds after write
  if (sdWriteFlash && (now - lastSDWriteTime < 2000)) {
    sprintf(line, "SD:WR  Up:%luh%02lum", upHr, upM);
  } else {
    sdWriteFlash = false;
    if (sdReady) {
      sprintf(line, "SD:OK  Up:%luh%02lum", upHr, upM);
    } else {
      sprintf(line, "SD:--  Up:%luh%02lum", upHr, upM);
    }
  }
  lcd.print(line);
  len = strlen(line);
  for (int i = len; i < 20; i++) lcd.print(' ');
}

void setup() {
  Serial.begin(9600);
  digitalWrite(RELAY_PIN, HIGH);   // deactivate relay BEFORE setting as output (active-LOW)
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BTN_TOGGLE, INPUT_PULLUP);

  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("FLANKTUS v6.0"));
  lcd.setCursor(0, 1);
  lcd.print(F("Booting..."));

  // Init sensors
  waterSensor.begin();
  airSensor.begin();
  cachedAirTemp = readAirTemp();
  cachedWaterTemp = readWaterTemp();
  if (cachedAirTemp < -99.0) cachedAirTemp = 20.0;
  if (cachedWaterTemp < -99.0) cachedWaterTemp = 20.0;

  // Init SD card
  lcd.setCursor(0, 2);
  lcd.print(F("SD card..."));
  initSD();
  lcd.setCursor(0, 2);
  if (sdReady) {
    lcd.print(F("SD card: OK       "));
  } else {
    lcd.print(F("SD card: NOT FOUND"));
  }
  delay(1000);

  // Start cycling immediately on boot
  cycleStart = millis();
  if (shouldPumpRun(cachedAirTemp)) {
    setPump(true);
  } else {
    setPump(false);
  }

  Serial.println(F("FLANKTUS v6.0 | d=dump c=clear s=status"));
  Serial.println(F("Auto mode ON — cycling started."));
  if (sdReady) {
    Serial.println(F("SD card: OK"));
  } else {
    Serial.println(F("SD card: NOT FOUND — logging to EEPROM only"));
  }
}

void loop() {
  unsigned long now = millis();

  // ── Refresh cached temps every 30s ──
  refreshTemps(now);

  // ── Button: toggle auto mode ──
  if (digitalRead(BTN_TOGGLE) == LOW && (now - lastDebounce) > 500) {
    lastDebounce = now;
    autoMode = !autoMode;

    if (autoMode) {
      pumpOn = true;
      setPump(true);
      cycleStart = now;
    } else {
      setPump(false);
    }
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

  // ── Log every 60 min ──
  if (firstLog || (now - lastLogTime >= LOG_INTERVAL)) {
    lastLogTime = now;
    firstLog = false;
    logSensorData();
  }

  // ── Update LCD every 1s ──
  if (now - lastLCDUpdate >= LCD_INTERVAL) {
    lastLCDUpdate = now;
    updateLCD(now);
  }

  // ── Debug: print sensor data every 5s ──
  if (now - lastDebugTime >= DEBUG_INTERVAL) {
    lastDebugTime = now;
    unsigned long upMin = now / 60000UL;
    unsigned long upSec = (now / 1000UL) % 60;
    Serial.print(F("[DBG "));
    Serial.print(upMin); Serial.print(':');
    if (upSec < 10) Serial.print('0');
    Serial.print(upSec);
    Serial.print(F("] water="));
    Serial.print(cachedWaterTemp, 1);
    Serial.print(F("C air="));
    Serial.print(cachedAirTemp, 1);
    Serial.print(F("C pump="));
    Serial.print(pumpOn ? F("ON") : F("OFF"));
    Serial.print(F(" auto="));
    Serial.print(autoMode ? F("ON") : F("OFF"));
    Serial.print(F(" sd="));
    Serial.print(sdReady ? F("OK") : F("NO"));
    if (autoMode && shouldPumpRun(cachedAirTemp)) {
      unsigned long elapsed = now - cycleStart;
      unsigned long target = pumpOn ? getOnTime(cachedAirTemp) : getOffTime(cachedAirTemp);
      unsigned long remain = 0;
      if (elapsed < target) remain = (target - elapsed) / 1000UL;
      Serial.print(F(" cycle_left="));
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
