CC      = gcc
CFLAGS  = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -g -Isrc
TARGET  = shakilscript.exe
SOURCES = main.c parser.tab.c lex.yy.c src/runtime.c

# Windows needs a bigger stack for deep recursion (Python allows ~1000 nested calls)
ifeq ($(OS),Windows_NT)
LDFLAGS = -Wl,--stack,16777216
endif

.PHONY: all clean test

all: $(TARGET)

parser.tab.c parser.tab.h: src/parser.y src/runtime.h
	bison -d -o parser.tab.c src/parser.y

lex.yy.c: src/lexer.l parser.tab.h src/runtime.h
	flex -o lex.yy.c src/lexer.l

$(TARGET): parser.tab.c lex.yy.c $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) -lm $(LDFLAGS)

test: $(TARGET)
	sh tests/run_tests.sh

clean:
	rm -f $(TARGET) parser.tab.c parser.tab.h lex.yy.c
