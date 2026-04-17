#!/bin/bash
# Clear the EEPROM ring buffer by sending 'c' to the Arduino.
# Usage: ./clear_eeprom.sh

PORT=$(arduino-cli board list | grep arduino:avr:uno | awk '{print $1}')
if [ -z "$PORT" ]; then
  echo "No Arduino found. Check USB connection."
  exit 1
fi

stty -f "$PORT" 9600 cs8 -cstopb -parenb
sleep 2
echo "c" > "$PORT"
echo "Sent clear command to $PORT. EEPROM log reset."
