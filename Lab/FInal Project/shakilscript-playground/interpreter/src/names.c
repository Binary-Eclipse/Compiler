#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "names.h"

/* ShakilScript name  ->  internal (front-end) name.  Edit THIS table to rename a word. */
typedef struct { const char *ss; const char *internal; int in_messages; } Word;
static const Word WORDS[] = {
    /* 1  print   */ { "show",     "print",    1 },
    /* 2  input   */ { "ask",      "input",    1 },
    /* 12 when    */ { "when",     "if",       0 },
                     { "orwhen",   "elif",     0 },
                     { "otherwise","else",     0 },
    /* 13 repeat  */ { "each",     "for",      0 },
                     { "during",   "while",    0 },
                     { "span",     "range",    1 },
    /* 14 stop    */ { "stop",     "break",    0 },
    /* 15 skip    */ { "skip",     "continue", 0 },
    /* 16 return  */ { "give",     "return",   0 },
    /* 17 call    */ { "task",     "def",      0 },
    /* 18 sort    */ { "arrange",  "sort",     1 },
                     { "arranged", "sorted",   1 },
    /* 19 find    */ { "within",   "in",       0 },
                     { "locate",   "index",    0 },
                     { "seek",     "find",     1 },
    /* 21 append  */ { "push",     "append",   1 },
    /* 22 remove  */ { "erase",    "remove",   1 },
                     { "pull",     "pop",      1 },
    /* 23 count   */ { "size",     "len",      1 },
                     { "tally",    "count",    1 },
    /* 24 reverse */ { "flip",     "reverse",  0 },
    /* 25 convert */ { "whole",    "int",      1 },
                     { "decimal",  "float",    1 },
                     { "text",     "str",      1 },
                     { "truth",    "bool",     1 },
};
#define NWORDS (sizeof WORDS / sizeof WORDS[0])

static int is_id_start(int c) { return isalpha(c) || c == '_'; }
static int is_id_char(int c)  { return isalnum(c) || c == '_'; }

char *ss_translate_source(const char *src) {
    size_t n = strlen(src);
    char *out = malloc(n * 2 + 16);   /* mapped words never grow beyond 2x */
    size_t o = 0, i = 0;
    int line = 1;
    if (!out) return NULL;
    while (i < n) {
        char c = src[i];
        if (c == '\n') { line++; out[o++] = c; i++; }
        else if (c == '#') { while (i < n && src[i] != '\n') out[o++] = src[i++]; }
        else if (c == '"' || c == '\'') {
            char q = c;
            out[o++] = src[i++];
            while (i < n && src[i] != q && src[i] != '\n') {
                if (src[i] == '\\' && i + 1 < n && src[i + 1] != '\n') out[o++] = src[i++];
                out[o++] = src[i++];
            }
            if (i < n && src[i] == q) out[o++] = src[i++];
        }
        else if (is_id_start((unsigned char)c)) {
            size_t s = i;
            while (i < n && is_id_char((unsigned char)src[i])) i++;
            size_t len = i - s;
            const char *rep = NULL, *hint = NULL;
            for (size_t k = 0; k < NWORDS; k++) {
                if (strlen(WORDS[k].ss) == len && !strncmp(src + s, WORDS[k].ss, len)) { rep = WORDS[k].internal; break; }
            }
            if (!rep) {                         /* old Python word? -> reject */
                for (size_t k = 0; k < NWORDS; k++) {
                    if (strlen(WORDS[k].internal) == len && !strncmp(src + s, WORDS[k].internal, len)) { hint = WORDS[k].ss; break; }
                }
                if (hint) {
                    fflush(stdout);
                    fprintf(stderr, "ShakilScript error (line %d): NameError: '%.*s' is not a ShakilScript word; use '%s'\n",
                            line, (int)len, src + s, hint);
                    free(out);
                    return NULL;
                }
            }
            if (rep) { size_t rl = strlen(rep); memcpy(out + o, rep, rl); o += rl; }
            else { memcpy(out + o, src + s, len); o += len; }
        }
        else { out[o++] = c; i++; }
    }
    out[o] = 0;
    return out;
}

char *ss_localize_message(const char *msg) {
    size_t n = strlen(msg);
    char *out = malloc(n * 2 + 16);
    size_t o = 0, i = 0;
    if (!out) return NULL;
    while (i < n) {
        if (is_id_start((unsigned char)msg[i]) && (i == 0 || !is_id_char((unsigned char)msg[i - 1]))) {
            size_t s = i;
            while (i < n && is_id_char((unsigned char)msg[i])) i++;
            size_t len = i - s;
            int quoted = s > 0 && msg[s - 1] == '\'' && i < n && msg[i] == '\'';
            int call = i < n && msg[i] == '(';
            const char *rep = NULL;
            if (quoted || call)
                for (size_t k = 0; k < NWORDS; k++)
                    if (WORDS[k].in_messages && strlen(WORDS[k].internal) == len && !strncmp(msg + s, WORDS[k].internal, len)) { rep = WORDS[k].ss; break; }
            if (rep) { size_t rl = strlen(rep); memcpy(out + o, rep, rl); o += rl; }
            else { memcpy(out + o, msg + s, len); o += len; }
        } else out[o++] = msg[i++];
    }
    out[o] = 0;
    return out;
}
