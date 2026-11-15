%option noyywrap
%option yylineno
%option nounput
%option noinput

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "parser.tab.h"
#include "runtime.h"

/* Number of lexical errors reported so far (the parser stays quiet about them). */
int lexical_error_count = 0;

/* Return a token and remember its source line for Bison's location tracking. */
#define TOKEN(t) do { yylloc.first_line = yylloc.last_line = yylineno; return (t); } while (0)
%}

DIGIT       [0-9]
INTEGER     {DIGIT}+
FLOATING    {DIGIT}+"."{DIGIT}+
STRING      \"([^\\\"\n]|\\.)*\"
OPENSTRING  \"([^\\\"\n]|\\.)*

%%

"print"     { TOKEN(PRINT); }
"input"     { TOKEN(INPUT); }
"create"    { TOKEN(CREATE); }
"update"    { TOKEN(UPDATE); }
"get"       { TOKEN(GET); }
"add"       { TOKEN(ADD); }
"subtract"  { TOKEN(SUBTRACT); }
"multiply"  { TOKEN(MULTIPLY); }
"divide"    { TOKEN(DIVIDE); }
"modulo"    { TOKEN(MODULO); }
"compare"   { TOKEN(COMPARE); }
"when"      { TOKEN(WHEN); }
"repeat"    { TOKEN(REPEAT); }
"stop"      { TOKEN(STOP); }
"skip"      { TOKEN(SKIP); }
"return"    { TOKEN(RETURN); }
"call"      { TOKEN(CALL); }
"sort"      { TOKEN(SORT); }
"find"      { TOKEN(FIND); }
"at"        { TOKEN(AT); }
"append"    { TOKEN(APPEND); }
"remove"    { TOKEN(REMOVE); }
"count"     { TOKEN(COUNT); }
"reverse"   { TOKEN(REVERSE); }
"convert"   { TOKEN(CONVERT); }
"function"  { TOKEN(FUNCTION); }

"true"      { TOKEN(TRUE_VALUE); }
"false"     { TOKEN(FALSE_VALUE); }

{FLOATING}  {
                yylval.value = value_float(strtod(yytext, NULL));
                TOKEN(FLOATING);
            }

[a-zA-Z_][a-zA-Z0-9_]* { yylval.string = strdup(yytext); TOKEN(IDENT); }

{INTEGER}   {
                errno = 0;
                long long n = strtoll(yytext, NULL, 10);
                if (errno == ERANGE) {
                    fprintf(stderr, "ShakilScript lexer error at line %d: integer literal '%s' is too large\n",
                            yylineno, yytext);
                    lexical_error_count++;
                    TOKEN(BAD_TOKEN);
                }
                yylval.value = value_int(n);
                TOKEN(INTEGER);
            }

{STRING}    {
                yylval.string = decode_string(yytext);
                TOKEN(STRING);
            }

{OPENSTRING} {
                fprintf(stderr, "ShakilScript lexer error at line %d: unterminated string (missing closing quotation mark)\n",
                        yylineno);
                lexical_error_count++;
                TOKEN(BAD_TOKEN);
            }

"//".*      { /* Ignore single-line comments. */ }
[ \t\r\n]+  { /* Ignore whitespace. */ }

"("         { TOKEN('('); }
"{"         { TOKEN('{'); }
"}"         { TOKEN('}'); }
"["         { TOKEN('['); }
"]"         { TOKEN(']'); }
")"         { TOKEN(')'); }
","         { TOKEN(','); }
";"         { TOKEN(';'); }
"+"         { TOKEN('+'); }
"-"         { TOKEN('-'); }
"*"         { TOKEN('*'); }
"/"         { TOKEN('/'); }

.           {
                fprintf(stderr,
                        "ShakilScript lexer error at line %d: unexpected character '%s'\n",
                        yylineno, yytext);
                lexical_error_count++;
                TOKEN(BAD_TOKEN);
            }

%%
