#!/bin/sh
# ShakilScript test runner.
#   tests/run_tests.sh [path-to-interpreter]
#
# tests/valid/NAME.shakil    must exit 0, print exactly NAME.out on stdout, print nothing on stderr.
#                            NAME.in (optional) is fed to standard input.
# tests/invalid/NAME.shakil  must exit non-zero; stdout must equal NAME.out (empty if missing);
#                            stderr must contain every non-empty line of NAME.err.
cd "$(dirname "$0")/.." || exit 1

BIN=${1:-}
if [ -z "$BIN" ]; then
    if [ -x ./shakilscript.exe ]; then BIN=./shakilscript.exe; else BIN=./shakilscript; fi
fi
if [ ! -x "$BIN" ]; then echo "Interpreter not found: $BIN (run 'make' first)"; exit 2; fi

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/shakil_tests_$$")
mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT
pass=0; fail=0

strip_cr() { tr -d '\r' < "$1" > "$1.n"; }

run_one() {   # $1 = source file, $2 = valid|invalid
    src=$1; kind=$2; base=${src%.shakil}; name=$(basename "$base")
    if [ -f "$base.in" ]; then input="$base.in"; else input=/dev/null; fi
    "$BIN" "$src" < "$input" > "$TMP/out" 2> "$TMP/err"; code=$?
    strip_cr "$TMP/out"; strip_cr "$TMP/err"
    if [ -f "$base.out" ]; then tr -d '\r' < "$base.out" > "$TMP/exp"; else : > "$TMP/exp"; fi
    problems=""
    if ! cmp -s "$TMP/out.n" "$TMP/exp"; then problems="$problems stdout-differs"; fi
    if [ "$kind" = valid ]; then
        [ "$code" -eq 0 ] || problems="$problems exit-code-$code"
        [ ! -s "$TMP/err.n" ] || problems="$problems unexpected-stderr"
    else
        [ "$code" -ne 0 ] || problems="$problems expected-nonzero-exit"
        if [ -f "$base.err" ]; then
            while IFS= read -r pat; do
                pat=$(printf '%s' "$pat" | tr -d '\r')
                [ -n "$pat" ] || continue
                grep -F -q -- "$pat" "$TMP/err.n" || problems="$problems stderr-missing[$pat]"
            done < "$base.err"
        else
            problems="$problems no-.err-file"
        fi
    fi
    if [ -z "$problems" ]; then
        pass=$((pass + 1)); echo "PASS  $kind/$name"
    else
        fail=$((fail + 1)); echo "FAIL  $kind/$name :$problems"
        echo "      --- expected stdout"; sed 's/^/      | /' "$TMP/exp"
        echo "      --- actual stdout";   sed 's/^/      | /' "$TMP/out.n"
        echo "      --- actual stderr";   sed 's/^/      | /' "$TMP/err.n"
    fi
}

for f in tests/valid/*.shakil;   do [ -f "$f" ] && run_one "$f" valid;   done
for f in tests/invalid/*.shakil; do [ -f "$f" ] && run_one "$f" invalid; done

echo
echo "Passed: $pass   Failed: $fail"
[ "$fail" -eq 0 ]
