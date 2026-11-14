#ifndef SHAKILSCRIPT_RUNTIME_H
#define SHAKILSCRIPT_RUNTIME_H
#include <stdbool.h>
#include <stddef.h>

typedef enum { VALUE_INT, VALUE_FLOAT, VALUE_STRING, VALUE_BOOL, VALUE_LIST } ValueType;
typedef struct Value Value;
typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct Stmt Stmt;
typedef struct StringList StringList;
struct Value {
    ValueType type;
    union { long long integer; double floating; char *string; bool boolean;
            struct { Value **items; size_t length; size_t capacity; } list; } as;
};

/* Values */
Value *value_int(long long n); Value *value_float(double n); Value *value_string(const char *s); Value *value_bool(bool b); Value *value_list(void);

/* Expressions */
Expr *expr_value(Value *v); Expr *expr_variable(char *name); Expr *expr_binary(int op, Expr *a, Expr *b); Expr *expr_call(char *name, ExprList *args); Expr *expr_list(ExprList *items);
ExprList *expr_list_one(Expr *e); ExprList *expr_list_add(ExprList *list, Expr *e); ExprList *expr_list_prepend(Expr *e, ExprList *list);
StringList *string_list_one(char *s); StringList *string_list_add(StringList *list, char *s);

/* Statements */
Stmt *stmt_print(Expr *e); Stmt *stmt_create(char *name, Expr *e); Stmt *stmt_update(char *name, Expr *e); Stmt *stmt_expr(Expr *e);
Stmt *stmt_when(Expr *condition, Stmt *body); Stmt *stmt_repeat(Expr *count, Stmt *body); Stmt *stmt_control(int kind, Expr *e);
Stmt *stmt_function(char *name, StringList *params, Stmt *body); Stmt *stmt_block_add(Stmt *list, Stmt *item);
Stmt *stmt_at(Stmt *s, int line);   /* records the source line of a statement (for error messages); returns s */

/* Program / execution */
void program_add(Stmt *s); void program_add_list(Stmt *list);
void runtime_execute(void);          /* semantic check, then run the program */

/* Helpers */
char *decode_string(const char *quoted_text);
void runtime_error(const char *message);
#endif
