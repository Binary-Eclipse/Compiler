# ShakilScript

A small beginner-friendly programming language with English function names, built with C, Flex and Bison
(Compiler Design Lab project). The language is defined by [`manual.txt`](manual.txt); this README is a quick
reference and setup guide.

```javascript
create("name", input("Your name: "));
print("Hello!");
print(get("name"));
```

## Quick start (Windows + VS Code)

1. Install [VS Code](https://code.visualstudio.com/) and open this folder (**File → Open Folder**).
2. Open any program, for example `examples/first_program.ss` or `examples/student_info.shakil`.
3. Press **F5**.

The first time, the build script notices that gcc/make/flex/bison are missing and installs them
automatically (MSYS2 via `winget`, or the official MSYS2 installer as a fallback). This needs an internet
connection and takes a few minutes; Windows may ask for permission. After that, F5 only rebuilds when a
source file changed, then runs your program in the VS Code terminal. Output, errors and `input()` prompts
all appear (and work) in that terminal.

Other ways to run, all of which build automatically:

| What | How |
|---|---|
| Run the open file | **F5**, or *Terminal → Run Task → ShakilScript: Run Current File* |
| Build only | **Ctrl+Shift+B** |
| Run all tests | *Terminal → Run Task → ShakilScript: Run Tests* |
| Without VS Code | double-click `build_windows.bat`, then `run_windows.bat examples\first_program.ss` (or drag a file onto `run_windows.bat`) |
| Command line | `.\shakilscript.exe examples\first_program.ss` (PowerShell) or `./shakilscript examples/first_program.ss` |

Source files may use any extension; `.shakil` is the recommended one, and the manual's examples use `.ss`.

## Manual setup (optional)

Needs **gcc, make, flex, bison**.

* **Windows:** install [MSYS2](https://www.msys2.org), open *MSYS2 UCRT64*, run
  `pacman -S --needed mingw-w64-ucrt-x86_64-gcc make flex bison`, then `make` in this folder.
  (`scripts\build.bat` does all of this for you, including finding MSYS2 and setting `PATH`.)
* **Ubuntu/Debian:** `sudo apt-get install build-essential flex bison`, then `make`.
* **macOS:** `brew install flex bison`, put Homebrew's bison first on `PATH` (Apple's bison is too old), then `make`.

`make` builds `shakilscript(.exe)`; `make test` runs the tests; `make doctor` shows which tools were found;
`make clean` removes generated files (they live in `build/`).

## The language

Every statement ends with `;`. Comments start with `//`. Text goes in `"double quotes"` (escapes: `\n \t \r \" \\`).
Types: integer (`24`), float (`3.85`), string (`"Shakil"`), boolean (`true`/`false`), list (`[1, 2, 3]`).
Operators `+ - * /` and parentheses work in expressions with the usual precedence; `-x` negates.
Names of functions below are reserved words and cannot be used as parameter names.

### The 25 functions (numbering follows the table in manual §15)

| # | Function | Syntax | Result / behaviour |
|--:|---|---|---|
| 1 | `print` | `print(value);` | Shows the value and a newline. Floats show up to 15 significant digits (`3.90` → `3.9`). |
| 2 | `input` | `input("prompt")` | Shows the prompt, reads a line, returns it as **text** (use `convert` for numbers). |
| 3 | `create` | `create("name", value);` | New variable. Error if it already exists in the current scope. |
| 4 | `update` | `update("name", value);` | Changes an existing variable. Error if undefined. |
| 5 | `get` | `get("name")` | The variable's value. Error if undefined. |
| 6 | `add` | `add(a, b)` | Sum. Integers stay integers; any float makes a float. |
| 7 | `subtract` | `subtract(a, b)` | `a - b`. |
| 8 | `multiply` | `multiply(a, b)` | Product. |
| 9 | `divide` | `divide(a, b)` | `a / b`. Exact integer division gives an integer (`20/5` → `4`), otherwise a float (`7/2` → `3.5`). Dividing by zero is a runtime error. |
| 10 | `modulo` | `modulo(a, b)` | Remainder of two integers (`modulo(10,3)` → `1`; sign follows `a`, as in C). Zero divisor is an error. |
| 11 | `compare` | `compare(a, "op", b)` | `true`/`false`. `op` is `==` `!=` `>` `<` `>=` `<=`. Ordering needs two numbers or two strings. |
| 12 | `when` | `when(condition) { ... }` | Runs the block if the condition is true (booleans; numbers ≠ 0; non-empty strings). |
| 13 | `repeat` | `repeat(n) { ... }` | Runs the block *n* times (*n* a non-negative whole number). |
| 14 | `stop` | `stop();` | Leaves the innermost `repeat`. |
| 15 | `skip` | `skip();` | Jumps to the next iteration of the innermost `repeat`. |
| 16 | `return` | `return(value);` | Ends a user function and gives back the value (`return();` gives `0`). |
| 17 | `call` | `call("name", args...)` | Calls a function defined with `function`. Argument count must match. |
| 18 | `sort` | `sort(list)` | Sorts **in place** (numbers or strings, stable); returns the list. |
| 19 | `find` | `find(list, value)` | Index of the first match, or `-1` if not found. |
| 20 | `at` | `at(list, index)` | Element at `index` (first is `0`). Out-of-range is an error. |
| 21 | `append` | `append(list, value)` | Adds to the end **in place**; returns the list. |
| 22 | `remove` | `remove(list, index)` | Removes the element at `index` (index-based, as the manual specifies) **in place**; returns the list. |
| 23 | `count` | `count(list)` | Number of elements. |
| 24 | `reverse` | `reverse(list)` | Reverses **in place**; returns the list. |
| 25 | `convert` | `convert(value, "type")` | `type` is `"integer"`, `"float"`, `"string"` or `"boolean"` (`"int"`/`"bool"` also accepted). Invalid conversions are errors, e.g. `convert("abc","integer")`. |

User-defined functions use the `function` keyword (it is syntax, not one of the 25):

```javascript
function square(number) {
    return(multiply(number, number));
}
print(call("square", 5));   // 25
```

Inside a function, parameters can be read by bare name (`number`) or `get("number")`.

### Rules the manual leaves open (what this implementation does)

* `stop()`/`skip()` must be inside a `repeat` of the same function, and `return()` inside a `function`.
  This is checked **before the program starts**, so a program with such a mistake prints nothing.
* Syntax errors also stop the program before anything runs.
* A function body has its own variable scope: it can read and `update` outer variables, and `create` makes a local.
  Variables created inside `when`/`repeat` blocks stay visible afterwards, so `create`-ing the same name twice
  (for example inside a loop) is an error — use `update`.
* Lists are shared, not copied: `create("b", get("a"))` makes `b` the same list; passing a list to a function lets it modify it.
* A function must be defined (its `function` statement executed) before it is called.
* A function that never calls `return` gives `0`. Recursion is limited to 1000 nested calls.
* Integer overflow, division by zero, bad indexes, wrong argument counts etc. are runtime errors with the line number.

## Errors

```
ShakilScript syntax error at line 2: syntax error, unexpected print, expecting ';'
ShakilScript semantic error at line 5: stop() can only be used inside repeat()
ShakilScript runtime error at line 3: undefined variable 'age'
```
Output printed before a runtime error is kept. The exit code is non-zero on any error.

## Examples

`examples/` — `first_program.ss`, `input_example.ss`, `functions_11_25.ss` (named in the manual) and the manual's six
programs: `student_info`, `calculator`, `age_check`, `repeat_message`, `user_function`, `collections`; plus `age_from_input`.

## Tests

`make test` (or `scripts\test.bat`, or the *Run Tests* task) builds and runs `tests/run_tests.sh`:
`tests/valid/*.shakil` are compared with `.out` files (optional `.in` is fed to stdin), and `tests/invalid/*.shakil`
must fail with the messages listed in their `.err` files. Every one of the 25 functions has tests, plus combinations,
syntax/semantic/runtime errors and edge cases.

## Project layout

```
main.c  Makefile  manual.txt  README.md
src/       lexer.l  parser.y  runtime.c  runtime.h
examples/  tests/{valid,invalid,run_tests.sh}
scripts/   build/run/test .bat + .sh, env.bat, setup_windows.ps1
.vscode/   tasks.json  launch.json  settings.json
build_windows.bat  run_windows.bat
```

## Troubleshooting

| Problem | Fix |
|---|---|
| "Build tools are missing" and setup fails | Check your internet connection, then run `scripts\setup_windows.ps1` again; or install MSYS2 manually (see *Manual setup*). If MSYS2 is not in `C:\msys64`, set the environment variable `SHAKIL_MSYS2` to its folder. |
| F5 does nothing or asks for an environment | Make sure the file you want to run is the active editor tab; choose **Run current ShakilScript file** in the Run and Debug panel. As a fallback use *Terminal → Run Task → ShakilScript: Run Current File*. |
| F5 prints weird path errors | Your default terminal is Git Bash/WSL. The workspace sets Command Prompt for Windows; if you overrode that, use the Run Task route. |
| `'make' is not recognized` | Run via `scripts\build.bat` / F5 (they add MSYS2 to `PATH`), or open the *MSYS2 UCRT64* terminal. |
| `Build FAILED` mentioning `parser.y`/`lexer.l` | Read the Bison/Flex message; after fixing run `make clean` then build again. |
| PowerShell: `/shakilscript.exe` not found | Use `.\shakilscript.exe examples\first_program.ss` (backslash and leading dot). |
| Accented/Bengali text looks wrong | The interpreter switches the console to UTF-8 while it runs; save your `.shakil` file as UTF-8 and use a font that has the glyphs (e.g. Cascadia Mono + Windows Terminal). |
| `unexpected character` / syntax error right at line 1 of a Notepad file | Fixed: a UTF-8 byte-order mark is skipped automatically. |
| `input()` fails with "end of input" | The program asked for a line but stdin was empty/closed; run it in a terminal. |
| macOS: `%code`/`parse.error` Bison errors | Apple's Bison is 2.3; install a newer one with Homebrew (see *Manual setup*). |
