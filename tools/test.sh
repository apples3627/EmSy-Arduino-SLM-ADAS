#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
python3 "$ROOT/tools/verify.py"
TESTDIR=$(mktemp -d)
trap 'rm -rf "$TESTDIR"' EXIT
c++ -std=c++11 -O2 "$ROOT/tools/firmware_test.cpp" -o "$TESTDIR/firmware_test"
"$TESTDIR/firmware_test"
