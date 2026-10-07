#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CLI=${ARDUINO_CLI:-/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli}
for PART in 0 1 2; do
 mkdir -p "$ROOT/build/part$PART"
 "$CLI" compile --fqbn arduino:avr:uno --build-path "$ROOT/build/part$PART" --build-property "compiler.cpp.extra_flags=-DINTENT_PART=$PART" "$ROOT/uno_probe"
done
