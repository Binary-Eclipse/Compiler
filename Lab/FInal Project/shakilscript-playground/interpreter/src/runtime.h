#ifndef SHAKILSCRIPT_RUNTIME_H
#define SHAKILSCRIPT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    VALUE_NONE, VALUE_BOOL, VALUE_INT, VALUE_FLOAT, VALUE_STRING, VALUE_LIST, VALUE_FUNCTION
} ValueType;

typedef struct Value Value;
typedef struct List List;
typedef struct Function Function;
typedef struct Expr Expr;
typedef struct ExprList ExprList;
typedef struct StringList StringList;
typedef struct ParamList ParamList;
typedef struct Stmt Stmt;

struct Value {
    ValueType type;
    union {
        bool boolean;
        long long integer;
        double floating;
        char *string;
        List *list;
        Function *function;
    } as;
};
struct List { Value *items; size_t length; size_t capacity; };

/* Operator codes shared by the parser and the runtime */
enum {
    OP_ADD = '+', OP_SUB = '-', OP_MUL = '*', OP_DIV = '/', OP_MOD = '%',
    OP_FLOORDIV = 1000, OP_POW,
    OP_EQ, OP_NE, OP_LT, OP_GT, OP_LE, OP_GE, OP_IN, OP_NOTIN, OP_IS, OP_ISNOT,
    OP_AND, OP_OR, OP_NOT, OP_NEG, OP_POS
};

/* ---- values ---- */
Value value_none(void);
Value value_bool(bool b);
Value value_int(long long n);
Value value_float(double n);
Value value_string(const char *s);
Value value_list(void);

/* ---- expressions ---- */
Expr *expr_const(Value v);
Expr *expr_name(char *name);
Expr *expr_unary(int op, Expr *a);
Expr *expr_binary(int op, Expr *a, Expr *b);
Expr *expr_compare(int op, Expr *a, Expr *b);
Expr *expr_logical(int op, Expr *a, Expr *b);
Expr *expr_call(Expr *callee, ExprList *args);
Expr *expr_method(Expr *object, char *name, ExprList *args);
Expr *expr_index(Expr *object, Expr *index);
Expr *expr_slice(Expr *object, Expr *start, Expr *stop, Expr *step);
Expr *expr_list(ExprList *items);
Expr *expr_kwarg(char *name, Expr *value);
bool expr_is_target(const Expr *e);

ExprList *expr_list_one(Expr *e);
ExprList *expr_list_add(ExprList *list, Expr *e);
StringList *string_list_one(char *s);
StringList *string_list_add(StringList *list, char *s);
ParamList *param_list_one(char *name, Expr *default_value);
ParamList *param_list_add(ParamList *list, char *name, Expr *default_value);

/* ---- statements ---- */
Stmt *stmt_expr(Expr *e);
Stmt *stmt_assign(Expr *target, Expr *value);
Stmt *stmt_if(Expr *condition, Stmt *body, Stmt *orelse);
Stmt *stmt_while(Expr *condition, Stmt *body);
Stmt *stmt_for(char *variable, Expr *iterable, Stmt *body);
Stmt *stmt_def(char *name, ParamList *params, Stmt *body);
Stmt *stmt_return(Expr *e);
Stmt *stmt_break(void);
Stmt *stmt_continue(void);
Stmt *stmt_pass(void);
Stmt *stmt_global(StringList *names);
Stmt *stmt_block_add(Stmt *list, Stmt *item);
Stmt *stmt_set_line(Stmt *s, int line);

/* ---- program ---- */
void program_set(Stmt *list);
int runtime_run(int check_only);   /* semantic check, then execute; returns exit code */

char *decode_string(const char *quoted_text);
void runtime_error(const char *message);

#endif
