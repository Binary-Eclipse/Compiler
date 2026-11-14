#!/bin/sh
# Builds ShakilScript if needed, then runs one program:  scripts/run.sh path/to/program.shakil
[ $# -ge 1 ] || { echo "Usage: scripts/run.sh <program.shakil>"; exit 1; }
case "$1" in /*) SRC=$1 ;; *) SRC=$PWD/$1 ;; esac
[ -f "$SRC" ] || { echo "File not found: $SRC"; exit 1; }
ROOT=$(cd "$(dirname "$0")/.." && pwd)
"$ROOT/scripts/build.sh" || exit 1
echo "--- running $(basename "$SRC") ---"
"$ROOT/shakilscript" "$SRC"
rc=$?
echo
echo "--- finished (exit code $rc) ---"
exit $rc
