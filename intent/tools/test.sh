#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
python3 "$ROOT/tools/verify.py"
TEMP=$(mktemp -d)
trap 'rm -rf "$TEMP"' EXIT
c++ -std=c++11 -O2 "$ROOT/tools/policy_test.cpp" -o "$TEMP/policy"
"$TEMP/policy"
