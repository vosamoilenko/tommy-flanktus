#!/bin/bash
# Remove rows with -99 (disconnected sensor) from exported CSV.
# Usage: ./clean_csv.sh input.csv [output.csv]
#   If no output file given, prints to stdout.

INPUT="${1:?Usage: $0 input.csv [output.csv]}"
OUTPUT="${2:-}"

if [ ! -f "$INPUT" ]; then
  echo "File not found: $INPUT"
  exit 1
fi

if [ -n "$OUTPUT" ]; then
  head -1 "$INPUT" > "$OUTPUT"
  tail -n +2 "$INPUT" | grep -v '\-99' >> "$OUTPUT"
  TOTAL=$(tail -n +2 "$INPUT" | wc -l | tr -d ' ')
  CLEAN=$(tail -n +2 "$OUTPUT" | wc -l | tr -d ' ')
  echo "Cleaned: $CLEAN/$TOTAL rows kept → $OUTPUT"
else
  head -1 "$INPUT"
  tail -n +2 "$INPUT" | grep -v '\-99'
fi
