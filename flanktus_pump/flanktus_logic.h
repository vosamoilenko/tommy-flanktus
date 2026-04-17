#ifndef FLANKTUS_LOGIC_H
#define FLANKTUS_LOGIC_H

/*
 * Pure logic extracted from flanktus_pump.ino for native testing.
 * No Arduino dependencies — just math and thresholds.
 */

// ── Constants ──
const int ENTRY_SIZE   = 6;
const int MAX_ENTRIES  = (1024 - 2) / ENTRY_SIZE;  // 170

// ── Timing profiles ──

// Returns ON time in ms based on air temp
inline unsigned long getOnTime(float airC) {
  if (airC > 30.0f) return 2UL * 60UL * 1000UL;  // 2 min
  return 1UL * 60UL * 1000UL;                      // 1 min
}

// Returns OFF time in ms based on air temp
inline unsigned long getOffTime(float airC) {
  if (airC > 30.0f) return  2UL * 60UL * 1000UL;  // 2 min
  if (airC > 25.0f) return  5UL * 60UL * 1000UL;  // 5 min
  return 10UL * 60UL * 1000UL;                     // 10 min
}

// Returns true if pump should cycle at this temperature
inline bool shouldPumpRun(float airC) {
  return airC > 10.0f;
}

// ── EEPROM address calc ──
inline int entryAddress(int idx) {
  return 2 + idx * ENTRY_SIZE;
}

// ── Ring buffer index ──
inline int ringIndex(int count) {
  return count % MAX_ENTRIES;
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
