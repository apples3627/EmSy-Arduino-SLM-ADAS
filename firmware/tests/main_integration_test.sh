#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
peer_root=${MAIN_PEER_ROOT:-$(pwd)/firmware}
build_dir=$(mktemp -d /tmp/main-integration.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
cxx=${CXX:-c++}
"$cxx" -std=c++17 -Wall -Wextra -DMAIN_PEER=0 "-DACTOR_HEADER=\"$peer_root/actuator_uart/src/control_engine.h\"" -c firmware/tests/main_peer_bridge.cpp -o "$build_dir/actor.o"
"$cxx" -std=c++17 -Wall -Wextra -DMAIN_PEER=1 "-DCOMPUTE_HEADER=\"$peer_root/compute_i2c/src/neural_worker.h\"" -c firmware/tests/main_peer_bridge.cpp -o "$build_dir/i2c.o"
"$cxx" -std=c++17 -Wall -Wextra -DMAIN_PEER=2 "-DCOMPUTE_HEADER=\"$peer_root/compute_spi/src/neural_worker.h\"" -c firmware/tests/main_peer_bridge.cpp -o "$build_dir/spi.o"
"$cxx" -std=c++17 -Wall -Wextra -Ifirmware/tests/main_fake_avr firmware/tests/main_orchestration_test.cpp "$build_dir/actor.o" "$build_dir/i2c.o" "$build_dir/spi.o" -o "$build_dir/test"
"$build_dir/test"
