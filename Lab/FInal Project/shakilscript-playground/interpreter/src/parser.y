%{
/*
 * ShakilScript grammar (Python-format syntax, a practical Python subset).
 * Blocks are written with ':' + NEWLINE + INDENT ... DEDENT, produced by the lexer.
 */
#include <stdio.h>
#include <stdlib.h>
#include "runtime.h"

int yylex(void);
void yyerror(const char *message);
extern int lexer_reported_error;
%}

%code requires { #include "runtime.h" }
%define parse.error detailed
%locations

%union {
    char *string;
    int ival;
    Value value;
    Expr *expr;
    ExprList *exprs;
    StringList *names;
    ParamList *params;
    Stmt *stmt;
}

%token IF "if"  ELIF "elif"  ELSE "else"  WHILE "while"  FOR "for"  IN "in"  DEF "def"
%token RETURN "return"  BREAK "break"  CONTINUE "continue"  PASS "pass"  GLOBAL "global"
%token AND "and"  OR "or"  NOT "not"  IS "is"  ISNOT "is not"  NOTIN "not in"
%token TRUE_VALUE "True"  FALSE_VALUE "False"  NONE_VALUE "None"
%token EQ "=="  NE "!="  LE "<="  GE ">="  FLOORDIV "//"  POW "**"
%token <ival> AUGASSIGN "augmented assignment"
%token <string> STRING "string"  NAME "name"
%token <value> INTEGER "integer"  FLOAT "decimal number"
%token NEWLINE "end of line"  INDENT "indent"  DEDENT "dedent"
%token BAD_TOKEN "invalid token"

%type <expr> expression opt_expression
%type <exprs> call_args arg_list list_items
%type <names> name_list
%type <params> parameters opt_parameters
%type <stmt> statements statement_list statement simple_stmt compound_stmt suite
%type <stmt> if_stmt elif_clause
%type <expr> argument

/* lowest -> highest precedence (same order as Python) */
%left OR
%left AND
%right NOT
%nonassoc EQ NE '<' '>' LE GE IN NOTIN IS ISNOT
%left '+' '-'
%left '*' '/' FLOORDIV '%'
%right UMINUS
%right POW
%left '(' '[' '.'

%start program

%%

program
    : statements                        { program_set($1); }
    ;

statements
    : /* empty */                       { $$ = NULL; }
    | statements statement              { $$ = stmt_block_add($1, $2); }
    ;

statement_list
    : statement                         { $$ = $1; }
    | statement_list statement          { $$ = stmt_block_add($1, $2); }
    ;

statement
    : simple_stmt NEWLINE               { $$ = $1; }
    | compound_stmt                     { $$ = $1; }
    ;

suite
    : simple_stmt NEWLINE               { $$ = $1; }
    | NEWLINE INDENT statement_list DEDENT { $$ = $3; }
    ;

simple_stmt
    : expression
        { $$ = stmt_set_line(stmt_expr($1), @1.first_line); }
    | expression '=' expression
        {
            if (!expr_is_target($1)) { yyerror("cannot assign to this expression"); YYERROR; }
            $$ = stmt_set_line(stmt_assign($1, $3), @1.first_line);
        }
    | expression AUGASSIGN expression
        {
            if (!expr_is_target($1)) { yyerror("illegal target for augmented assignment"); YYERROR; }
            $$ = stmt_set_line(stmt_assign($1, expr_binary($2, $1, $3)), @1.first_line);
        }
    | PASS                              { $$ = stmt_set_line(stmt_pass(), @1.first_line); }
    | BREAK                             { $$ = stmt_set_line(stmt_break(), @1.first_line); }
    | CONTINUE                          { $$ = stmt_set_line(stmt_continue(), @1.first_line); }
    | RETURN                            { $$ = stmt_set_line(stmt_return(NULL), @1.first_line); }
    | RETURN expression                 { $$ = stmt_set_line(stmt_return($2), @1.first_line); }
    | GLOBAL name_list                  { $$ = stmt_set_line(stmt_global($2), @1.first_line); }
    ;

compound_stmt
    : if_stmt                           { $$ = $1; }
    | WHILE expression ':' suite
        { $$ = stmt_set_line(stmt_while($2, $4), @1.first_line); }
    | FOR NAME IN expression ':' suite
        { $$ = stmt_set_line(stmt_for($2, $4, $6), @1.first_line); }
    | DEF NAME '(' opt_parameters ')' ':' suite
        { $$ = stmt_set_line(stmt_def($2, $4, $7), @1.first_line); }
    ;

if_stmt
    : IF expression ':' suite
        { $$ = stmt_set_line(stmt_if($2, $4, NULL), @1.first_line); }
    | IF expression ':' suite ELSE ':' suite
        { $$ = stmt_set_line(stmt_if($2, $4, $7), @1.first_line); }
    | IF expression ':' suite elif_clause
        { $$ = stmt_set_line(stmt_if($2, $4, $5), @1.first_line); }
    ;

elif_clause
    : ELIF expression ':' suite
        { $$ = stmt_set_line(stmt_if($2, $4, NULL), @1.first_line); }
    | ELIF expression ':' suite ELSE ':' suite
        { $$ = stmt_set_line(stmt_if($2, $4, $7), @1.first_line); }
    | ELIF expression ':' suite elif_clause
        { $$ = stmt_set_line(stmt_if($2, $4, $5), @1.first_line); }
    ;

name_list
    : NAME                              { $$ = string_list_one($1); }
    | name_list ',' NAME                { $$ = string_list_add($1, $3); }
    ;

opt_parameters
    : /* empty */                       { $$ = NULL; }
    | parameters                        { $$ = $1; }
    | parameters ','                    { $$ = $1; }
    ;

parameters
    : NAME                              { $$ = param_list_one($1, NULL); }
    | NAME '=' expression               { $$ = param_list_one($1, $3); }
    | parameters ',' NAME               { $$ = param_list_add($1, $3, NULL); }
    | parameters ',' NAME '=' expression { $$ = param_list_add($1, $3, $5); }
    ;

call_args
    : /* empty */                       { $$ = NULL; }
    | arg_list                          { $$ = $1; }
    | arg_list ','                      { $$ = $1; }
    ;

arg_list
    : argument                          { $$ = expr_list_one($1); }
    | arg_list ',' argument             { $$ = expr_list_add($1, $3); }
    ;

argument
    : expression                        { $$ = $1; }
    | NAME '=' expression               { $$ = expr_kwarg($1, $3); }
    ;

list_items
    : /* empty */                       { $$ = NULL; }
    | arg_list                          { $$ = $1; }
    | arg_list ','                      { $$ = $1; }
    ;

opt_expression
    : /* empty */                       { $$ = NULL; }
    | expression                        { $$ = $1; }
    ;

expression
    : INTEGER                           { $$ = expr_const($1); }
    | FLOAT                             { $$ = expr_const($1); }
    | STRING                            { $$ = expr_const(value_string($1)); free($1); }
    | TRUE_VALUE                        { $$ = expr_const(value_bool(true)); }
    | FALSE_VALUE                       { $$ = expr_const(value_bool(false)); }
    | NONE_VALUE                        { $$ = expr_const(value_none()); }
    | NAME                              { $$ = expr_name($1); }
    | '(' expression ')'                { $$ = $2; }
    | '[' list_items ']'                { $$ = expr_list($2); }
    | expression '(' call_args ')'      { $$ = expr_call($1, $3); }
    | expression '.' NAME '(' call_args ')' { $$ = expr_method($1, $3, $5); }
    | expression '[' expression ']'     { $$ = expr_index($1, $3); }
    | expression '[' opt_expression ':' opt_expression ']'
                                        { $$ = expr_slice($1, $3, $5, NULL); }
    | expression '[' opt_expression ':' opt_expression ':' opt_expression ']'
                                        { $$ = expr_slice($1, $3, $5, $7); }
    | '-' expression %prec UMINUS       { $$ = expr_unary(OP_NEG, $2); }
    | '+' expression %prec UMINUS       { $$ = expr_unary(OP_POS, $2); }
    | NOT expression                    { $$ = expr_unary(OP_NOT, $2); }
    | expression '+' expression         { $$ = expr_binary(OP_ADD, $1, $3); }
    | expression '-' expression         { $$ = expr_binary(OP_SUB, $1, $3); }
    | expression '*' expression         { $$ = expr_binary(OP_MUL, $1, $3); }
    | expression '/' expression         { $$ = expr_binary(OP_DIV, $1, $3); }
    | expression FLOORDIV expression    { $$ = expr_binary(OP_FLOORDIV, $1, $3); }
    | expression '%' expression         { $$ = expr_binary(OP_MOD, $1, $3); }
    | expression POW expression         { $$ = expr_binary(OP_POW, $1, $3); }
    | expression EQ expression          { $$ = expr_compare(OP_EQ, $1, $3); }
    | expression NE expression          { $$ = expr_compare(OP_NE, $1, $3); }
    | expression '<' expression         { $$ = expr_compare(OP_LT, $1, $3); }
    | expression '>' expression         { $$ = expr_compare(OP_GT, $1, $3); }
    | expression LE expression          { $$ = expr_compare(OP_LE, $1, $3); }
    | expression GE expression          { $$ = expr_compare(OP_GE, $1, $3); }
    | expression IN expression          { $$ = expr_compare(OP_IN, $1, $3); }
    | expression NOTIN expression       { $$ = expr_compare(OP_NOTIN, $1, $3); }
    | expression IS expression          { $$ = expr_compare(OP_IS, $1, $3); }
    | expression ISNOT expression       { $$ = expr_compare(OP_ISNOT, $1, $3); }
    | expression AND expression         { $$ = expr_logical(OP_AND, $1, $3); }
    | expression OR expression          { $$ = expr_logical(OP_OR, $1, $3); }
    ;

%%

void yyerror(const char *message) {
    extern YYLTYPE yylloc;
    if (lexer_reported_error) return;   /* the lexer already explained the problem */
    fflush(stdout);
    fprintf(stderr, "ShakilScript error (line %d): SyntaxError: %s\n", yylloc.first_line, message);
}
