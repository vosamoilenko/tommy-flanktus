#!/bin/bash
# Read live debug logs from the Arduino serial port.
# Usage: ./read_logs.sh [duration_seconds]
#   No argument  → stream until Ctrl+C
#   With argument → capture for N seconds then stop
#
# Logs are printed to console AND saved to sensor_log.txt

PORT=$(arduino-cli board list | grep arduino:avr:uno | awk '{print $1}')
if [ -z "$PORT" ]; then
  echo "No Arduino found. Check USB connection."
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOGFILE="$PROJECT_DIR/logs/sensor_log.txt"
DURATION="${1:-}"

echo "Reading from $PORT (9600 baud)..."
echo "Saving to $LOGFILE"

stty -f "$PORT" 9600 cs8 -cstopb -parenb

if [ -n "$DURATION" ]; then
  echo "Capturing for ${DURATION}s..."
  timeout "$DURATION" cat "$PORT" | tee "$LOGFILE"
  echo ""
  echo "Done. $(wc -l < "$LOGFILE" | tr -d ' ') lines saved to $LOGFILE"
else
  echo "Streaming... press Ctrl+C to stop."
  cat "$PORT" | tee "$LOGFILE"
fi
