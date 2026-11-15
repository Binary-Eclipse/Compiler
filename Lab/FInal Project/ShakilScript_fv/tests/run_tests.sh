#!/bin/sh
# ShakilScript test runner.
#   tests/t*.ss        -> stdout must equal tests/*.out  (stdin from tests/*.in when present)
#   tests/errors/e*.ss -> "line N: ErrorType: message" must equal tests/errors/*.err
# The .out/.err files were produced by running the same programs with real Python 3,
# so a pass means ShakilScript behaves exactly like Python for these programs.
# Optional: SHAKIL_EXE="path/to/other.exe" sh tests/run_tests.sh
cd "$(dirname "$0")/.." || exit 1
EXE=${SHAKIL_EXE:-./shakilscript.exe}
TMP=${TMPDIR:-/tmp}
pass=0; fail=0

run() {  # run <file> <stdin>  -> $TMP/ss_out.txt, $TMP/ss_err.txt (CR removed), sets $status
    $EXE "$1" < "$2" > "$TMP/ss_raw_out.txt" 2> "$TMP/ss_raw_err.txt"
    status=$?
    tr -d '\r' < "$TMP/ss_raw_out.txt" > "$TMP/ss_out.txt"
    tr -d '\r' < "$TMP/ss_raw_err.txt" > "$TMP/ss_err.txt"
}

for f in tests/t*.ss; do
    base=${f%.ss}
    if [ -f "$base.in" ]; then input="$base.in"; else input=/dev/null; fi
    run "$f" "$input"
    if [ $status -eq 0 ] && cmp -s "$TMP/ss_out.txt" "$base.out"; then
        pass=$((pass+1)); echo "PASS  $f"
    else
        fail=$((fail+1)); echo "FAIL  $f"
        diff "$TMP/ss_out.txt" "$base.out" | head -5; head -3 "$TMP/ss_err.txt"
    fi
done

for f in tests/errors/e*.ss; do
    base=${f%.ss}
    run "$f" /dev/null
    got=$(sed -e 's/^ShakilScript error (line \([0-9]*\)): /line \1: /' "$TMP/ss_err.txt" | head -1)
    want=$(cat "$base.err")
    if [ $status -ne 0 ] && [ "$got" = "$want" ] && cmp -s "$TMP/ss_out.txt" "$base.out"; then
        pass=$((pass+1)); echo "PASS  $f"
    else
        fail=$((fail+1)); echo "FAIL  $f"; echo "   expected: $want"; echo "   got:      $got"
    fi
done

echo "-----------------------------------"
echo "passed: $pass   failed: $fail"
[ $fail -eq 0 ]
