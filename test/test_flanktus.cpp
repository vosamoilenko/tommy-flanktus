/*
 * Native tests for FLANKTUS v6.0 logic.
 * Compile & run:  g++ -std=c++11 -o test_flanktus test_flanktus.cpp && ./test_flanktus
 */

#include <cassert>
#include <cstdio>
#include "flanktus_pump/flanktus_logic.h"

#define MIN_MS(m) ((m) * 60UL * 1000UL)
#define SEC_MS(s) ((s) * 1000UL)

static int tests_passed = 0;

#define TEST(name) static void name()
#define RUN(name) do { \
    printf("  %-50s", #name); \
    name(); \
    tests_passed++; \
    printf("PASS\n"); \
  } while(0)

// ═══════════════════════════════════════════
// Production timing profiles
// ═══════════════════════════════════════════

TEST(test_on_time_below_30) {
  assert(getOnTime(5.0f, false)  == MIN_MS(1));
  assert(getOnTime(10.0f, false) == MIN_MS(1));
  assert(getOnTime(25.0f, false) == MIN_MS(1));
  assert(getOnTime(30.0f, false) == MIN_MS(1));
}

TEST(test_on_time_above_30) {
  assert(getOnTime(30.1f, false) == MIN_MS(2));
  assert(getOnTime(32.0f, false) == MIN_MS(2));
  assert(getOnTime(35.0f, false) == MIN_MS(2));
}

TEST(test_off_time_default) {
  assert(getOffTime(10.1f, false) == MIN_MS(10));
  assert(getOffTime(15.0f, false) == MIN_MS(10));
  assert(getOffTime(25.0f, false) == MIN_MS(10));
}

TEST(test_off_time_warm) {
  assert(getOffTime(25.1f, false) == MIN_MS(5));
  assert(getOffTime(28.0f, false) == MIN_MS(5));
  assert(getOffTime(30.0f, false) == MIN_MS(5));
}

TEST(test_off_time_hot) {
  assert(getOffTime(30.1f, false) == MIN_MS(2));
  assert(getOffTime(32.0f, false) == MIN_MS(2));
  assert(getOffTime(35.0f, false) == MIN_MS(2));
}

// ═══════════════════════════════════════════
// Test mode timing profiles
// ═══════════════════════════════════════════

TEST(test_mode_on_time) {
  assert(getOnTime(20.0f, true) == SEC_MS(10));
  assert(getOnTime(35.0f, true) == SEC_MS(20));
}

TEST(test_mode_off_time) {
  assert(getOffTime(20.0f, true) == SEC_MS(30));
  assert(getOffTime(28.0f, true) == SEC_MS(20));
  assert(getOffTime(35.0f, true) == SEC_MS(10));
}

// ═══════════════════════════════════════════
// Boundary tests
// ═══════════════════════════════════════════

TEST(test_boundary_at_exactly_30) {
  assert(getOnTime(30.0f, false)  == MIN_MS(1));
  assert(getOffTime(30.0f, false) == MIN_MS(5));
}

TEST(test_boundary_at_exactly_25) {
  assert(getOffTime(25.0f, false) == MIN_MS(10));
}

TEST(test_boundary_at_exactly_1) {
  assert(shouldPumpRun(1.0f) == false);
  assert(shouldPumpRun(0.0f) == false);
}

// ═══════════════════════════════════════════
// Pump cycle decision
// ═══════════════════════════════════════════

TEST(test_pump_on_should_not_toggle_early) {
  assert(shouldTogglePump(true, 20.0f, 30000, false) == false);
}

TEST(test_pump_on_should_toggle_at_threshold) {
  assert(shouldTogglePump(true, 20.0f, MIN_MS(1), false) == true);
}

TEST(test_pump_off_should_not_toggle_early) {
  assert(shouldTogglePump(false, 20.0f, MIN_MS(5), false) == false);
}

TEST(test_pump_off_should_toggle_at_threshold) {
  assert(shouldTogglePump(false, 20.0f, MIN_MS(10), false) == true);
}

TEST(test_pump_hot_cycle) {
  // 30-32°C: still cycles with hot profile
  assert(shouldTogglePump(true, 31.0f, MIN_MS(2), false) == true);
  assert(shouldTogglePump(false, 31.0f, MIN_MS(2), false) == true);
  assert(shouldTogglePump(true, 31.0f, MIN_MS(1), false) == false);
  assert(shouldTogglePump(false, 31.0f, MIN_MS(1), false) == false);
}

TEST(test_pump_extreme_always_on) {
  // > 32°C: pump always on — never toggles off, immediately toggles on
  assert(shouldPumpAlwaysOn(32.1f) == true);
  assert(shouldPumpAlwaysOn(40.0f) == true);
  assert(shouldPumpAlwaysOn(32.0f) == false);
  assert(shouldPumpAlwaysOn(30.0f) == false);

  // If pump is on, never toggle off regardless of elapsed time
  assert(shouldTogglePump(true, 35.0f, 0, false) == false);
  assert(shouldTogglePump(true, 35.0f, MIN_MS(60), false) == false);

  // If pump is off, toggle on immediately
  assert(shouldTogglePump(false, 35.0f, 0, false) == true);
  assert(shouldTogglePump(false, 35.0f, MIN_MS(60), false) == true);
}

TEST(test_pump_should_not_run_freezing) {
  assert(shouldPumpRun(1.0f) == false);
  assert(shouldPumpRun(0.0f) == false);
  assert(shouldPumpRun(-10.0f) == false);
  assert(shouldPumpRun(1.1f) == true);
  assert(shouldPumpRun(5.0f) == true);
}

TEST(test_pump_toggle_test_mode) {
  // In test mode at 20C: ON=10s, OFF=30s
  assert(shouldTogglePump(true, 20.0f, SEC_MS(10), true) == true);
  assert(shouldTogglePump(true, 20.0f, SEC_MS(5), true) == false);
  assert(shouldTogglePump(false, 20.0f, SEC_MS(30), true) == true);
  assert(shouldTogglePump(false, 20.0f, SEC_MS(15), true) == false);
}

// ═══════════════════════════════════════════
// Runner
// ═══════════════════════════════════════════

int main() {
  printf("FLANKTUS v6.0 Logic Tests\n");
  printf("=========================\n");

  printf("\nProduction timings:\n");
  RUN(test_on_time_below_30);
  RUN(test_on_time_above_30);
  RUN(test_off_time_default);
  RUN(test_off_time_warm);
  RUN(test_off_time_hot);

  printf("\nTest mode timings:\n");
  RUN(test_mode_on_time);
  RUN(test_mode_off_time);

  printf("\nBoundary tests:\n");
  RUN(test_boundary_at_exactly_30);
  RUN(test_boundary_at_exactly_25);
  RUN(test_boundary_at_exactly_1);

  printf("\nPump cycle decisions:\n");
  RUN(test_pump_on_should_not_toggle_early);
  RUN(test_pump_on_should_toggle_at_threshold);
  RUN(test_pump_off_should_not_toggle_early);
  RUN(test_pump_off_should_toggle_at_threshold);
  RUN(test_pump_hot_cycle);
  RUN(test_pump_extreme_always_on);
  RUN(test_pump_should_not_run_freezing);
  RUN(test_pump_toggle_test_mode);

  printf("\n=========================\n");
  printf("All %d tests passed!\n", tests_passed);
  return 0;
}
