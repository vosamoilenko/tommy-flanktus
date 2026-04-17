#!/bin/bash
PORT=$(arduino-cli board list | grep arduino:avr:uno | awk '{print $1}')
if [ -z "$PORT" ]; then
  echo "No Arduino found. Check USB connection."
  exit 1
fi
echo "Found Arduino on $PORT"

# Kill anything holding the serial port
lsof "$PORT" 2>/dev/null | awk 'NR>1{print $2}' | sort -u | xargs kill 2>/dev/null && sleep 2

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

arduino-cli compile --fqbn arduino:avr:uno "$PROJECT_DIR/flanktus_pump" && \
arduino-cli upload -p "$PORT" --fqbn arduino:avr:uno "$PROJECT_DIR/flanktus_pump"
