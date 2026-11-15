%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
int yylex(void);
void yyerror(const char *message);
extern FILE *yyin;
extern int lexical_error_count;
%}
%code requires { #include "runtime.h" }
%locations
%define parse.error verbose
%union { char *string; Value *value; Expr *expr; ExprList *exprs; StringList *strings; Stmt *stmt; }
%token PRINT "print" INPUT "input" CREATE "create" UPDATE "update" GET "get" ADD "add" SUBTRACT "subtract" MULTIPLY "multiply" DIVIDE "divide" MODULO "modulo"
%token COMPARE "compare" WHEN "when" REPEAT "repeat" STOP "stop" SKIP "skip" RETURN "return" CALL "call" SORT "sort" FIND "find" AT "at" APPEND "append" REMOVE "remove" COUNT "count" REVERSE "reverse" CONVERT "convert" FUNCTION "function"
%token TRUE_VALUE "true" FALSE_VALUE "false"
%token <string> STRING "string" IDENT "name"
%token <value> INTEGER "integer" FLOATING "float"
%token BAD_TOKEN "invalid token"
%type <expr> expression optional_expression
%type <exprs> arguments optional_arguments optional_call_args
%type <strings> parameters optional_parameters
%type <stmt> statement block statements
%left '+' '-'
%left '*' '/'
%right UMINUS
%start program
%%
program: statements { program_add_list($1); runtime_execute(); };
statements: /* empty */ { $$=NULL; }
 | statements statement { $$=stmt_block_add($1,$2); };
block: '{' statements '}' { $$=$2; };
statement:
 PRINT '(' expression ')' ';' { $$=stmt_at(stmt_print($3),@1.first_line); }
| CREATE '(' STRING ',' expression ')' ';' { $$=stmt_at(stmt_create($3,$5),@1.first_line); }
| UPDATE '(' STRING ',' expression ')' ';' { $$=stmt_at(stmt_update($3,$5),@1.first_line); }
| expression ';' { $$=stmt_at(stmt_expr($1),@1.first_line); }
| WHEN '(' expression ')' block { $$=stmt_at(stmt_when($3,$5),@1.first_line); }
| REPEAT '(' expression ')' block { $$=stmt_at(stmt_repeat($3,$5),@1.first_line); }
| STOP '(' ')' ';' { $$=stmt_at(stmt_control(1,NULL),@1.first_line); }
| SKIP '(' ')' ';' { $$=stmt_at(stmt_control(2,NULL),@1.first_line); }
| RETURN '(' optional_expression ')' ';' { $$=stmt_at(stmt_control(3,$3),@1.first_line); }
| FUNCTION IDENT '(' optional_parameters ')' block { $$=stmt_at(stmt_function($2,$4,$6),@1.first_line); }
;
optional_expression: /* empty */ { $$=NULL; } | expression { $$=$1; };
optional_arguments: /* empty */ { $$=NULL; } | arguments { $$=$1; };
arguments: expression { $$=expr_list_one($1); } | arguments ',' expression { $$=expr_list_add($1,$3); };
optional_parameters: /* empty */ { $$=NULL; } | parameters { $$=$1; };
parameters: IDENT { $$=string_list_one($1); } | parameters ',' IDENT { $$=string_list_add($1,$3); };
expression:
 INTEGER { $$=expr_value($1); }
| FLOATING { $$=expr_value($1); }
| STRING { $$=expr_value(value_string($1)); free($1); }
| TRUE_VALUE { $$=expr_value(value_bool(true)); }
| FALSE_VALUE { $$=expr_value(value_bool(false)); }
| IDENT { $$=expr_variable($1); }
| GET '(' STRING ')' { $$=expr_variable($3); }
| INPUT '(' expression ')' { $$=expr_call("input",expr_list_one($3)); }
| ADD '(' arguments ')' { $$=expr_call("add",$3); }
| SUBTRACT '(' arguments ')' { $$=expr_call("subtract",$3); }
| MULTIPLY '(' arguments ')' { $$=expr_call("multiply",$3); }
| DIVIDE '(' arguments ')' { $$=expr_call("divide",$3); }
| MODULO '(' arguments ')' { $$=expr_call("modulo",$3); }
| COMPARE '(' arguments ')' { $$=expr_call("compare",$3); }
| CALL '(' STRING optional_call_args ')' { $$=expr_call("call",expr_list_prepend(expr_value(value_string($3)),$4)); }
| SORT '(' arguments ')' { $$=expr_call("sort",$3); }
| FIND '(' arguments ')' { $$=expr_call("find",$3); }
| AT '(' arguments ')' { $$=expr_call("at",$3); }
| APPEND '(' arguments ')' { $$=expr_call("append",$3); }
| REMOVE '(' arguments ')' { $$=expr_call("remove",$3); }
| COUNT '(' arguments ')' { $$=expr_call("count",$3); }
| REVERSE '(' arguments ')' { $$=expr_call("reverse",$3); }
| CONVERT '(' arguments ')' { $$=expr_call("convert",$3); }
| '[' optional_arguments ']' { $$=expr_list($2); }
| '(' expression ')' { $$=$2; }
| '-' expression %prec UMINUS { $$=expr_binary('-',expr_value(value_int(0)),$2); }
| expression '+' expression { $$=expr_binary('+',$1,$3); }
| expression '-' expression { $$=expr_binary('-',$1,$3); }
| expression '*' expression { $$=expr_binary('*',$1,$3); }
| expression '/' expression { $$=expr_binary('/',$1,$3); }
;
optional_call_args: /* empty */ { $$=NULL; } | ',' arguments { $$=$2; };
%%
void yyerror(const char *message) {
    if (lexical_error_count > 0) return;   /* the lexer already explained the problem */
    fprintf(stderr, "ShakilScript syntax error at line %d: %s\n", yylloc.first_line, message);
    if (strstr(message, "';'"))
        fprintf(stderr, "  hint: every statement must end with ';' (check the line above this one)\n");
}
