#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CLI=${ARDUINO_CLI:-/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli}
if [ ! -x "$CLI" ]; then CLI=arduino-cli; fi
mkdir -p "$ROOT/build"
"$CLI" compile --fqbn arduino:avr:uno --build-path "$ROOT/build" "$ROOT/tiny_uno"
