#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
build_dir=$(mktemp -d /tmp/multi-verification.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT
for name in protocol_test main_policy_test actuator_engine_test multi_fw_actor_test compute_worker_test; do
 c++ -std=c++17 -O2 -Wall -Wextra "firmware/tests/$name.cpp" -o "$build_dir/$name"
 "$build_dir/$name"
 echo "$name PASS"
done
c++ -std=c++17 -O2 -Wall -Wextra intent/multi_verification/protocol_negative.cpp -o "$build_dir/protocol_negative"
"$build_dir/protocol_negative"
echo 'VERSION1/LABELS2/oldmodel rejection PASS'
sh firmware/tests/main_integration_test.sh
echo 'real MAIN + I2C/SPI workers + actor integration PASS'
