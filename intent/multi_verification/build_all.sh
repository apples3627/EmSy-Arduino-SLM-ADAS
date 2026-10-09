#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
cli=${ARDUINO_CLI:-/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli}
for part in 0 1 2; do
 "$cli" compile --warnings all --fqbn arduino:avr:uno --build-path "intent/multi_verification/build_probe_$part" --build-property "compiler.cpp.extra_flags=-DINTENT_PART=$part" intent/uno_probe > "intent/multi_verification/build_probe_$part.log" 2>&1
done
for board in main compute_i2c compute_spi actuator_uart; do
 "$cli" compile --warnings all --fqbn arduino:avr:uno --build-path "intent/multi_verification/build_$board" "firmware/$board" > "intent/multi_verification/build_$board.log" 2>&1
done
