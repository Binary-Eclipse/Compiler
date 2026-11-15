#!/bin/sh
# Builds ShakilScript on Linux/macOS. Prints install hints if a tool is missing.
cd "$(dirname "$0")/.." || exit 1
missing=""
for t in gcc make flex bison; do command -v "$t" >/dev/null 2>&1 || missing="$missing $t"; done
if [ -n "$missing" ]; then
    echo "[ShakilScript] Missing build tools:$missing"
    if command -v apt-get >/dev/null 2>&1; then echo "  Install with: sudo apt-get install -y build-essential flex bison"
    elif command -v dnf >/dev/null 2>&1; then echo "  Install with: sudo dnf install -y gcc make flex bison"
    elif command -v pacman >/dev/null 2>&1; then echo "  Install with: sudo pacman -S --needed base-devel flex bison"
    elif command -v brew >/dev/null 2>&1; then echo "  Install with: brew install flex bison  (then put brew's bison first on PATH)"
    fi
    exit 1
fi
major=$(bison --version | head -n 1 | sed 's/[^0-9]*\([0-9][0-9]*\).*/\1/')
if [ "${major:-0}" -lt 3 ]; then
    echo "[ShakilScript] Bison $major is too old (3.x needed). On macOS: brew install bison, then"
    echo "  export PATH=\"\$(brew --prefix bison)/bin:\$PATH\""
    exit 1
fi
exec make --no-print-directory
