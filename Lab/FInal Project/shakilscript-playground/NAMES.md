# ShakilScript word list (the renamed functions)

| #  | Function | Old (Python) | **ShakilScript** | Example |
|---:|----------|--------------|------------------|---------|
| 1  | print    | `print`      | **`show`**       | `show("Hi", x)` |
| 2  | input    | `input`      | **`ask`**        | `name = ask("Name: ")` |
| 3-5 | create / update / get | `=` `+=` | unchanged (assignment) | `age = 20` |
| 6-10 | add subtract multiply divide modulo | `+ - * / %` | unchanged (operators) | `a + b` |
| 11 | compare  | `== != < >`  | unchanged        | `a >= b` |
| 12 | when     | `if` `elif` `else` | **`when` `orwhen` `otherwise`** | `when x > 5:` |
| 13 | repeat   | `for` `while` `in` `range` | **`each` `during` `within` `span`** | `each i within span(5):` |
| 14 | stop     | `break`      | **`stop`**       | `stop` |
| 15 | skip     | `continue`   | **`skip`**       | `skip` |
| 16 | return   | `return`     | **`give`**       | `give a + b` |
| 17 | call     | `def`        | **`task`**       | `task add(a, b):` |
| 18 | sort     | `sort` `sorted` | **`arrange` `arranged`** | `items.arrange()` |
| 19 | find     | `index` `find` `in` | **`locate` `seek` `within`** | `items.locate(9)` / `x within items` |
| 20 | at       | `items[2]`   | unchanged        | `items[2]` |
| 21 | append   | `append`     | **`push`**       | `items.push(9)` |
| 22 | remove   | `remove` `pop` | **`erase` `pull`** | `items.erase(9)` / `items.pull(0)` |
| 23 | count    | `len` `count` | **`size` `tally`** | `size(items)` |
| 24 | reverse  | `reverse`    | **`flip`**       | `items.flip()` / `arranged(items, flip=True)` |
| 25 | convert  | `int float str bool` | **`whole decimal text truth`** | `whole("12")` |

Old Python names are rejected: `print("x")` -> `NameError: 'print' is not a ShakilScript word; use 'show'`.
`and or not is pass global True False None` are unchanged. Because the new words are reserved,
do not use them as variable names (e.g. `text`, `size`, `stop`).

To rename a word, edit the table at the top of `interpreter/src/names.c` and
`tools/convert_to_ss.py`, then rebuild. To convert old programs: `python3 tools/convert_to_ss.py --inplace *.ss`.
