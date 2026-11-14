#!/bin/sh
# Builds ShakilScript and runs the test suite.
ROOT=$(cd "$(dirname "$0")/.." && pwd)
"$ROOT/scripts/build.sh" || exit 1
exec sh "$ROOT/tests/run_tests.sh" "$ROOT/shakilscript"
