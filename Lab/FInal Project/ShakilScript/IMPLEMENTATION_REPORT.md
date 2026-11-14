# ShakilScript — Implementation Report

## 1. Verification status (read this first)

| Area | Status |
|---|---|
| `runtime.c` — all 25 functions, operators, scoping, errors | **Verified by execution** (see §4) |
| `main.c` — BOM skipping, bad path, usage | **Verified** with a stub parser |
| Grammar has no LALR conflicts | **Checked with my own analyzer** (not Bison itself) — 0 conflicts; analyzer validated against 3 deliberately broken grammars |
| `lexer.l` / `parser.y` actually processed by **Flex / Bison** | **NOT verified** — neither tool exists in my sandbox and there was no network |
| A real `shakilscript` binary built through the Makefile | **NOT verified** (same reason). The Makefile was dry-run and its non-generated objects compiled |
| Windows scripts (`*.bat`, `setup_windows.ps1`), MSYS2 auto-install, `-static` link | **NOT verified** — written from knowledge of cmd/PowerShell/MSYS2; never executed |
| VS Code F5 / tasks / launch config | **NOT verified** — JSON validated only; VS Code was not available |

The first thing to do on your machine: run `scripts\build.bat` (or `make test`). If Flex/Bison report anything,
it will be in `src/lexer.l` / `src/parser.y`, which differ from your originals only as listed in §3.

### How the runtime was verified without Flex/Bison
I wrote a throw-away hand-written front end (`standin`, **not shipped**) that mirrors `parser.y` rule by rule and calls
the same `runtime.h` API, linked it with the real `runtime.c`, and ran the test suite through it.
Expected outputs were derived by hand from `manual.txt`, not captured from the program.

## 2. Checklist — the 25 functions (manual §15 table)

All behave as the manual specifies in the 90-test suite (90/90 pass with the stand-in front end).

print · input · create · update · get · add · subtract · multiply · divide · modulo · compare · when · repeat · stop ·
skip · return · call · sort · find · at · append · remove · count · reverse · convert — plus `function` definitions,
list literals, `+ - * /`, unary minus, comments, booleans, floats, strings with escapes.

## 3. What was wrong / what changed

Gaps against `manual.txt` and bugs found in the original code:

1. **Bare identifiers could not be used** (`print(name)`, `return(add(a, b))` in manual §11/§14) — grammar had no rule. Added `expression: IDENT` (one production; the only grammar rule change).
2. **`convert()`** accepted `"int"`/`"bool"` but the manual says `"integer"`/`"boolean"`. Both now work. String→integer is now strict (`"3.5"`, `"abc"`, `"nan"`, `"1e999"` are errors; previously `strtod` + truncation, and `inf` caused undefined behaviour).
3. **`stop()`/`skip()`/`return()` placement** was only checked when executed, and a `stop()` in a called function could cancel the *caller's* loop. Now a pre-run semantic check rejects misplaced ones with a line number.
4. **`divide()` always returned a float**, so `modulo(divide(20,5),3)` failed. Exact integer division now stays an integer (prints identically).
5. **No line numbers** in errors → added `%locations` + `stmt_at()`; messages name variables/functions/values.
6. **`input()`** truncated lines > 4095 chars (rest leaked into the next `input`), kept `\r` from CRLF input, and put a 4 KB buffer in every recursive frame. Rewritten.
7. **Integer overflow** was undefined behaviour → now an error. `modulo(x,-1)` edge handled.
8. **`sort`** was O(n²) and unstable → stable merge sort. `find`/`compare ==` now compare lists deeply.
9. **Recursion** had no limit (stack crash) → clean error at depth 1000; Windows link adds a 16 MB stack.
10. Float printing `%.10g` → `%.15g` (so `10/3` is not cut to 10 digits).
11. Lexer: unterminated-string and too-large-integer diagnostics; double error messages suppressed.
12. `main.c`: UTF-8 BOM skipped (Windows Notepad), UTF-8 console on Windows, better messages.
13. Project: created the folder layout shown in the manual (`src/`, `examples/`), generated files go to `build/`, Makefile is portable (`.exe` only on Windows), scripts, `.vscode`, tests, README.

`runtime.c` was **re-laid out** (the original was ~89 very long lines and nearly every function needed changes). Architecture (AST + tree-walking interpreter), all exported function names and existing behaviours were kept. `lexer.l`/`parser.y`/`main.c` were edited, not rewritten.

## 4. Verification results

* 90 tests (31 valid, 59 invalid: syntax, semantic, runtime) → 90/90 pass.
* Mutation testing: 12 injected runtime bugs; first run caught 10, the 2 survivors exposed real gaps (float overflow in `convert`, CR in `input`) → tests added → all 12 caught.
* AddressSanitizer + UBSan over the full suite: clean. Depth-1000 recursion limit triggers cleanly even with a 512 KB stack.
* `main.c` with a stub parser: BOM skipped, 2-byte/empty files fine, missing file/no args exit 1.

## 5. Decisions where the manual and the task disagree or are silent

* **File extension:** task says `.shakil`, manual says `.ss`. Both work (any extension does). Manual-named examples are `.ss`; others `.shakil`.
* **Function numbering:** manual section headings ("Function 1: create()") conflict with its §15 table ("1 print()"). README follows the table.
* **`manual.txt` was left untouched** (single source of truth), so its notes that some functions "still need grammar integration" are now stale.
* Silent in manual, chosen: `find` → `-1` when missing; `remove` index-based (manual says so); list functions mutate in place and return the list; no-`return` function yields `0`; `modulo` sign follows C; variables made in blocks stay visible; functions must be defined before calling.

## 6. Remaining limitations

* Keywords (`count`, `at`, `add`, …) can't be used as parameter names.
* Error messages carry a line number but not the file name, so VS Code can't make them clickable.
* No `else`, string concatenation, block comments — none are in the manual, so none were invented.
* Memory is never freed (fine for scripts of this scale).
* Syntax-error *wording* comes from Bison and is untested; tests only check the `ShakilScript syntax error at line N` prefix and the missing-`;` hint.
