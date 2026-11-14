# ShakilScript build file (GNU make).
#
#   make              build the interpreter (shakilscript / shakilscript.exe)
#   make test         build, then run the whole test suite
#   make run FILE=examples/first_program.ss
#   make doctor       show which build tools were found
#   make clean        remove everything that was generated
#
# Needs: gcc, make, flex, bison.  On Windows use scripts\build.bat (it installs them for you).

CC    := gcc
FLEX  := flex
BISON := bison

ifeq ($(OS),Windows_NT)
  EXE := .exe
  # static: the .exe runs without MSYS2 on PATH; bigger stack: deep recursion in user programs
  PLATFORM_LDFLAGS := -static -Wl,--stack,16777216
else
  EXE :=
  PLATFORM_LDFLAGS :=
endif

TARGET := shakilscript$(EXE)
BUILD  := build

CFLAGS     := -std=gnu11 -O2 -g -Wall -Wextra -Isrc -I$(BUILD)
GEN_CFLAGS := -std=gnu11 -O2 -g -w -Isrc -I$(BUILD)

OBJS := $(BUILD)/main.o $(BUILD)/runtime.o $(BUILD)/parser.tab.o $(BUILD)/lex.yy.o

.PHONY: all clean rebuild test run run-example run-input doctor

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(PLATFORM_LDFLAGS) -lm

$(BUILD):
	mkdir -p $(BUILD)

# Bison writes parser.tab.c and parser.tab.h in one run.
$(BUILD)/parser.tab.c: src/parser.y | $(BUILD)
	$(BISON) -d -o $@ $<
$(BUILD)/parser.tab.h: $(BUILD)/parser.tab.c

$(BUILD)/lex.yy.c: src/lexer.l $(BUILD)/parser.tab.h | $(BUILD)
	$(FLEX) -o $@ $<

$(BUILD)/main.o: main.c src/runtime.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/runtime.o: src/runtime.c src/runtime.h | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

# Generated code is compiled without warnings (they are not ours to fix).
$(BUILD)/parser.tab.o: $(BUILD)/parser.tab.c src/runtime.h
	$(CC) $(GEN_CFLAGS) -c -o $@ $<

$(BUILD)/lex.yy.o: $(BUILD)/lex.yy.c $(BUILD)/parser.tab.h src/runtime.h
	$(CC) $(GEN_CFLAGS) -c -o $@ $<

test: $(TARGET)
	sh tests/run_tests.sh ./$(TARGET)

run: $(TARGET)
	@test -n "$(FILE)" || { echo "Usage: make run FILE=examples/first_program.ss"; exit 1; }
	./$(TARGET) "$(FILE)"

run-example: $(TARGET)
	./$(TARGET) examples/first_program.ss

run-input: $(TARGET)
	./$(TARGET) examples/input_example.ss

doctor:
	@for t in $(CC) make $(FLEX) $(BISON); do \
	  if command -v $$t >/dev/null 2>&1; then echo "found    $$t: $$($$t --version 2>&1 | head -n 1)"; \
	  else echo "MISSING  $$t"; fi; done

rebuild: clean all

clean:
	rm -rf $(BUILD) $(TARGET)
	rm -f parser.tab.c parser.tab.h lex.yy.c
