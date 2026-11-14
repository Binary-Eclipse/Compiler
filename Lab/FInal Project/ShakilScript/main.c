#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/runtime.h"

#ifdef _WIN32
#include <windows.h>
static UINT saved_output_cp, saved_input_cp;
static void restore_console(void) {
    SetConsoleOutputCP(saved_output_cp);
    SetConsoleCP(saved_input_cp);
}
/* Make the console use UTF-8 so non-English text in .shakil files prints correctly. */
static void setup_console(void) {
    saved_output_cp = GetConsoleOutputCP();
    saved_input_cp = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    atexit(restore_console);
}
#else
static void setup_console(void) {}
#endif

int yyparse(void);
extern FILE *yyin;

/* Editors such as Windows Notepad may save a UTF-8 byte-order mark; skip it. */
static void skip_utf8_bom(FILE *f) {
    unsigned char bom[3];
    size_t n = fread(bom, 1, 3, f);
    if (n == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) return;
    rewind(f);
}

int main(int argc, char **argv) {
    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        printf("Usage: %s <source-file.shakil | source-file.ss>\n", argv[0]);
        return EXIT_SUCCESS;
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <source-file.shakil | source-file.ss>\n", argv[0]);
        return EXIT_FAILURE;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        fprintf(stderr, "Could not open source file '%s': ", argv[1]);
        perror(NULL);
        return EXIT_FAILURE;
    }
    skip_utf8_bom(yyin);
    setup_console();

    int result = yyparse();
    fclose(yyin);

    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
