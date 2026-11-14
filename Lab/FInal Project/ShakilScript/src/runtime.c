/*
 * ShakilScript runtime: AST construction, semantic checks and a tree-walking
 * interpreter for the 25 functions documented in manual.txt.
 */
#include "runtime.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Data structures                                                     */
/* ------------------------------------------------------------------ */

struct Expr     { int kind, op; Value *value; char *name; Expr *left, *right; ExprList *args; };
struct ExprList { Expr *expr; ExprList *next; };
struct StringList { char *name; StringList *next; };
struct Stmt     { int kind, control, line; char *name; Expr *expr; Expr *condition; StringList *params; Stmt *body, *next; };

typedef struct Variable { char *name; Value *value; struct Variable *next; } Variable;
typedef struct Function { char *name; StringList *params; Stmt *body; struct Function *next; } Function;

enum { EX_VALUE, EX_VARIABLE, EX_BINARY, EX_CALL, EX_LIST };
enum { ST_PRINT = 1, ST_CREATE, ST_UPDATE, ST_EXPR, ST_WHEN, ST_REPEAT, ST_CONTROL, ST_FUNCTION };
enum { CTRL_STOP = 1, CTRL_SKIP, CTRL_RETURN };

#define MAX_CALL_DEPTH 1000

static Stmt *program = NULL, **program_tail = &program;
static Variable *variables = NULL;        /* newest first; shared by all scopes */
static Variable *scope_boundary = NULL;   /* variables below this belong to callers */
static Function *functions = NULL;
static int function_depth = 0;
static int loop_signal = 0;               /* CTRL_STOP / CTRL_SKIP while unwinding a loop */
static int returning = 0;
static Value *return_value = NULL;
static int current_line = 0;              /* line of the statement being executed */

/* ------------------------------------------------------------------ */
/* Errors                                                              */
/* ------------------------------------------------------------------ */

void runtime_error(const char *m) {
    fflush(stdout);
    if (current_line > 0) fprintf(stderr, "ShakilScript runtime error at line %d: %s\n", current_line, m);
    else                  fprintf(stderr, "ShakilScript runtime error: %s\n", m);
    exit(EXIT_FAILURE);
}

static void runtime_errorf(const char *fmt, ...) {
    char buffer[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, ap);
    va_end(ap);
    runtime_error(buffer);
}

static void semantic_error(int line, const char *m) {
    fflush(stdout);
    fprintf(stderr, "ShakilScript semantic error at line %d: %s\n", line, m);
    exit(EXIT_FAILURE);
}

static void *xcalloc(size_t n, size_t size) {
    void *p = calloc(n ? n : 1, size);
    if (!p) { fprintf(stderr, "ShakilScript: out of memory\n"); exit(EXIT_FAILURE); }
    return p;
}

static char *dupstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = xcalloc(n, 1);
    memcpy(p, s, n);
    return p;
}

/* ------------------------------------------------------------------ */
/* Values                                                              */
/* ------------------------------------------------------------------ */

static Value *alloc_value(ValueType t) { Value *v = xcalloc(1, sizeof *v); v->type = t; return v; }
Value *value_int(long long n)    { Value *v = alloc_value(VALUE_INT);    v->as.integer = n;  return v; }
Value *value_float(double n)     { Value *v = alloc_value(VALUE_FLOAT);  v->as.floating = n; return v; }
Value *value_string(const char *s) { Value *v = alloc_value(VALUE_STRING); v->as.string = dupstr(s); return v; }
Value *value_bool(bool b)        { Value *v = alloc_value(VALUE_BOOL);   v->as.boolean = b;  return v; }
Value *value_list(void)          { return alloc_value(VALUE_LIST); }

static void list_push(Value *l, Value *item) {
    if (l->as.list.length == l->as.list.capacity) {
        size_t cap = l->as.list.capacity ? l->as.list.capacity * 2 : 4;
        Value **p = realloc(l->as.list.items, cap * sizeof(Value *));
        if (!p) runtime_error("out of memory");
        l->as.list.items = p;
        l->as.list.capacity = cap;
    }
    l->as.list.items[l->as.list.length++] = item;
}

static const char *type_name(const Value *v) {
    switch (v->type) {
        case VALUE_INT: return "integer";
        case VALUE_FLOAT: return "float";
        case VALUE_STRING: return "string";
        case VALUE_BOOL: return "boolean";
        case VALUE_LIST: return "list";
    }
    return "unknown";
}

static bool numeric(const Value *v) { return v->type == VALUE_INT || v->type == VALUE_FLOAT; }
static double num(const Value *v) {
    if (v->type == VALUE_INT) return (double)v->as.integer;
    if (v->type == VALUE_FLOAT) return v->as.floating;
    runtime_error("expected a number");
    return 0;
}

/* ------------------------------------------------------------------ */
/* AST construction (called from the Bison grammar)                    */
/* ------------------------------------------------------------------ */

static Expr *new_expr(int k) { Expr *e = xcalloc(1, sizeof *e); e->kind = k; return e; }
Expr *expr_value(Value *v)               { Expr *e = new_expr(EX_VALUE);    e->value = v; return e; }
Expr *expr_variable(char *n)             { Expr *e = new_expr(EX_VARIABLE); e->name = n; return e; }
Expr *expr_binary(int op, Expr *a, Expr *b) { Expr *e = new_expr(EX_BINARY); e->op = op; e->left = a; e->right = b; return e; }
Expr *expr_call(char *n, ExprList *a)    { Expr *e = new_expr(EX_CALL);     e->name = n; e->args = a; return e; }
Expr *expr_list(ExprList *a)             { Expr *e = new_expr(EX_LIST);     e->args = a; return e; }

ExprList *expr_list_one(Expr *e) { ExprList *l = xcalloc(1, sizeof *l); l->expr = e; return l; }
ExprList *expr_list_add(ExprList *l, Expr *e) {
    if (!l) return expr_list_one(e);
    ExprList *p = l;
    while (p->next) p = p->next;
    p->next = expr_list_one(e);
    return l;
}
ExprList *expr_list_prepend(Expr *e, ExprList *l) { ExprList *n = expr_list_one(e); n->next = l; return n; }

StringList *string_list_one(char *s) { StringList *l = xcalloc(1, sizeof *l); l->name = s; return l; }
StringList *string_list_add(StringList *l, char *s) {
    if (!l) return string_list_one(s);
    StringList *p = l;
    while (p->next) p = p->next;
    p->next = string_list_one(s);
    return l;
}

static Stmt *new_stmt(int k) { Stmt *s = xcalloc(1, sizeof *s); s->kind = k; return s; }
Stmt *stmt_print(Expr *e)                { Stmt *s = new_stmt(ST_PRINT);    s->expr = e; return s; }
Stmt *stmt_create(char *n, Expr *e)      { Stmt *s = new_stmt(ST_CREATE);   s->name = n; s->expr = e; return s; }
Stmt *stmt_update(char *n, Expr *e)      { Stmt *s = new_stmt(ST_UPDATE);   s->name = n; s->expr = e; return s; }
Stmt *stmt_expr(Expr *e)                 { Stmt *s = new_stmt(ST_EXPR);     s->expr = e; return s; }
Stmt *stmt_when(Expr *c, Stmt *b)        { Stmt *s = new_stmt(ST_WHEN);     s->condition = c; s->body = b; return s; }
Stmt *stmt_repeat(Expr *c, Stmt *b)      { Stmt *s = new_stmt(ST_REPEAT);   s->expr = c; s->body = b; return s; }
Stmt *stmt_control(int k, Expr *e)       { Stmt *s = new_stmt(ST_CONTROL);  s->control = k; s->expr = e; return s; }
Stmt *stmt_function(char *n, StringList *p, Stmt *b) { Stmt *s = new_stmt(ST_FUNCTION); s->name = n; s->params = p; s->body = b; return s; }
Stmt *stmt_at(Stmt *s, int line)         { if (s) s->line = line; return s; }
Stmt *stmt_block_add(Stmt *l, Stmt *i) {
    if (!l) return i;
    Stmt *p = l;
    while (p->next) p = p->next;
    p->next = i;
    return l;
}
void program_add(Stmt *s)      { if (!s) return; *program_tail = s; while (*program_tail) program_tail = &(*program_tail)->next; }
void program_add_list(Stmt *l) { program_add(l); }

/* Turns the quoted lexeme from the lexer into the string's real contents. */
char *decode_string(const char *q) {
    size_t n = strlen(q);
    char *r = xcalloc(n + 1, 1);
    size_t j = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        if (q[i] == '\\' && i + 2 < n) {
            i++;
            switch (q[i]) {
                case 'n': r[j++] = '\n'; break;
                case 't': r[j++] = '\t'; break;
                case 'r': r[j++] = '\r'; break;
                default:  r[j++] = q[i];
            }
        } else r[j++] = q[i];
    }
    r[j] = 0;
    return r;
}

/* ------------------------------------------------------------------ */
/* Output                                                              */
/* ------------------------------------------------------------------ */

static void print_inline(const Value *v) {
    switch (v->type) {
        case VALUE_INT:    printf("%lld", v->as.integer); break;
        case VALUE_FLOAT:  printf("%.15g", v->as.floating); break;
        case VALUE_STRING: printf("%s", v->as.string); break;
        case VALUE_BOOL:   printf("%s", v->as.boolean ? "true" : "false"); break;
        case VALUE_LIST:
            printf("[");
            for (size_t i = 0; i < v->as.list.length; i++) {
                if (i) printf(", ");
                print_inline(v->as.list.items[i]);
            }
            printf("]");
            break;
    }
}
static void print_value(const Value *v) { print_inline(v); putchar('\n'); }

/* ------------------------------------------------------------------ */
/* Variables                                                           */
/* ------------------------------------------------------------------ */

static Variable *find_var(const char *n) {
    for (Variable *v = variables; v; v = v->next) if (!strcmp(v->name, n)) return v;
    return NULL;
}
static void declare_var(const char *n, Value *v) {
    for (Variable *p = variables; p != scope_boundary; p = p->next)
        if (!strcmp(p->name, n))
            runtime_errorf("variable '%s' already exists in this scope; use update() to change it", n);
    Variable *x = xcalloc(1, sizeof *x);
    x->name = dupstr(n);
    x->value = v;
    x->next = variables;
    variables = x;
}
static Value *get_var(const char *n) {
    Variable *v = find_var(n);
    if (!v) runtime_errorf("undefined variable '%s'", n);
    return v->value;
}
static void update_var(const char *n, Value *v) {
    Variable *x = find_var(n);
    if (!x) runtime_errorf("cannot update undefined variable '%s'; create it with create() first", n);
    x->value = v;
}

/* ------------------------------------------------------------------ */
/* Comparison helpers                                                  */
/* ------------------------------------------------------------------ */

static bool truth(const Value *v) {
    switch (v->type) {
        case VALUE_BOOL:   return v->as.boolean;
        case VALUE_INT:    return v->as.integer != 0;
        case VALUE_FLOAT:  return v->as.floating != 0;
        case VALUE_STRING: return v->as.string[0] != 0;
        default: break;
    }
    runtime_error("a list cannot be used as a condition");
    return false;
}

static bool equal_values(const Value *a, const Value *b) {
    if (numeric(a) && numeric(b)) return num(a) == num(b);
    if (a->type != b->type) return false;
    switch (a->type) {
        case VALUE_STRING: return !strcmp(a->as.string, b->as.string);
        case VALUE_BOOL:   return a->as.boolean == b->as.boolean;
        case VALUE_LIST:
            if (a->as.list.length != b->as.list.length) return false;
            for (size_t i = 0; i < a->as.list.length; i++)
                if (!equal_values(a->as.list.items[i], b->as.list.items[i])) return false;
            return true;
        default: return false;
    }
}

static int compare_order(const Value *a, const Value *b) {
    if (numeric(a) && numeric(b)) { double x = num(a), y = num(b); return x < y ? -1 : x > y ? 1 : 0; }
    if (a->type == VALUE_STRING && b->type == VALUE_STRING) return strcmp(a->as.string, b->as.string);
    runtime_errorf("cannot order a %s and a %s (only two numbers or two strings can be ordered)", type_name(a), type_name(b));
    return 0;
}

/* ------------------------------------------------------------------ */
/* Arithmetic (shared by add()/subtract()/... and the + - * / operators) */
/* ------------------------------------------------------------------ */

static Value *float_result(double d, const char *who) {
    if (!isfinite(d)) runtime_errorf("%s: floating-point overflow", who);
    return value_float(d);
}

static Value *arith(char op, const Value *a, const Value *b, const char *who) {
    if (op == '%') {
        if (a->type != VALUE_INT || b->type != VALUE_INT) runtime_errorf("%s requires two integers", who);
        if (b->as.integer == 0) runtime_errorf("%s: modulo by zero", who);
        if (b->as.integer == -1) return value_int(0);
        return value_int(a->as.integer % b->as.integer);
    }
    if (!numeric(a) || !numeric(b))
        runtime_errorf("%s requires numbers (got %s and %s)", who, type_name(a), type_name(b));
    bool ints = a->type == VALUE_INT && b->type == VALUE_INT;
    long long r;
    switch (op) {
        case '+':
            if (!ints) return float_result(num(a) + num(b), who);
            if (__builtin_add_overflow(a->as.integer, b->as.integer, &r)) runtime_errorf("%s: integer overflow", who);
            return value_int(r);
        case '-':
            if (!ints) return float_result(num(a) - num(b), who);
            if (__builtin_sub_overflow(a->as.integer, b->as.integer, &r)) runtime_errorf("%s: integer overflow", who);
            return value_int(r);
        case '*':
            if (!ints) return float_result(num(a) * num(b), who);
            if (__builtin_mul_overflow(a->as.integer, b->as.integer, &r)) runtime_errorf("%s: integer overflow", who);
            return value_int(r);
        case '/':
            if (num(b) == 0) runtime_errorf("%s: division by zero", who);
            /* An exact integer division stays an integer (20/5 -> 4), otherwise the result is a float. */
            if (ints && !(a->as.integer == LLONG_MIN && b->as.integer == -1) && a->as.integer % b->as.integer == 0)
                return value_int(a->as.integer / b->as.integer);
            return float_result(num(a) / num(b), who);
    }
    runtime_error("internal error: unknown arithmetic operator");
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Evaluation                                                          */
/* ------------------------------------------------------------------ */

static Value *eval(Expr *e);
static void execute_block(Stmt *s);

static Value **args_eval(ExprList *l, size_t *n) {
    *n = 0;
    for (ExprList *p = l; p; p = p->next) (*n)++;
    Value **a = xcalloc(*n, sizeof *a);
    size_t i = 0;
    for (ExprList *p = l; p; p = p->next) a[i++] = eval(p->expr);
    return a;
}

/* ---- user-defined functions ---- */

static Value *call_user(const char *name, Value **args, size_t argc) {
    Function *f = functions;
    while (f && strcmp(f->name, name)) f = f->next;
    if (!f) runtime_errorf("unknown function '%s' in call() (define it with function before calling it)", name);

    size_t expected = 0;
    for (StringList *p = f->params; p; p = p->next) expected++;
    if (argc != expected)
        runtime_errorf("function '%s' expects %zu argument(s) but call() gave %zu", name, expected, argc);
    if (function_depth >= MAX_CALL_DEPTH)
        runtime_errorf("maximum call depth (%d) exceeded in function '%s' (infinite recursion?)", MAX_CALL_DEPTH, name);

    Variable *saved = variables;
    Variable *old_boundary = scope_boundary;
    int old_return = returning, old_line = current_line;
    Value *old_value = return_value;

    scope_boundary = saved;              /* the function gets a fresh local scope */
    function_depth++;
    size_t i = 0;
    for (StringList *p = f->params; p; p = p->next) declare_var(p->name, args[i++]);
    returning = 0;
    return_value = value_int(0);         /* value of a function that never calls return() */

    execute_block(f->body);

    Value *result = return_value;
    while (variables != saved) { Variable *x = variables; variables = x->next; free(x->name); free(x); }
    scope_boundary = old_boundary;
    function_depth--;
    returning = old_return;
    return_value = old_value;
    current_line = old_line;
    return result;
}

/* ---- input() ---- */

static Value *builtin_input(const Value *prompt) {
    print_inline(prompt);
    fflush(stdout);
    size_t cap = 128, len = 0;
    char *buf = xcalloc(cap, 1);
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (len + 2 > cap) {
            cap *= 2;
            char *p = realloc(buf, cap);
            if (!p) runtime_error("out of memory");
            buf = p;
        }
        buf[len++] = (char)c;
    }
    if (c == EOF && len == 0) runtime_error("input() could not read a line (end of input)");
    if (len && buf[len - 1] == '\r') len--;
    buf[len] = 0;
    Value *v = value_string(buf);
    free(buf);
    return v;
}

/* ---- sort() : stable merge sort ---- */

static void merge_sort(Value **items, Value **tmp, size_t n) {
    if (n < 2) return;
    size_t mid = n / 2;
    merge_sort(items, tmp, mid);
    merge_sort(items + mid, tmp, n - mid);
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < n) tmp[k++] = compare_order(items[j], items[i]) < 0 ? items[j++] : items[i++];
    while (i < mid) tmp[k++] = items[i++];
    while (j < n)   tmp[k++] = items[j++];
    memcpy(items, tmp, n * sizeof(Value *));
}

/* ---- convert() ---- */

static const char *trim_bounds(const char *s, size_t *len) {
    while (isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) n--;
    *len = n;
    return s;
}

static bool parse_int_text(const char *text, long long *out) {
    size_t n; const char *s = trim_bounds(text, &n);
    if (n == 0) return false;
    char *copy = xcalloc(n + 1, 1);
    memcpy(copy, s, n);
    size_t i = (copy[0] == '+' || copy[0] == '-') ? 1 : 0;
    bool ok = i < n;
    for (size_t k = i; k < n && ok; k++) if (!isdigit((unsigned char)copy[k])) ok = false;
    if (ok) { errno = 0; *out = strtoll(copy, NULL, 10); if (errno == ERANGE) ok = false; }
    free(copy);
    return ok;
}

static bool parse_float_text(const char *text, double *out) {
    size_t n; const char *s = trim_bounds(text, &n);
    if (n == 0) return false;
    char *copy = xcalloc(n + 1, 1);
    memcpy(copy, s, n);
    bool ok = true;
    for (size_t k = 0; k < n && ok; k++) if (!strchr("0123456789+-.eE", copy[k])) ok = false;   /* no nan/inf/hex */
    if (ok) {
        char *end = NULL;
        *out = strtod(copy, &end);
        if (end == copy || *end || !isfinite(*out)) ok = false;
    }
    free(copy);
    return ok;
}

static Value *builtin_convert(Value **a) {
    if (a[1]->type != VALUE_STRING) runtime_error("convert() target type must be a string such as \"integer\"");
    const char *t = a[1]->as.string;
    const Value *v = a[0];

    if (!strcmp(t, "string")) {
        char buf[64];
        switch (v->type) {
            case VALUE_STRING: return value_string(v->as.string);
            case VALUE_INT:    snprintf(buf, sizeof buf, "%lld", v->as.integer); return value_string(buf);
            case VALUE_FLOAT:  snprintf(buf, sizeof buf, "%.15g", v->as.floating); return value_string(buf);
            case VALUE_BOOL:   return value_string(v->as.boolean ? "true" : "false");
            default: runtime_error("convert(): a list cannot be converted to string");
        }
    }
    if (!strcmp(t, "integer") || !strcmp(t, "int")) {
        long long n = 0;
        switch (v->type) {
            case VALUE_INT:  return value_int(v->as.integer);
            case VALUE_BOOL: return value_int(v->as.boolean ? 1 : 0);
            case VALUE_FLOAT:
                if (!isfinite(v->as.floating) || v->as.floating >= 9223372036854775808.0 || v->as.floating < -9223372036854775808.0)
                    runtime_error("convert(): float is out of the integer range");
                return value_int((long long)v->as.floating);      /* truncates toward zero */
            case VALUE_STRING:
                if (!parse_int_text(v->as.string, &n))
                    runtime_errorf("convert(): \"%s\" is not a valid integer", v->as.string);
                return value_int(n);
            default: runtime_error("convert(): a list cannot be converted to integer");
        }
    }
    if (!strcmp(t, "float")) {
        double d = 0;
        switch (v->type) {
            case VALUE_INT:   return value_float((double)v->as.integer);
            case VALUE_FLOAT: return value_float(v->as.floating);
            case VALUE_BOOL:  return value_float(v->as.boolean ? 1.0 : 0.0);
            case VALUE_STRING:
                if (!parse_float_text(v->as.string, &d))
                    runtime_errorf("convert(): \"%s\" is not a valid float", v->as.string);
                return value_float(d);
            default: runtime_error("convert(): a list cannot be converted to float");
        }
    }
    if (!strcmp(t, "boolean") || !strcmp(t, "bool")) {
        if (v->type == VALUE_STRING) {
            if (!strcmp(v->as.string, "true"))  return value_bool(true);
            if (!strcmp(v->as.string, "false")) return value_bool(false);
            runtime_errorf("convert(): \"%s\" is not a valid boolean (use \"true\" or \"false\")", v->as.string);
        }
        if (v->type == VALUE_LIST) runtime_error("convert(): a list cannot be converted to boolean");
        return value_bool(truth(v));
    }
    runtime_errorf("convert(): unknown target type \"%s\" (use \"integer\", \"float\", \"string\" or \"boolean\")", t);
    return NULL;
}

/* ---- compare() ---- */

static Value *builtin_compare(Value **a) {
    if (a[1]->type != VALUE_STRING) runtime_error("compare() second argument must be an operator string such as \">\"");
    const char *op = a[1]->as.string;
    bool b;
    if (!strcmp(op, "=="))      b = equal_values(a[0], a[2]);
    else if (!strcmp(op, "!=")) b = !equal_values(a[0], a[2]);
    else {
        if (strcmp(op, "<") && strcmp(op, "<=") && strcmp(op, ">") && strcmp(op, ">="))
            runtime_errorf("unsupported compare() operator \"%s\" (use ==, !=, >, <, >= or <=)", op);
        int c = compare_order(a[0], a[2]);
        if (!strcmp(op, "<")) b = c < 0;
        else if (!strcmp(op, "<=")) b = c <= 0;
        else if (!strcmp(op, ">")) b = c > 0;
        else b = c >= 0;
    }
    return value_bool(b);
}

/* ---- collections ---- */

static Value *builtin_collection(const char *f, Value **a, size_t n, size_t expected) {
    if (n != expected) runtime_errorf("%s() expects %zu argument(s) but got %zu", f, expected, n);
    if (a[0]->type != VALUE_LIST) runtime_errorf("%s() expects a list as its first argument (got %s)", f, type_name(a[0]));
    Value *l = a[0];
    size_t len = l->as.list.length;
    Value **items = l->as.list.items;

    if (!strcmp(f, "count")) return value_int((long long)len);

    if (!strcmp(f, "at") || !strcmp(f, "remove")) {
        if (a[1]->type != VALUE_INT) runtime_errorf("%s() index must be an integer", f);
        long long ix = a[1]->as.integer;
        if (ix < 0 || (unsigned long long)ix >= len)
            runtime_errorf("%s() index %lld is out of range (the list has %zu element(s))", f, ix, len);
        if (!strcmp(f, "at")) return items[ix];
        for (size_t i = (size_t)ix; i + 1 < len; i++) items[i] = items[i + 1];
        l->as.list.length--;
        return l;
    }
    if (!strcmp(f, "find")) {
        for (size_t i = 0; i < len; i++) if (equal_values(items[i], a[1])) return value_int((long long)i);
        return value_int(-1);                                 /* not found */
    }
    if (!strcmp(f, "append")) { list_push(l, a[1]); return l; }
    if (!strcmp(f, "reverse")) {
        for (size_t i = 0; i < len / 2; i++) { Value *t = items[i]; items[i] = items[len - 1 - i]; items[len - 1 - i] = t; }
        return l;
    }
    /* sort */
    if (len > 1) {
        Value **tmp = xcalloc(len, sizeof *tmp);
        merge_sort(items, tmp, len);
        free(tmp);
    }
    return l;
}

/* ---- dispatcher ---- */

static Value *eval_call(Expr *e) {
    size_t n = 0;
    Value **a = args_eval(e->args, &n);
    const char *f = e->name;
    Value *r = NULL;
#define NEED(k) do { if (n != (k)) runtime_errorf("%s() expects %d argument(s) but got %zu", f, (int)(k), n); } while (0)

    if (!strcmp(f, "add"))           { NEED(2); r = arith('+', a[0], a[1], "add()"); }
    else if (!strcmp(f, "subtract")) { NEED(2); r = arith('-', a[0], a[1], "subtract()"); }
    else if (!strcmp(f, "multiply")) { NEED(2); r = arith('*', a[0], a[1], "multiply()"); }
    else if (!strcmp(f, "divide"))   { NEED(2); r = arith('/', a[0], a[1], "divide()"); }
    else if (!strcmp(f, "modulo"))   { NEED(2); r = arith('%', a[0], a[1], "modulo()"); }
    else if (!strcmp(f, "input"))    { NEED(1); r = builtin_input(a[0]); }
    else if (!strcmp(f, "compare"))  { NEED(3); r = builtin_compare(a); }
    else if (!strcmp(f, "convert"))  { NEED(2); r = builtin_convert(a); }
    else if (!strcmp(f, "call")) {
        if (n < 1 || a[0]->type != VALUE_STRING) runtime_error("call() first argument must be a function-name string");
        r = call_user(a[0]->as.string, a + 1, n - 1);
    }
    else if (!strcmp(f, "count") || !strcmp(f, "reverse") || !strcmp(f, "sort")) {
        if (n < 1) runtime_errorf("%s() expects a list argument", f);
        r = builtin_collection(f, a, n, 1);
    }
    else if (!strcmp(f, "at") || !strcmp(f, "find") || !strcmp(f, "append") || !strcmp(f, "remove")) {
        if (n < 1) runtime_errorf("%s() expects a list and one more argument", f);
        r = builtin_collection(f, a, n, 2);
    }
    else runtime_errorf("unknown function '%s'", f);
#undef NEED
    free(a);
    return r;
}

static Value *eval(Expr *e) {
    if (!e) return value_int(0);
    switch (e->kind) {
        case EX_VALUE:    return e->value;
        case EX_VARIABLE: return get_var(e->name);
        case EX_CALL:     return eval_call(e);
        case EX_LIST: {
            Value *v = value_list();
            for (ExprList *p = e->args; p; p = p->next) list_push(v, eval(p->expr));
            return v;
        }
        case EX_BINARY: {
            Value *a = eval(e->left), *b = eval(e->right);
            switch (e->op) {
                case '+': return arith('+', a, b, "operator +");
                case '-': return arith('-', a, b, "operator -");
                case '*': return arith('*', a, b, "operator *");
                case '/': return arith('/', a, b, "operator /");
            }
        }
    }
    return value_int(0);
}

/* ------------------------------------------------------------------ */
/* Semantic check (runs once, before anything is executed)             */
/* ------------------------------------------------------------------ */

static void check_block(Stmt *s, bool in_loop, bool in_function) {
    for (; s; s = s->next) {
        switch (s->kind) {
            case ST_WHEN:   check_block(s->body, in_loop, in_function); break;
            case ST_REPEAT: check_block(s->body, true, in_function); break;
            case ST_FUNCTION:
                for (StringList *p = s->params; p; p = p->next)
                    for (StringList *q = p->next; q; q = q->next)
                        if (!strcmp(p->name, q->name)) {
                            char msg[256];
                            snprintf(msg, sizeof msg, "function '%s' has two parameters named '%s'", s->name, p->name);
                            semantic_error(s->line, msg);
                        }
                check_block(s->body, false, true);   /* a function body starts outside any loop */
                break;
            case ST_CONTROL:
                if (s->control == CTRL_RETURN) {
                    if (!in_function) semantic_error(s->line, "return() can only be used inside a function");
                } else if (!in_loop) {
                    semantic_error(s->line, s->control == CTRL_STOP
                                   ? "stop() can only be used inside repeat()"
                                   : "skip() can only be used inside repeat()");
                }
                break;
            default: break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Execution                                                           */
/* ------------------------------------------------------------------ */

static void execute_block(Stmt *s) {
    for (; s; s = s->next) {
        if (returning || loop_signal) break;
        current_line = s->line;
        switch (s->kind) {
            case ST_PRINT:  print_value(eval(s->expr)); break;
            case ST_CREATE: declare_var(s->name, eval(s->expr)); break;
            case ST_UPDATE: update_var(s->name, eval(s->expr)); break;
            case ST_EXPR:   (void)eval(s->expr); break;
            case ST_WHEN:
                if (truth(eval(s->condition))) execute_block(s->body);
                break;
            case ST_REPEAT: {
                Value *v = eval(s->expr);
                if (!numeric(v) || num(v) < 0 || floor(num(v)) != num(v) || num(v) >= 9.2e18)
                    runtime_error("repeat() requires a non-negative whole number");
                long long count = (long long)num(v);
                for (long long i = 0; i < count; i++) {
                    execute_block(s->body);
                    if (loop_signal == CTRL_STOP) { loop_signal = 0; break; }
                    if (loop_signal == CTRL_SKIP) { loop_signal = 0; continue; }
                    if (returning) break;
                }
                break;
            }
            case ST_CONTROL:
                if (s->control == CTRL_RETURN) {
                    return_value = s->expr ? eval(s->expr) : value_int(0);
                    returning = 1;
                } else loop_signal = s->control;
                break;
            case ST_FUNCTION: {
                Function *f = xcalloc(1, sizeof *f);
                f->name = s->name; f->params = s->params; f->body = s->body;
                f->next = functions; functions = f;     /* a newer definition shadows an older one */
                break;
            }
        }
    }
}

void runtime_execute(void) {
    check_block(program, false, false);
    execute_block(program);
    fflush(stdout);
}
