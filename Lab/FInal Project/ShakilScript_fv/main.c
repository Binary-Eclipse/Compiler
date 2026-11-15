#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/runtime.h"

int yyparse(void);
extern FILE *yyin;

int main(int argc, char **argv) {
    int check_only = 0;
    const char *path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--check")) check_only = 1;
        else if (!strcmp(argv[i], "--version")) { puts("ShakilScript 2.0 (Python-format syntax)"); return EXIT_SUCCESS; }
        else path = argv[i];
    }
    if (!path) {
        fprintf(stderr, "Usage: %s [--check] <program.ss>\n", argv[0]);
        return EXIT_FAILURE;
    }
    yyin = fopen(path, "r");
    if (!yyin) { perror("Could not open source file"); return EXIT_FAILURE; }
    int parsed = yyparse();
    fclose(yyin);
    if (parsed != 0) return EXIT_FAILURE;
    return runtime_run(check_only) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
