#ifndef FLANKTUS_LOGIC_H
#define FLANKTUS_LOGIC_H

/*
 * Pure logic extracted from flanktus_pump.ino for native testing.
 * No Arduino dependencies — just math and thresholds.
 */

// ── Timing profiles (edit these to tune pump cycling) ──
//
//  Air temp  │  ON          │  OFF
//  > 30°C    │  ON_HOT      │  OFF_HOT
//  25-30°C   │  ON_DEFAULT  │  OFF_WARM
//  1-25°C    │  ON_DEFAULT  │  OFF_DEFAULT
//  ≤ 1°C     │  pump off    │  pump off
//
const unsigned long ON_DEFAULT  = 10UL * 1000UL;  // 10 sec (TEST — prod: 1 min)
const unsigned long ON_HOT      = 20UL * 1000UL;  // 20 sec (TEST — prod: 2 min)
const unsigned long OFF_DEFAULT = 30UL * 1000UL;  // 30 sec (TEST — prod: 10 min)
const unsigned long OFF_WARM    = 20UL * 1000UL;  // 20 sec (TEST — prod: 5 min)
const unsigned long OFF_HOT     = 10UL * 1000UL;  // 10 sec (TEST — prod: 2 min)

const float TEMP_HOT       = 30.0f;  // °C — above this: hot profile
const float TEMP_WARM      = 25.0f;  // °C — above this: warm profile
const float TEMP_MIN_RUN   =  1.0f;  // °C — air at or below this: pump stays off

// Returns ON time in ms based on air temp
inline unsigned long getOnTime(float airC) {
  if (airC > TEMP_HOT) return ON_HOT;
  return ON_DEFAULT;
}

// Returns OFF time in ms based on air temp
inline unsigned long getOffTime(float airC) {
  if (airC > TEMP_HOT)  return OFF_HOT;
  if (airC > TEMP_WARM) return OFF_WARM;
  return OFF_DEFAULT;
}

// Returns true if pump should cycle — stops if air temp ≤1°C
inline bool shouldPumpRun(float airC) {
  return airC > TEMP_MIN_RUN;
}

// ── Pump cycle decision ──
// Returns true if the pump state should flip
inline bool shouldTogglePump(bool pumpOn, float airC, unsigned long elapsed) {
  if (pumpOn) {
    return elapsed >= getOnTime(airC);
  } else {
    return elapsed >= getOffTime(airC);
  }
}

#endif
