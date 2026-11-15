#!/bin/sh
# Local run (Linux / macOS / WSL / MSYS2):  sh run_local.sh   ->  http://localhost:5000
set -e
cd "$(dirname "$0")"
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -Iinterpreter/src -o interpreter/shakilscript \
    interpreter/main.c interpreter/parser.tab.c interpreter/lex.yy.c interpreter/src/runtime.c interpreter/src/names.c -lm
pip install -q -r requirements.txt
python3 app.py
