#ifndef FLANKTUS_LOGIC_H
#define FLANKTUS_LOGIC_H

/*
 * Pure logic extracted from flanktus_pump.ino for native testing.
 * No Arduino dependencies — just math and thresholds.
 */

// ── Timing profiles (edit these to tune pump cycling) ──
//  Based on WATER temperature:
//
//  Water temp │  ON          │  OFF
//  > 32°C     │  always on   │  —
//  > 30°C     │  ON_HOT      │  OFF_HOT
//  25-30°C    │  ON_DEFAULT  │  OFF_WARM
//  1-25°C     │  ON_DEFAULT  │  OFF_DEFAULT
//  ≤ 1°C      │  pump off    │  pump off
//

// ── Production timings ──
const unsigned long ON_DEFAULT  =  1UL * 60UL * 1000UL;  //  1 min
const unsigned long ON_HOT      =  2UL * 60UL * 1000UL;  //  2 min
const unsigned long OFF_DEFAULT = 10UL * 60UL * 1000UL;  // 10 min
const unsigned long OFF_WARM    =  5UL * 60UL * 1000UL;  //  5 min
const unsigned long OFF_HOT     =  2UL * 60UL * 1000UL;  //  2 min

// ── Test timings (10x button tap to activate) ──
const unsigned long TEST_ON_DEFAULT  = 10UL * 1000UL;  // 10 sec
const unsigned long TEST_ON_HOT      = 20UL * 1000UL;  // 20 sec
const unsigned long TEST_OFF_DEFAULT = 30UL * 1000UL;  // 30 sec
const unsigned long TEST_OFF_WARM    = 20UL * 1000UL;  // 20 sec
const unsigned long TEST_OFF_HOT     = 10UL * 1000UL;  // 10 sec

const float TEMP_EXTREME   = 32.0f;  // °C — above this: pump always on
const float TEMP_HOT       = 30.0f;  // °C — above this: hot profile
const float TEMP_WARM      = 25.0f;  // °C — above this: warm profile
const float TEMP_MIN_RUN   =  1.0f;  // °C — air at or below this: pump stays off

// Returns ON time in ms based on air temp and test mode
inline unsigned long getOnTime(float airC, bool testMode) {
  if (testMode) {
    return (airC > TEMP_HOT) ? TEST_ON_HOT : TEST_ON_DEFAULT;
  }
  return (airC > TEMP_HOT) ? ON_HOT : ON_DEFAULT;
}

// Returns OFF time in ms based on air temp and test mode
inline unsigned long getOffTime(float airC, bool testMode) {
  if (testMode) {
    if (airC > TEMP_HOT)  return TEST_OFF_HOT;
    if (airC > TEMP_WARM) return TEST_OFF_WARM;
    return TEST_OFF_DEFAULT;
  }
  if (airC > TEMP_HOT)  return OFF_HOT;
  if (airC > TEMP_WARM) return OFF_WARM;
  return OFF_DEFAULT;
}

// Returns true if pump should cycle — stops if air temp ≤1°C
inline bool shouldPumpRun(float airC) {
  return airC > TEMP_MIN_RUN;
}

// Returns true if pump should stay on continuously (extreme heat)
inline bool shouldPumpAlwaysOn(float airC) {
  return airC > TEMP_EXTREME;
}

// ── Pump cycle decision ──
// Returns true if the pump state should flip
inline bool shouldTogglePump(bool pumpOn, float airC, unsigned long elapsed, bool testMode) {
  if (shouldPumpAlwaysOn(airC)) {
    return !pumpOn;  // if off, toggle on immediately; if on, never toggle off
  }
  if (pumpOn) {
    return elapsed >= getOnTime(airC, testMode);
  } else {
    return elapsed >= getOffTime(airC, testMode);
  }
}

#endif
