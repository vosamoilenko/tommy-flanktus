#!/bin/bash
# Upload sketch and monitor serial output in one step.
# Checks that Arduino + sensors are connected and working.
# Usage: ./deploy.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOGFILE="$PROJECT_DIR/logs/sensor_log.txt"
FQBN="arduino:avr:uno"

# ── Find Arduino ──
PORT=$(arduino-cli board list | grep "$FQBN" | awk '{print $1}')
if [ -z "$PORT" ]; then
  echo "ERROR: No Arduino Uno found. Check USB connection."
  echo ""
  echo "Available boards:"
  arduino-cli board list
  exit 1
fi
echo "Found Arduino on $PORT"

# ── Kill anything holding the serial port ──
lsof "$PORT" 2>/dev/null | awk 'NR>1{print $2}' | sort -u | xargs kill 2>/dev/null && sleep 2 || true

# ── Compile ──
echo ""
echo "Compiling..."
arduino-cli compile --fqbn "$FQBN" "$PROJECT_DIR/flanktus_pump"

# ── Upload ──
echo ""
echo "Uploading..."
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$PROJECT_DIR/flanktus_pump"
echo "Upload OK."

# ── Wait for Arduino to reboot ──
sleep 3

# ── Configure serial ──
stty -f "$PORT" 9600 cs8 -cstopb -parenb

# ── Health check: read first 10 seconds of output and verify ──
echo ""
echo "Running health check (10s)..."
echo "────────────────────────────────────"

HEALTH_OUTPUT=""
END=$((SECONDS + 10))
while [ $SECONDS -lt $END ]; do
  if read -r -t 1 line < "$PORT"; then
    echo "  $line"
    HEALTH_OUTPUT="$HEALTH_OUTPUT$line"$'\n'
  fi
done

echo "────────────────────────────────────"

# Check for boot message
ERRORS=0
if echo "$HEALTH_OUTPUT" | grep -q "FLANKTUS"; then
  echo "OK  Arduino is running"
else
  echo "FAIL  No boot message received — check USB connection"
  ERRORS=$((ERRORS + 1))
fi

# Check for debug output with sensor readings
if echo "$HEALTH_OUTPUT" | grep -q "\[DBG"; then
  # Check water sensor
  if echo "$HEALTH_OUTPUT" | grep "\[DBG" | grep -q "water=-99.9"; then
    echo "WARN  Water sensor disconnected (reads -99.9C)"
  else
    echo "OK  Water sensor connected"
  fi
  # Check air sensor
  if echo "$HEALTH_OUTPUT" | grep "\[DBG" | grep -q "air=-99.9"; then
    echo "WARN  Air sensor disconnected (reads -99.9C)"
  else
    echo "OK  Air sensor connected"
  fi
  # Check pump/auto state
  if echo "$HEALTH_OUTPUT" | grep "\[DBG" | grep -q "auto=ON"; then
    echo "OK  Auto mode active — pump is cycling"
  else
    echo "WARN  Auto mode is OFF"
  fi
else
  echo "FAIL  No debug output — something is wrong"
  ERRORS=$((ERRORS + 1))
fi

echo ""
if [ $ERRORS -gt 0 ]; then
  echo "Health check found $ERRORS error(s). Check wiring and connections."
  echo "Continuing to serial monitor anyway..."
fi

# ── Stream serial output ──
echo ""
echo "Streaming serial output... (Ctrl+C to stop)"
echo "Saving to $LOGFILE"
echo "────────────────────────────────────"
cat "$PORT" | tee "$LOGFILE"
