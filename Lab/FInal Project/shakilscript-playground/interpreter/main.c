#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/runtime.h"
#include "src/names.h"

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
    FILE *f = fopen(path, "rb");
    if (!f) { perror("Could not open source file"); return EXIT_FAILURE; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    char *src = malloc((size_t)sz + 1);
    if (!src) { fclose(f); return EXIT_FAILURE; }
    size_t got = fread(src, 1, (size_t)sz, f); src[got] = 0; fclose(f);
    /* ShakilScript words -> internal keywords (see src/names.c) */
    char *translated = ss_translate_source(src);
    free(src);
    if (!translated) return EXIT_FAILURE;
    yyin = tmpfile();
    if (!yyin) { perror("tmpfile"); return EXIT_FAILURE; }
    fputs(translated, yyin); rewind(yyin);
    free(translated);
    int parsed = yyparse();
    fclose(yyin);
    if (parsed != 0) return EXIT_FAILURE;
    return runtime_run(check_only) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
