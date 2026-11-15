%option noyywrap
%option yylineno
%option nounput
%option noinput

%{
/*
 * ShakilScript lexer (Python-format syntax).
 *
 * Flex produces the raw tokens (raw_lex). The wrapper yylex() at the bottom turns
 * leading whitespace into INDENT / DEDENT tokens, exactly like Python's tokenizer.
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.tab.h"
#include "runtime.h"

#define YY_DECL int raw_lex(void)
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;

#define MAX_INDENT 100
static int indent_stack[MAX_INDENT] = {0};
static int indent_top = 0;
static int at_line_start = 1;   /* no token seen yet on the current logical line   */
static int pending_indent = 0;  /* width of the leading whitespace of this line    */
static int paren_depth = 0;     /* inside ( ) or [ ] newlines are ignored          */
int lexer_reported_error = 0;   /* set when the lexer already printed an error      */

static int measure_indent(const char *s) {
    int w = 0;
    for (; *s; s++) {
        if (*s == ' ') w++;
        else if (*s == '\t') w = (w / 8 + 1) * 8;
        else if (*s == '\f') w = 0;
    }
    return w;
}
static int lex_error(const char *msg) {
    lexer_reported_error = 1;
    fprintf(stderr, "ShakilScript error (line %d): SyntaxError: %s\n", yylineno, msg);
    return BAD_TOKEN;
}
%}

DIGIT       [0-9]
EXPONENT    [eE][+-]?{DIGIT}+
FLOATNUM    ({DIGIT}+"."{DIGIT}*|"."{DIGIT}+){EXPONENT}?|{DIGIT}+{EXPONENT}
STRING      \"([^\\\"\n]|\\.)*\"|\'([^\\\'\n]|\\.)*\'
BADSTRING   \"([^\\\"\n]|\\.)*|\'([^\\\'\n]|\\.)*
NOTID       [^a-zA-Z0-9_]

%%

"\\"\r?\n                   { /* explicit line continuation */ }
"#".*                       { /* comment */ }
^[ \t\f]+                   { pending_indent = measure_indent(yytext); }
[ \t\f\r]+                  { }
\r?\n                       {
                                if (paren_depth == 0 && !at_line_start) {
                                    at_line_start = 1;
                                    pending_indent = 0;
                                    yylloc.first_line = yylloc.last_line = yylineno - 1;
                                    return NEWLINE;
                                }
                                if (paren_depth == 0) pending_indent = 0;   /* blank line */
                            }

"if"                        { return IF; }
"elif"                      { return ELIF; }
"else"                      { return ELSE; }
"while"                     { return WHILE; }
"for"                       { return FOR; }
"in"                        { return IN; }
"def"                       { return DEF; }
"return"                    { return RETURN; }
"break"                     { return BREAK; }
"continue"                  { return CONTINUE; }
"pass"                      { return PASS; }
"global"                    { return GLOBAL; }
"and"                       { return AND; }
"or"                        { return OR; }
"not"[ \t]+"in"/{NOTID}     { return NOTIN; }
"not"                       { return NOT; }
"is"[ \t]+"not"/{NOTID}     { return ISNOT; }
"is"                        { return IS; }
"True"                      { return TRUE_VALUE; }
"False"                     { return FALSE_VALUE; }
"None"                      { return NONE_VALUE; }

"class"|"import"|"from"|"try"|"except"|"finally"|"raise"|"with"|"lambda"|"yield"|"del"|"assert"|"nonlocal"|"async"|"await" {
                                char msg[128];
                                snprintf(msg, sizeof msg, "'%s' is not supported in ShakilScript", yytext);
                                return lex_error(msg);
                            }

\"\"\"|\'\'\'               { return lex_error("triple-quoted strings are not supported (use # comments)"); }
[fFrRbBuU]\"|[fFrRbBuU]\'   { return lex_error("prefixed strings (f\"..\", r\"..\", b\"..\") are not supported; use str() and +"); }

{FLOATNUM}                  { yylval.value = value_float(strtod(yytext, NULL)); return FLOAT; }
{DIGIT}+                    {
                                errno = 0;
                                long long n = strtoll(yytext, NULL, 10);
                                if (errno == ERANGE) return lex_error("integer literal too large (64-bit limit)");
                                yylval.value = value_int(n);
                                return INTEGER;
                            }
[a-zA-Z_][a-zA-Z0-9_]*      { yylval.string = strdup(yytext); return NAME; }
{STRING}                    { yylval.string = decode_string(yytext); return STRING; }
{BADSTRING}                 { return lex_error("unterminated string literal"); }

"=="                        { return EQ; }
"!="                        { return NE; }
"<="                        { return LE; }
">="                        { return GE; }
"//="                       { yylval.ival = OP_FLOORDIV; return AUGASSIGN; }
"**="                       { yylval.ival = OP_POW; return AUGASSIGN; }
"+="                        { yylval.ival = OP_ADD; return AUGASSIGN; }
"-="                        { yylval.ival = OP_SUB; return AUGASSIGN; }
"*="                        { yylval.ival = OP_MUL; return AUGASSIGN; }
"/="                        { yylval.ival = OP_DIV; return AUGASSIGN; }
"%="                        { yylval.ival = OP_MOD; return AUGASSIGN; }
"//"                        { return FLOORDIV; }
"**"                        { return POW; }
"("                         { paren_depth++; return '('; }
")"                         { if (paren_depth > 0) paren_depth--; return ')'; }
"["                         { paren_depth++; return '['; }
"]"                         { if (paren_depth > 0) paren_depth--; return ']'; }
[,:.=<>+*/%-]               { return yytext[0]; }

.                           {
                                char msg[64];
                                snprintf(msg, sizeof msg, "invalid character '%s'", yytext);
                                return lex_error(msg);
                            }

%%

/* ---------- INDENT / DEDENT generation ---------- */
static int queue[MAX_INDENT + 8];
static int qhead = 0, qtail = 0;
static int held_token = 0, has_held = 0, held_line = 1;

static void enqueue(int tok) {
    if (qhead == qtail) qhead = qtail = 0;
    queue[qtail++] = tok;
}
static int next_queued(void) {
    int t = queue[qhead++];
    yylloc.first_line = yylloc.last_line = held_line;
    return t;
}

int yylex(void) {
    if (qhead < qtail) return next_queued();
    if (has_held) { has_held = 0; return held_token; }

    int tok = raw_lex();
    held_line = yylineno;

    if (tok == 0) {                         /* end of file */
        if (!at_line_start) { enqueue(NEWLINE); at_line_start = 1; }
        while (indent_top > 0) { indent_top--; enqueue(DEDENT); }
        enqueue(0);
        return next_queued();
    }

    if (tok == NEWLINE) return tok;         /* the newline itself never triggers indentation logic */

    if (at_line_start && paren_depth == 0) {
        at_line_start = 0;
        int w = pending_indent;
        if (w > indent_stack[indent_top]) {
            if (indent_top + 1 >= MAX_INDENT) return lex_error("too many levels of indentation");
            indent_stack[++indent_top] = w;
            enqueue(INDENT);
        } else {
            while (w < indent_stack[indent_top]) { indent_top--; enqueue(DEDENT); }
            if (w != indent_stack[indent_top])
                return lex_error("IndentationError: unindent does not match any outer indentation level");
        }
        if (qhead < qtail) {
            held_token = tok; has_held = 1;
            held_line = yylloc.first_line;
            return next_queued();
        }
    }
    return tok;
}
