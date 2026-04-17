/*
 * Native tests for FLANKTUS logic.
 * Compile & run:  g++ -std=c++11 -o test_flanktus test_flanktus.cpp && ./test_flanktus
 */

#include <cassert>
#include <cstdio>
#include <cmath>
#include "flanktus_pump/flanktus_logic.h"

#define MIN_MS(m) ((m) * 60UL * 1000UL)

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name()
#define RUN(name) do { \
    printf("  %-50s", #name); \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
  } while(0)

// ═══════════════════════════════════════════
// Timing profiles
// ═══════════════════════════════════════════

TEST(test_on_time_below_30) {
  // All temps <= 30 should give 1 min ON
  assert(getOnTime(5.0f)  == MIN_MS(1));
  assert(getOnTime(10.0f) == MIN_MS(1));
  assert(getOnTime(18.0f) == MIN_MS(1));
  assert(getOnTime(25.0f) == MIN_MS(1));
  assert(getOnTime(30.0f) == MIN_MS(1));
}

TEST(test_on_time_above_30) {
  // Above 30 → 2 min ON
  assert(getOnTime(30.1f) == MIN_MS(2));
  assert(getOnTime(35.0f) == MIN_MS(2));
  assert(getOnTime(40.0f) == MIN_MS(2));
}

TEST(test_off_time_very_cold) {
  // <= 10 → 20 min OFF
  assert(getOffTime(5.0f)  == MIN_MS(20));
  assert(getOffTime(0.0f)  == MIN_MS(20));
  assert(getOffTime(-5.0f) == MIN_MS(20));
  assert(getOffTime(10.0f) == MIN_MS(20));
}

TEST(test_off_time_cool) {
  // 10 < t <= 18 → 10 min OFF
  assert(getOffTime(10.1f) == MIN_MS(10));
  assert(getOffTime(15.0f) == MIN_MS(10));
  assert(getOffTime(18.0f) == MIN_MS(10));
}

TEST(test_off_time_warm) {
  // 18 < t <= 25 → 10 min OFF
  assert(getOffTime(18.1f) == MIN_MS(10));
  assert(getOffTime(22.0f) == MIN_MS(10));
  assert(getOffTime(25.0f) == MIN_MS(10));
}

TEST(test_off_time_hot) {
  // 25 < t <= 30 → 6 min OFF
  assert(getOffTime(25.1f) == MIN_MS(6));
  assert(getOffTime(28.0f) == MIN_MS(6));
  assert(getOffTime(30.0f) == MIN_MS(6));
}

TEST(test_off_time_very_hot) {
  // > 30 → 3 min OFF
  assert(getOffTime(30.1f) == MIN_MS(3));
  assert(getOffTime(35.0f) == MIN_MS(3));
}

// ═══════════════════════════════════════════
// Boundary tests (exact thresholds)
// ═══════════════════════════════════════════

TEST(test_boundary_at_exactly_30) {
  // 30.0 is NOT > 30, so ON=1min, OFF=6min
  assert(getOnTime(30.0f)  == MIN_MS(1));
  assert(getOffTime(30.0f) == MIN_MS(6));
}

TEST(test_boundary_at_exactly_25) {
  // 25.0 is NOT > 25, so OFF=10min
  assert(getOffTime(25.0f) == MIN_MS(10));
}

TEST(test_boundary_at_exactly_18) {
  // 18.0 is NOT > 18, so OFF=10min
  assert(getOffTime(18.0f) == MIN_MS(10));
}

TEST(test_boundary_at_exactly_10) {
  // 10.0 is NOT > 10, so OFF=20min
  assert(getOffTime(10.0f) == MIN_MS(20));
}

// ═══════════════════════════════════════════
// EEPROM layout
// ═══════════════════════════════════════════

TEST(test_entry_address_first) {
  assert(entryAddress(0) == 2);
}

TEST(test_entry_address_sequential) {
  assert(entryAddress(1) == 8);
  assert(entryAddress(2) == 14);
  assert(entryAddress(10) == 62);
}

TEST(test_entry_address_last) {
  // Last valid entry = MAX_ENTRIES - 1
  int addr = entryAddress(MAX_ENTRIES - 1);
  // Should fit within 1024 bytes: addr + 6 <= 1024
  assert(addr + ENTRY_SIZE <= 1024);
}

TEST(test_max_entries_value) {
  assert(MAX_ENTRIES == 170);
}

// ═══════════════════════════════════════════
// Ring buffer
// ═══════════════════════════════════════════

TEST(test_ring_index_wraps) {
  assert(ringIndex(0) == 0);
  assert(ringIndex(169) == 169);
  assert(ringIndex(170) == 0);   // wraps
  assert(ringIndex(171) == 1);
  assert(ringIndex(340) == 0);   // wraps again
}

// ═══════════════════════════════════════════
// Pump cycle decision
// ═══════════════════════════════════════════

TEST(test_pump_on_should_not_toggle_early) {
  // Pump ON at 20C, only 30s elapsed — should stay ON (needs 1 min)
  assert(shouldTogglePump(true, 20.0f, 30000) == false);
}

TEST(test_pump_on_should_toggle_at_threshold) {
  // Pump ON at 20C, exactly 1 min elapsed
  assert(shouldTogglePump(true, 20.0f, MIN_MS(1)) == true);
}

TEST(test_pump_off_should_not_toggle_early) {
  // Pump OFF at 20C, 5 min elapsed — needs 10 min
  assert(shouldTogglePump(false, 20.0f, MIN_MS(5)) == false);
}

TEST(test_pump_off_should_toggle_at_threshold) {
  // Pump OFF at 20C, 10 min elapsed
  assert(shouldTogglePump(false, 20.0f, MIN_MS(10)) == true);
}

TEST(test_pump_hot_cycle) {
  // At 35C: ON=2min, OFF=3min
  assert(shouldTogglePump(true, 35.0f, MIN_MS(2)) == true);
  assert(shouldTogglePump(false, 35.0f, MIN_MS(3)) == true);
  // Not yet
  assert(shouldTogglePump(true, 35.0f, MIN_MS(1)) == false);
  assert(shouldTogglePump(false, 35.0f, MIN_MS(2)) == false);
}

TEST(test_pump_cold_cycle) {
  // At 5C: ON=1min, OFF=20min
  assert(shouldTogglePump(true, 5.0f, MIN_MS(1)) == true);
  assert(shouldTogglePump(false, 5.0f, MIN_MS(20)) == true);
  assert(shouldTogglePump(false, 5.0f, MIN_MS(19)) == false);
}

// ═══════════════════════════════════════════
// Runner
// ═══════════════════════════════════════════

int main() {
  printf("FLANKTUS Logic Tests\n");
  printf("====================\n");

  printf("\nTiming profiles:\n");
  RUN(test_on_time_below_30);
  RUN(test_on_time_above_30);
  RUN(test_off_time_very_cold);
  RUN(test_off_time_cool);
  RUN(test_off_time_warm);
  RUN(test_off_time_hot);
  RUN(test_off_time_very_hot);

  printf("\nBoundary tests:\n");
  RUN(test_boundary_at_exactly_30);
  RUN(test_boundary_at_exactly_25);
  RUN(test_boundary_at_exactly_18);
  RUN(test_boundary_at_exactly_10);

  printf("\nEEPROM layout:\n");
  RUN(test_entry_address_first);
  RUN(test_entry_address_sequential);
  RUN(test_entry_address_last);
  RUN(test_max_entries_value);

  printf("\nRing buffer:\n");
  RUN(test_ring_index_wraps);

  printf("\nPump cycle decisions:\n");
  RUN(test_pump_on_should_not_toggle_early);
  RUN(test_pump_on_should_toggle_at_threshold);
  RUN(test_pump_off_should_not_toggle_early);
  RUN(test_pump_off_should_toggle_at_threshold);
  RUN(test_pump_hot_cycle);
  RUN(test_pump_cold_cycle);

  printf("\n====================\n");
  printf("All %d tests passed!\n", tests_passed);
  return 0;
}
