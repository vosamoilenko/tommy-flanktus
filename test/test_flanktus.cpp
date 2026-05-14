/*
 * Native tests for FLANKTUS v6.0 logic.
 * Compile & run:  g++ -std=c++11 -o test_flanktus test_flanktus.cpp && ./test_flanktus
 */

#include <cassert>
#include <cstdio>
#include <cmath>
#include "flanktus_pump/flanktus_logic.h"

#define MIN_MS(m) ((m) * 60UL * 1000UL)

static int tests_passed = 0;

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
  assert(getOnTime(5.0f)  == MIN_MS(1));
  assert(getOnTime(10.0f) == MIN_MS(1));
  assert(getOnTime(18.0f) == MIN_MS(1));
  assert(getOnTime(25.0f) == MIN_MS(1));
  assert(getOnTime(30.0f) == MIN_MS(1));
}

TEST(test_on_time_above_30) {
  assert(getOnTime(30.1f) == MIN_MS(2));
  assert(getOnTime(35.0f) == MIN_MS(2));
  assert(getOnTime(40.0f) == MIN_MS(2));
}

TEST(test_off_time_cold) {
  assert(getOffTime(5.0f)  == MIN_MS(10));
  assert(getOffTime(0.0f)  == MIN_MS(10));
  assert(getOffTime(-5.0f) == MIN_MS(10));
  assert(getOffTime(10.0f) == MIN_MS(10));
}

TEST(test_off_time_default) {
  assert(getOffTime(10.1f) == MIN_MS(10));
  assert(getOffTime(15.0f) == MIN_MS(10));
  assert(getOffTime(18.0f) == MIN_MS(10));
  assert(getOffTime(22.0f) == MIN_MS(10));
  assert(getOffTime(25.0f) == MIN_MS(10));
}

TEST(test_off_time_warm) {
  assert(getOffTime(25.1f) == MIN_MS(5));
  assert(getOffTime(28.0f) == MIN_MS(5));
  assert(getOffTime(30.0f) == MIN_MS(5));
}

TEST(test_off_time_hot) {
  assert(getOffTime(30.1f) == MIN_MS(2));
  assert(getOffTime(35.0f) == MIN_MS(2));
}

// ═══════════════════════════════════════════
// Boundary tests
// ═══════════════════════════════════════════

TEST(test_boundary_at_exactly_30) {
  assert(getOnTime(30.0f)  == MIN_MS(1));
  assert(getOffTime(30.0f) == MIN_MS(5));
}

TEST(test_boundary_at_exactly_25) {
  assert(getOffTime(25.0f) == MIN_MS(10));
}

TEST(test_boundary_at_exactly_1) {
  assert(shouldPumpRun(1.0f) == false);
  assert(shouldPumpRun(0.0f) == false);
}

// ═══════════════════════════════════════════
// Pump cycle decision
// ═══════════════════════════════════════════

TEST(test_pump_on_should_not_toggle_early) {
  assert(shouldTogglePump(true, 20.0f, 30000) == false);
}

TEST(test_pump_on_should_toggle_at_threshold) {
  assert(shouldTogglePump(true, 20.0f, MIN_MS(1)) == true);
}

TEST(test_pump_off_should_not_toggle_early) {
  assert(shouldTogglePump(false, 20.0f, MIN_MS(5)) == false);
}

TEST(test_pump_off_should_toggle_at_threshold) {
  assert(shouldTogglePump(false, 20.0f, MIN_MS(10)) == true);
}

TEST(test_pump_hot_cycle) {
  assert(shouldTogglePump(true, 35.0f, MIN_MS(2)) == true);
  assert(shouldTogglePump(false, 35.0f, MIN_MS(2)) == true);
  assert(shouldTogglePump(true, 35.0f, MIN_MS(1)) == false);
  assert(shouldTogglePump(false, 35.0f, MIN_MS(1)) == false);
}

TEST(test_pump_should_not_run_freezing) {
  assert(shouldPumpRun(1.0f) == false);
  assert(shouldPumpRun(0.0f) == false);
  assert(shouldPumpRun(-10.0f) == false);
  assert(shouldPumpRun(1.1f) == true);
  assert(shouldPumpRun(5.0f) == true);
}

// ═══════════════════════════════════════════
// Runner
// ═══════════════════════════════════════════

int main() {
  printf("FLANKTUS v6.0 Logic Tests\n");
  printf("=========================\n");

  printf("\nTiming profiles:\n");
  RUN(test_on_time_below_30);
  RUN(test_on_time_above_30);
  RUN(test_off_time_cold);
  RUN(test_off_time_default);
  RUN(test_off_time_warm);
  RUN(test_off_time_hot);

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
  RUN(test_pump_should_not_run_freezing);

  printf("\n=========================\n");
  printf("All %d tests passed!\n", tests_passed);
  return 0;
}
