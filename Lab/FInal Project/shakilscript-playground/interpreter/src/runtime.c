/*
 * ShakilScript runtime: AST construction, semantic check and tree-walking executor.
 * The language follows Python's syntax and semantics for the supported subset.
 */
/* MinGW: use the C99-compliant printf family (needed for %zu / %lld on Windows). */
#if defined(__MINGW32__) && !defined(__USE_MINGW_ANSI_STDIO)
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include "runtime.h"
#include "names.h"
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#if defined(__MINGW32__)
#define PRINTF_LIKE(a, b) __attribute__((format(gnu_printf, a, b)))
#else
#define PRINTF_LIKE(a, b) __attribute__((format(printf, a, b)))
#endif
#else
#define NORETURN
#define PRINTF_LIKE(a, b)
#endif

#define MAX_RECURSION 1000

/* ===================================================================== */
/*  AST                                                                  */
/* ===================================================================== */
enum { EX_CONST, EX_NAME, EX_UNARY, EX_BINARY, EX_COMPARE, EX_LOGICAL, EX_CALL, EX_METHOD,
       EX_INDEX, EX_SLICE, EX_LIST, EX_KWARG };
enum { ST_EXPR, ST_ASSIGN, ST_IF, ST_WHILE, ST_FOR, ST_DEF, ST_RETURN, ST_BREAK, ST_CONTINUE,
       ST_PASS, ST_GLOBAL };

struct ExprList   { Expr *expr; ExprList *next; };
struct StringList { char *name; StringList *next; };
struct ParamList  { char *name; Expr *default_expr; ParamList *next; };
struct Expr {
    int kind, op;
    Value value;
    char *name;
    Expr *left, *right, *extra, *extra2;
    ExprList *args;
};
struct Stmt {
    int kind, line;
    char *name;
    Expr *expr, *target;
    Stmt *body, *orelse, *next;
    ParamList *params;
    StringList *names;
};
struct Function {
    char *name;
    size_t nparams;
    char **param_names;
    Value *defaults;
    bool *has_default;
    Stmt *body;
};

static Stmt *program = NULL;
static int current_line = 0;

/* ===================================================================== */
/*  Helpers: memory, errors                                              */
/* ===================================================================== */
static void *xmalloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) { fprintf(stderr, "ShakilScript: out of memory\n"); exit(1); } return p; }
static void *xcalloc(size_t n, size_t s) { void *p = calloc(n ? n : 1, s); if (!p) { fprintf(stderr, "ShakilScript: out of memory\n"); exit(1); } return p; }
static void *xrealloc(void *q, size_t n) { void *p = realloc(q, n ? n : 1); if (!p) { fprintf(stderr, "ShakilScript: out of memory\n"); exit(1); } return p; }
static char *xstrdup(const char *s) { size_t n = strlen(s) + 1; char *p = xmalloc(n); memcpy(p, s, n); return p; }

NORETURN PRINTF_LIKE(2, 3)
static void py_error(const char *type, const char *fmt, ...) {
    va_list ap;
    fflush(stdout);
    if (current_line > 0) fprintf(stderr, "ShakilScript error (line %d): %s: ", current_line, type);
    else fprintf(stderr, "ShakilScript error: %s: ", type);
    char raw[2048];
    va_start(ap, fmt);
    vsnprintf(raw, sizeof raw, fmt, ap);
    va_end(ap);
    char *msg = ss_localize_message(raw);
    fputs(msg ? msg : raw, stderr);
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}
void runtime_error(const char *m) { py_error("RuntimeError", "%s", m); }

/* ===================================================================== */
/*  Values                                                               */
/* ===================================================================== */
Value value_none(void) { Value v; v.type = VALUE_NONE; v.as.integer = 0; return v; }
Value value_bool(bool b) { Value v; v.type = VALUE_BOOL; v.as.boolean = b; return v; }
Value value_int(long long n) { Value v; v.type = VALUE_INT; v.as.integer = n; return v; }
Value value_float(double n) { Value v; v.type = VALUE_FLOAT; v.as.floating = n; return v; }
Value value_string(const char *s) { Value v; v.type = VALUE_STRING; v.as.string = xstrdup(s); return v; }
static Value value_string_owned(char *s) { Value v; v.type = VALUE_STRING; v.as.string = s; return v; }
Value value_list(void) { Value v; v.type = VALUE_LIST; v.as.list = xcalloc(1, sizeof(List)); return v; }
static Value value_function(Function *f) { Value v; v.type = VALUE_FUNCTION; v.as.function = f; return v; }

static void list_push(List *l, Value item) {
    if (l->length == l->capacity) {
        l->capacity = l->capacity ? l->capacity * 2 : 4;
        l->items = xrealloc(l->items, l->capacity * sizeof(Value));
    }
    l->items[l->length++] = item;
}
static Value list_from_items(const Value *items, size_t n) {
    Value v = value_list();
    for (size_t i = 0; i < n; i++) list_push(v.as.list, items[i]);
    return v;
}

static const char *type_name(Value v) {
    switch (v.type) {
    case VALUE_NONE: return "NoneType";
    case VALUE_BOOL: return "bool";
    case VALUE_INT: return "int";
    case VALUE_FLOAT: return "float";
    case VALUE_STRING: return "str";
    case VALUE_LIST: return "list";
    case VALUE_FUNCTION: return "function";
    }
    return "object";
}
static bool is_intlike(Value v) { return v.type == VALUE_INT || v.type == VALUE_BOOL; }
static bool is_number(Value v) { return is_intlike(v) || v.type == VALUE_FLOAT; }
static long long as_ll(Value v) { return v.type == VALUE_BOOL ? (v.as.boolean ? 1 : 0) : v.as.integer; }
static double as_dbl(Value v) { return v.type == VALUE_FLOAT ? v.as.floating : (double)as_ll(v); }

static bool truthy(Value v) {
    switch (v.type) {
    case VALUE_NONE: return false;
    case VALUE_BOOL: return v.as.boolean;
    case VALUE_INT: return v.as.integer != 0;
    case VALUE_FLOAT: return v.as.floating != 0.0;
    case VALUE_STRING: return v.as.string[0] != 0;
    case VALUE_LIST: return v.as.list->length > 0;
    case VALUE_FUNCTION: return true;
    }
    return true;
}

/* ===================================================================== */
/*  String building and text conversion (str / repr)                     */
/* ===================================================================== */
typedef struct { char *data; size_t len, cap; } StrBuf;
static void sb_init(StrBuf *b) { b->cap = 64; b->len = 0; b->data = xmalloc(b->cap); b->data[0] = 0; }
static void sb_add_n(StrBuf *b, const char *s, size_t n) {
    if (b->len + n + 1 > b->cap) { while (b->len + n + 1 > b->cap) b->cap *= 2; b->data = xrealloc(b->data, b->cap); }
    memcpy(b->data + b->len, s, n); b->len += n; b->data[b->len] = 0;
}
static void sb_add(StrBuf *b, const char *s) { sb_add_n(b, s, strlen(s)); }
static void sb_add_char(StrBuf *b, char c) { sb_add_n(b, &c, 1); }

/* Shortest representation that round-trips, formatted like Python's repr(float). */
static char *float_repr(double x) {
    char e[64];
    if (isnan(x)) return xstrdup("nan");
    if (isinf(x)) return xstrdup(x < 0 ? "-inf" : "inf");
    if (x == 0) return xstrdup(signbit(x) ? "-0.0" : "0.0");
    for (int p = 1; p <= 17; p++) {
        snprintf(e, sizeof e, "%.*e", p - 1, x);
        if (strtod(e, NULL) == x) break;
    }
    const char *s = e; bool neg = false;
    if (*s == '-') { neg = true; s++; }
    char digits[40]; int nd = 0;
    for (; *s && *s != 'e'; s++) if (*s >= '0' && *s <= '9') digits[nd++] = *s;
    digits[nd] = 0;
    int exp10 = atoi(s + 1), decpt = exp10 + 1;
    StrBuf b; sb_init(&b);
    if (neg) sb_add_char(&b, '-');
    if (decpt <= -4 || decpt > 16) {
        sb_add_char(&b, digits[0]);
        if (nd > 1) { sb_add_char(&b, '.'); sb_add_n(&b, digits + 1, (size_t)nd - 1); }
        char ex[16]; snprintf(ex, sizeof ex, "e%c%02d", exp10 < 0 ? '-' : '+', abs(exp10)); sb_add(&b, ex);
    } else if (decpt <= 0) {
        sb_add(&b, "0.");
        for (int i = 0; i < -decpt; i++) sb_add_char(&b, '0');
        sb_add_n(&b, digits, (size_t)nd);
    } else if (decpt >= nd) {
        sb_add_n(&b, digits, (size_t)nd);
        for (int i = 0; i < decpt - nd; i++) sb_add_char(&b, '0');
        sb_add(&b, ".0");
    } else {
        sb_add_n(&b, digits, (size_t)decpt); sb_add_char(&b, '.'); sb_add_n(&b, digits + decpt, (size_t)(nd - decpt));
    }
    return b.data;
}

static void append_repr_string(StrBuf *b, const char *s) {
    char q = (strchr(s, '\'') && !strchr(s, '"')) ? '"' : '\'';
    sb_add_char(b, q);
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '\\') sb_add(b, "\\\\");
        else if (*p == (unsigned char)q) { sb_add_char(b, '\\'); sb_add_char(b, q); }
        else if (*p == '\n') sb_add(b, "\\n");
        else if (*p == '\t') sb_add(b, "\\t");
        else if (*p == '\r') sb_add(b, "\\r");
        else if (*p < 32 || *p == 127) { char t[8]; snprintf(t, sizeof t, "\\x%02x", *p); sb_add(b, t); }
        else sb_add_char(b, (char)*p);
    }
    sb_add_char(b, q);
}
static void append_str(StrBuf *b, Value v);
static void append_repr(StrBuf *b, Value v) {
    if (v.type == VALUE_STRING) append_repr_string(b, v.as.string);
    else append_str(b, v);
}
static void append_str(StrBuf *b, Value v) {
    char tmp[64];
    switch (v.type) {
    case VALUE_NONE: sb_add(b, "None"); break;
    case VALUE_BOOL: sb_add(b, v.as.boolean ? "True" : "False"); break;
    case VALUE_INT: snprintf(tmp, sizeof tmp, "%lld", v.as.integer); sb_add(b, tmp); break;
    case VALUE_FLOAT: { char *f = float_repr(v.as.floating); sb_add(b, f); free(f); break; }
    case VALUE_STRING: sb_add(b, v.as.string); break;
    case VALUE_LIST:
        sb_add_char(b, '[');
        for (size_t i = 0; i < v.as.list->length; i++) { if (i) sb_add(b, ", "); append_repr(b, v.as.list->items[i]); }
        sb_add_char(b, ']');
        break;
    case VALUE_FUNCTION: sb_add(b, "<function "); sb_add(b, v.as.function->name); sb_add_char(b, '>'); break;
    }
}
static char *repr_of(Value v) { StrBuf b; sb_init(&b); append_repr(&b, v); return b.data; }

char *decode_string(const char *q) {
    size_t n = strlen(q);
    char *r = xmalloc(n + 1);
    size_t j = 0;
    for (size_t i = 1; i + 1 < n; i++) {
        if (q[i] == '\\' && i + 2 < n) {
            char c = q[++i];
            switch (c) {
            case 'n': r[j++] = '\n'; break;
            case 't': r[j++] = '\t'; break;
            case 'r': r[j++] = '\r'; break;
            case '0': r[j++] = '\0'; break;
            case '\\': r[j++] = '\\'; break;
            case '\'': r[j++] = '\''; break;
            case '"': r[j++] = '"'; break;
            default: r[j++] = '\\'; r[j++] = c; break;
            }
        } else r[j++] = q[i];
    }
    r[j] = 0;
    return r;
}

/* ===================================================================== */
/*  AST constructors                                                     */
/* ===================================================================== */
static Expr *new_expr(int kind) { Expr *e = xcalloc(1, sizeof *e); e->kind = kind; return e; }
Expr *expr_const(Value v) { Expr *e = new_expr(EX_CONST); e->value = v; return e; }
Expr *expr_name(char *name) { Expr *e = new_expr(EX_NAME); e->name = name; return e; }
Expr *expr_unary(int op, Expr *a) { Expr *e = new_expr(EX_UNARY); e->op = op; e->left = a; return e; }
Expr *expr_binary(int op, Expr *a, Expr *b) { Expr *e = new_expr(EX_BINARY); e->op = op; e->left = a; e->right = b; return e; }
Expr *expr_compare(int op, Expr *a, Expr *b) { Expr *e = new_expr(EX_COMPARE); e->op = op; e->left = a; e->right = b; return e; }
Expr *expr_logical(int op, Expr *a, Expr *b) { Expr *e = new_expr(EX_LOGICAL); e->op = op; e->left = a; e->right = b; return e; }
Expr *expr_call(Expr *callee, ExprList *args) { Expr *e = new_expr(EX_CALL); e->left = callee; e->args = args; return e; }
Expr *expr_method(Expr *obj, char *name, ExprList *args) { Expr *e = new_expr(EX_METHOD); e->left = obj; e->name = name; e->args = args; return e; }
Expr *expr_index(Expr *obj, Expr *index) { Expr *e = new_expr(EX_INDEX); e->left = obj; e->right = index; return e; }
Expr *expr_slice(Expr *obj, Expr *a, Expr *b, Expr *c) { Expr *e = new_expr(EX_SLICE); e->left = obj; e->right = a; e->extra = b; e->extra2 = c; return e; }
Expr *expr_list(ExprList *items) { Expr *e = new_expr(EX_LIST); e->args = items; return e; }
Expr *expr_kwarg(char *name, Expr *value) { Expr *e = new_expr(EX_KWARG); e->name = name; e->left = value; return e; }
bool expr_is_target(const Expr *e) { return e->kind == EX_NAME || e->kind == EX_INDEX; }

ExprList *expr_list_one(Expr *e) { ExprList *l = xcalloc(1, sizeof *l); l->expr = e; return l; }
ExprList *expr_list_add(ExprList *l, Expr *e) { ExprList *p = l; while (p->next) p = p->next; p->next = expr_list_one(e); return l; }
StringList *string_list_one(char *s) { StringList *l = xcalloc(1, sizeof *l); l->name = s; return l; }
StringList *string_list_add(StringList *l, char *s) { StringList *p = l; while (p->next) p = p->next; p->next = string_list_one(s); return l; }
ParamList *param_list_one(char *name, Expr *d) { ParamList *l = xcalloc(1, sizeof *l); l->name = name; l->default_expr = d; return l; }
ParamList *param_list_add(ParamList *l, char *name, Expr *d) { ParamList *p = l; while (p->next) p = p->next; p->next = param_list_one(name, d); return l; }

static Stmt *new_stmt(int kind) { Stmt *s = xcalloc(1, sizeof *s); s->kind = kind; return s; }
Stmt *stmt_expr(Expr *e) { Stmt *s = new_stmt(ST_EXPR); s->expr = e; return s; }
Stmt *stmt_assign(Expr *t, Expr *v) { Stmt *s = new_stmt(ST_ASSIGN); s->target = t; s->expr = v; return s; }
Stmt *stmt_if(Expr *c, Stmt *b, Stmt *o) { Stmt *s = new_stmt(ST_IF); s->expr = c; s->body = b; s->orelse = o; return s; }
Stmt *stmt_while(Expr *c, Stmt *b) { Stmt *s = new_stmt(ST_WHILE); s->expr = c; s->body = b; return s; }
Stmt *stmt_for(char *v, Expr *it, Stmt *b) { Stmt *s = new_stmt(ST_FOR); s->name = v; s->expr = it; s->body = b; return s; }
Stmt *stmt_def(char *n, ParamList *p, Stmt *b) { Stmt *s = new_stmt(ST_DEF); s->name = n; s->params = p; s->body = b; return s; }
Stmt *stmt_return(Expr *e) { Stmt *s = new_stmt(ST_RETURN); s->expr = e; return s; }
Stmt *stmt_break(void) { return new_stmt(ST_BREAK); }
Stmt *stmt_continue(void) { return new_stmt(ST_CONTINUE); }
Stmt *stmt_pass(void) { return new_stmt(ST_PASS); }
Stmt *stmt_global(StringList *n) { Stmt *s = new_stmt(ST_GLOBAL); s->names = n; return s; }
Stmt *stmt_block_add(Stmt *l, Stmt *i) { if (!l) return i; Stmt *p = l; while (p->next) p = p->next; p->next = i; return l; }
Stmt *stmt_set_line(Stmt *s, int line) { s->line = line; return s; }
void program_set(Stmt *list) { program = list; }

/* ===================================================================== */
/*  Variables and scopes (Python rules: functions get a local scope)     */
/* ===================================================================== */
typedef struct Variable { const char *name; Value value; struct Variable *next; } Variable;
typedef struct Scope { Variable *vars; StringList *globals; } Scope;

static Variable *global_vars = NULL;
static Scope *cur_scope = NULL;     /* NULL while running module-level code */
static int call_depth = 0;

static Variable *find_in(Variable *v, const char *n) {
    for (; v; v = v->next) if (!strcmp(v->name, n)) return v;
    return NULL;
}
static bool declared_global(const char *n) {
    if (!cur_scope) return true;
    for (StringList *g = cur_scope->globals; g; g = g->next) if (!strcmp(g->name, n)) return true;
    return false;
}
static Variable *lookup_var(const char *n) {
    if (cur_scope && !declared_global(n)) {
        Variable *v = find_in(cur_scope->vars, n);
        if (v) return v;
    }
    return find_in(global_vars, n);
}
static void assign_var(const char *n, Value val) {
    Variable **head = declared_global(n) ? &global_vars : &cur_scope->vars;
    Variable *v = find_in(*head, n);
    if (v) { v->value = val; return; }
    v = xcalloc(1, sizeof *v);
    v->name = n; v->value = val; v->next = *head; *head = v;
}

/* ===================================================================== */
/*  Arithmetic, comparison                                               */
/* ===================================================================== */
static const char *op_symbol(int op) {
    switch (op) {
    case OP_ADD: return "+"; case OP_SUB: return "-"; case OP_MUL: return "*"; case OP_DIV: return "/";
    case OP_MOD: return "%"; case OP_FLOORDIV: return "//"; case OP_POW: return "**";
    case OP_LT: return "<"; case OP_GT: return ">"; case OP_LE: return "<="; case OP_GE: return ">=";
    }
    return "?";
}
static long long checked_add(long long a, long long b) { long long r; if (__builtin_add_overflow(a, b, &r)) py_error("OverflowError", "integer overflow (ShakilScript integers are 64-bit)"); return r; }
static long long checked_sub(long long a, long long b) { long long r; if (__builtin_sub_overflow(a, b, &r)) py_error("OverflowError", "integer overflow (ShakilScript integers are 64-bit)"); return r; }
static long long checked_mul(long long a, long long b) { long long r; if (__builtin_mul_overflow(a, b, &r)) py_error("OverflowError", "integer overflow (ShakilScript integers are 64-bit)"); return r; }

static Value repeat_sequence(Value seq, long long n) {
    if (n < 0) n = 0;
    if (seq.type == VALUE_STRING) {
        size_t len = strlen(seq.as.string);
        char *r = xmalloc(len * (size_t)n + 1), *p = r;
        for (long long i = 0; i < n; i++) { memcpy(p, seq.as.string, len); p += len; }
        *p = 0;
        return value_string_owned(r);
    }
    Value out = value_list();
    for (long long i = 0; i < n; i++)
        for (size_t j = 0; j < seq.as.list->length; j++) list_push(out.as.list, seq.as.list->items[j]);
    return out;
}

static Value binary_op(int op, Value a, Value b) {
    if (op == OP_ADD) {
        if (a.type == VALUE_STRING && b.type == VALUE_STRING) {
            size_t la = strlen(a.as.string), lb = strlen(b.as.string);
            char *r = xmalloc(la + lb + 1);
            memcpy(r, a.as.string, la); memcpy(r + la, b.as.string, lb + 1);
            return value_string_owned(r);
        }
        if (a.type == VALUE_LIST && b.type == VALUE_LIST) {
            Value out = list_from_items(a.as.list->items, a.as.list->length);
            for (size_t i = 0; i < b.as.list->length; i++) list_push(out.as.list, b.as.list->items[i]);
            return out;
        }
        if (a.type == VALUE_STRING) py_error("TypeError", "can only concatenate str (not \"%s\") to str", type_name(b));
        if (a.type == VALUE_LIST) py_error("TypeError", "can only concatenate list (not \"%s\") to list", type_name(b));
    }
    if (op == OP_MUL) {
        bool sa = a.type == VALUE_STRING || a.type == VALUE_LIST, sb = b.type == VALUE_STRING || b.type == VALUE_LIST;
        if (sa && is_intlike(b)) return repeat_sequence(a, as_ll(b));
        if (sb && is_intlike(a)) return repeat_sequence(b, as_ll(a));
        if (sa) py_error("TypeError", "can't multiply sequence by non-int of type '%s'", type_name(b));
        if (sb) py_error("TypeError", "can't multiply sequence by non-int of type '%s'", type_name(a));
    }
    if (!is_number(a) || !is_number(b))
        py_error("TypeError", "unsupported operand type(s) for %s: '%s' and '%s'", op_symbol(op), type_name(a), type_name(b));

    bool ints = is_intlike(a) && is_intlike(b);
    switch (op) {
    case OP_ADD: return ints ? value_int(checked_add(as_ll(a), as_ll(b))) : value_float(as_dbl(a) + as_dbl(b));
    case OP_SUB: return ints ? value_int(checked_sub(as_ll(a), as_ll(b))) : value_float(as_dbl(a) - as_dbl(b));
    case OP_MUL: return ints ? value_int(checked_mul(as_ll(a), as_ll(b))) : value_float(as_dbl(a) * as_dbl(b));
    case OP_DIV:
        if (as_dbl(b) == 0.0) py_error("ZeroDivisionError", "division by zero");
        return value_float(as_dbl(a) / as_dbl(b));
    case OP_FLOORDIV:
        if (ints) {
            long long x = as_ll(a), y = as_ll(b);
            if (y == 0) py_error("ZeroDivisionError", "integer division or modulo by zero");
            if (y == -1) return value_int(checked_sub(0, x));
            long long q = x / y;
            if ((x % y != 0) && ((x < 0) != (y < 0))) q--;
            return value_int(q);
        }
        if (as_dbl(b) == 0.0) py_error("ZeroDivisionError", "float floor division by zero");
        return value_float(floor(as_dbl(a) / as_dbl(b)));
    case OP_MOD:
        if (ints) {
            long long x = as_ll(a), y = as_ll(b);
            if (y == 0) py_error("ZeroDivisionError", "integer modulo by zero");
            if (y == -1) return value_int(0);
            long long r = x % y;
            if (r != 0 && ((r < 0) != (y < 0))) r += y;
            return value_int(r);
        } else {
            double x = as_dbl(a), y = as_dbl(b);
            if (y == 0.0) py_error("ZeroDivisionError", "float modulo by zero");
            double r = fmod(x, y);
            if (r != 0.0 && ((r < 0) != (y < 0))) r += y;
            return value_float(r);
        }
    case OP_POW:
        if (ints && as_ll(b) >= 0) {
            long long base = as_ll(a), e = as_ll(b), r = 1;
            while (e > 0) {
                if (e & 1) r = checked_mul(r, base);
                e >>= 1;
                if (e) base = checked_mul(base, base);
            }
            return value_int(r);
        } else {
            double x = as_dbl(a), y = as_dbl(b);
            if (x == 0.0 && y < 0) py_error("ZeroDivisionError", "0.0 cannot be raised to a negative power");
            if (x < 0 && y != floor(y)) py_error("ValueError", "negative number cannot be raised to a fractional power (complex numbers are not supported)");
            return value_float(pow(x, y));
        }
    }
    return value_none();
}

static bool values_equal(Value a, Value b) {
    if (is_number(a) && is_number(b)) {
        if (is_intlike(a) && is_intlike(b)) return as_ll(a) == as_ll(b);
        return as_dbl(a) == as_dbl(b);
    }
    if (a.type != b.type) return false;
    switch (a.type) {
    case VALUE_NONE: return true;
    case VALUE_STRING: return !strcmp(a.as.string, b.as.string);
    case VALUE_LIST:
        if (a.as.list == b.as.list) return true;
        if (a.as.list->length != b.as.list->length) return false;
        for (size_t i = 0; i < a.as.list->length; i++)
            if (!values_equal(a.as.list->items[i], b.as.list->items[i])) return false;
        return true;
    case VALUE_FUNCTION: return a.as.function == b.as.function;
    default: return false;
    }
}

/* Three-way ordering; raises TypeError for values that cannot be ordered. */
static int values_cmp(Value a, Value b, const char *op) {
    if (is_number(a) && is_number(b)) {
        if (is_intlike(a) && is_intlike(b)) { long long x = as_ll(a), y = as_ll(b); return x < y ? -1 : x > y; }
        double x = as_dbl(a), y = as_dbl(b);
        return x < y ? -1 : x > y;
    }
    if (a.type == VALUE_STRING && b.type == VALUE_STRING) {
        int c = strcmp(a.as.string, b.as.string);
        return c < 0 ? -1 : c > 0;
    }
    if (a.type == VALUE_LIST && b.type == VALUE_LIST) {
        size_t n = a.as.list->length < b.as.list->length ? a.as.list->length : b.as.list->length;
        for (size_t i = 0; i < n; i++)
            if (!values_equal(a.as.list->items[i], b.as.list->items[i]))
                return values_cmp(a.as.list->items[i], b.as.list->items[i], op);
        return a.as.list->length < b.as.list->length ? -1 : a.as.list->length > b.as.list->length;
    }
    py_error("TypeError", "'%s' not supported between instances of '%s' and '%s'", op, type_name(a), type_name(b));
}

static bool contains(Value item, Value container) {
    if (container.type == VALUE_STRING) {
        if (item.type != VALUE_STRING) py_error("TypeError", "'in <string>' requires string as left operand, not %s", type_name(item));
        return strstr(container.as.string, item.as.string) != NULL;
    }
    if (container.type == VALUE_LIST) {
        for (size_t i = 0; i < container.as.list->length; i++)
            if (values_equal(item, container.as.list->items[i])) return true;
        return false;
    }
    py_error("TypeError", "argument of type '%s' is not iterable", type_name(container));
}

static bool identical(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case VALUE_NONE: return true;
    case VALUE_BOOL: return a.as.boolean == b.as.boolean;
    case VALUE_INT: return a.as.integer == b.as.integer;
    case VALUE_FLOAT: return a.as.floating == b.as.floating;
    case VALUE_STRING: return !strcmp(a.as.string, b.as.string);
    case VALUE_LIST: return a.as.list == b.as.list;
    case VALUE_FUNCTION: return a.as.function == b.as.function;
    }
    return false;
}

static Value compare_op(int op, Value a, Value b) {
    switch (op) {
    case OP_EQ: return value_bool(values_equal(a, b));
    case OP_NE: return value_bool(!values_equal(a, b));
    case OP_LT: return value_bool(values_cmp(a, b, "<") < 0);
    case OP_GT: return value_bool(values_cmp(a, b, ">") > 0);
    case OP_LE: return value_bool(values_cmp(a, b, "<=") <= 0);
    case OP_GE: return value_bool(values_cmp(a, b, ">=") >= 0);
    case OP_IN: return value_bool(contains(a, b));
    case OP_NOTIN: return value_bool(!contains(a, b));
    case OP_IS: return value_bool(identical(a, b));
    case OP_ISNOT: return value_bool(!identical(a, b));
    }
    return value_bool(false);
}

/* ===================================================================== */
/*  Sequences: indexing, slicing, stable merge sort                      */
/* ===================================================================== */
static long long index_value(Value v, const char *errfmt_type, const char *container) {
    if (!is_intlike(v)) {
        if (!strcmp(container, "string")) py_error("TypeError", "string indices must be integers, not '%s'", type_name(v));
        py_error("TypeError", "%s indices must be integers or slices, not %s", errfmt_type, type_name(v));
    }
    return as_ll(v);
}

static Value index_value_of(Value obj, Value idx) {
    if (obj.type == VALUE_LIST) {
        long long i = index_value(idx, "list", "list");
        long long n = (long long)obj.as.list->length;
        if (i < 0) i += n;
        if (i < 0 || i >= n) py_error("IndexError", "list index out of range");
        return obj.as.list->items[i];
    }
    if (obj.type == VALUE_STRING) {
        long long i = index_value(idx, "string", "string");
        long long n = (long long)strlen(obj.as.string);
        if (i < 0) i += n;
        if (i < 0 || i >= n) py_error("IndexError", "string index out of range");
        char t[2] = { obj.as.string[i], 0 };
        return value_string(t);
    }
    py_error("TypeError", "'%s' object is not subscriptable", type_name(obj));
}

static long long slice_arg(Value v) {
    if (!is_intlike(v)) py_error("TypeError", "slice indices must be integers or None or have an __index__ method");
    return as_ll(v);
}
/* CPython's slice index adjustment. Returns the number of selected items. */
static long long slice_bounds(long long len, Value vstart, Value vstop, Value vstep, long long *start, long long *step) {
    long long st = vstep.type == VALUE_NONE ? 1 : slice_arg(vstep);
    if (st == 0) py_error("ValueError", "slice step cannot be zero");
    long long lower = st < 0 ? -1 : 0, upper = st < 0 ? len - 1 : len;
    long long s, e;
    if (vstart.type == VALUE_NONE) s = st < 0 ? upper : lower;
    else { s = slice_arg(vstart); if (s < 0) { s += len; if (s < lower) s = lower; } else if (s > upper) s = upper; }
    if (vstop.type == VALUE_NONE) e = st < 0 ? lower : upper;
    else { e = slice_arg(vstop); if (e < 0) { e += len; if (e < lower) e = lower; } else if (e > upper) e = upper; }
    *start = s; *step = st;
    if (st < 0) return e < s ? (s - e - 1) / (-st) + 1 : 0;
    return s < e ? (e - s - 1) / st + 1 : 0;
}
static Value slice_of(Value obj, Value a, Value b, Value c) {
    long long start, step, count;
    if (obj.type == VALUE_LIST) {
        count = slice_bounds((long long)obj.as.list->length, a, b, c, &start, &step);
        Value out = value_list();
        for (long long i = 0, k = start; i < count; i++, k += step) list_push(out.as.list, obj.as.list->items[k]);
        return out;
    }
    if (obj.type == VALUE_STRING) {
        count = slice_bounds((long long)strlen(obj.as.string), a, b, c, &start, &step);
        char *r = xmalloc((size_t)count + 1);
        for (long long i = 0, k = start; i < count; i++, k += step) r[i] = obj.as.string[k];
        r[count] = 0;
        return value_string_owned(r);
    }
    py_error("TypeError", "'%s' object is not subscriptable", type_name(obj));
}

static void merge_sort(Value *a, Value *tmp, size_t n, bool reverse) {
    if (n < 2) return;
    size_t mid = n / 2;
    merge_sort(a, tmp, mid, reverse);
    merge_sort(a + mid, tmp, n - mid, reverse);
    size_t i = 0, j = mid, k = 0;
    while (i < mid && j < n) {
        /* take from the right only when it must come strictly before the left */
        int c = reverse ? values_cmp(a[i], a[j], "<") : values_cmp(a[j], a[i], "<");
        if (reverse ? c < 0 : c < 0) tmp[k++] = a[j++]; else tmp[k++] = a[i++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < n) tmp[k++] = a[j++];
    memcpy(a, tmp, n * sizeof(Value));
}
static void sort_values(Value *items, size_t n, bool reverse) {
    if (n < 2) return;
    Value *tmp = xmalloc(n * sizeof(Value));
    merge_sort(items, tmp, n, reverse);
    free(tmp);
}

/* ===================================================================== */
/*  Evaluation                                                           */
/* ===================================================================== */
typedef struct {
    Value *pos; size_t npos;
    const char **kwname; Value *kwval; size_t nkw;
} Args;

static Value eval(Expr *e);
static Value call_user(Function *f, Args *a);

static void eval_args(ExprList *list, Args *a) {
    size_t n = 0;
    for (ExprList *p = list; p; p = p->next) n++;
    a->pos = xmalloc(n * sizeof(Value)); a->kwval = xmalloc(n * sizeof(Value));
    a->kwname = xmalloc(n * sizeof(char *));
    a->npos = a->nkw = 0;
    for (ExprList *p = list; p; p = p->next) {
        if (p->expr->kind == EX_KWARG) {
            a->kwname[a->nkw] = p->expr->name;
            a->kwval[a->nkw++] = eval(p->expr->left);
        } else {
            if (a->nkw) py_error("SyntaxError", "positional argument follows keyword argument");
            a->pos[a->npos++] = eval(p->expr);
        }
    }
}
static void args_free(Args *a) { free(a->pos); free(a->kwval); free(a->kwname); }

static void no_kwargs(const char *fn, Args *a) {
    if (a->nkw) py_error("TypeError", "%s() takes no keyword arguments", fn);
}
static void check_arity(const char *fn, Args *a, size_t min, size_t max) {
    no_kwargs(fn, a);
    if (a->npos >= min && a->npos <= max) return;
    if (min == max)
        py_error("TypeError", "%s() takes exactly %zu argument%s (%zu given)", fn, min, min == 1 ? "" : "s", a->npos);
    if (a->npos < min)
        py_error("TypeError", "%s() takes at least %zu argument%s (%zu given)", fn, min, min == 1 ? "" : "s", a->npos);
    py_error("TypeError", "%s() takes at most %zu argument%s (%zu given)", fn, max, max == 1 ? "" : "s", a->npos);
}

static bool kw_reverse(const char *fn, Args *a) {
    bool rev = false;
    for (size_t i = 0; i < a->nkw; i++) {
        if (!strcmp(a->kwname[i], "reverse")) rev = truthy(a->kwval[i]);
        else py_error("TypeError", "%s() got an unexpected keyword argument '%s'%s", fn, a->kwname[i],
                      !strcmp(a->kwname[i], "key") ? " (key= is not supported in ShakilScript)" : "");
    }
    return rev;
}

/* Turn a list or string into an array of items (strings become 1-char strings). */
static Value iterable_to_list(Value v) {
    if (v.type == VALUE_LIST) return list_from_items(v.as.list->items, v.as.list->length);
    if (v.type == VALUE_STRING) {
        Value out = value_list();
        for (const char *p = v.as.string; *p; p++) { char t[2] = { *p, 0 }; list_push(out.as.list, value_string(t)); }
        return out;
    }
    py_error("TypeError", "'%s' object is not iterable", type_name(v));
}

static void range_bounds(const char *fn, Args *a, long long *start, long long *stop, long long *step) {
    check_arity(fn, a, 1, 3);
    for (size_t i = 0; i < a->npos; i++)
        if (!is_intlike(a->pos[i])) py_error("TypeError", "'%s' object cannot be interpreted as an integer", type_name(a->pos[i]));
    *start = 0; *step = 1;
    if (a->npos == 1) *stop = as_ll(a->pos[0]);
    else { *start = as_ll(a->pos[0]); *stop = as_ll(a->pos[1]); if (a->npos == 3) *step = as_ll(a->pos[2]); }
    if (*step == 0) py_error("ValueError", "range() arg 3 must not be zero");
}

static bool parse_int_string(const char *s, long long *out) {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v') s++;
    const char *p = s; bool neg = false;
    if (*p == '+' || *p == '-') { neg = *p == '-'; p++; }
    if (!(*p >= '0' && *p <= '9')) return false;
    long long v = 0;
    for (; *p >= '0' && *p <= '9'; p++) {
        int d = *p - '0';
        if (__builtin_mul_overflow(v, 10, &v) || __builtin_add_overflow(v, neg ? -d : d, &v))
            py_error("OverflowError", "integer too large (ShakilScript integers are 64-bit)");
        /* note: accumulating negatively handled by adding signed digit */
    }
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == '\f' || *p == '\v') p++;
    if (*p) return false;
    *out = v;
    return true;
}
static bool parse_float_string(const char *s, double *out) {
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r' || *s == '\f' || *s == '\v') s++;
    if (!*s || strchr(s, 'x') || strchr(s, 'X')) return false;
    char *end;
    double d = strtod(s, &end);
    if (end == s) return false;
    while (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r' || *end == '\f' || *end == '\v') end++;
    if (*end) return false;
    *out = d;
    return true;
}

static Value builtin_int(Args *a) {
    check_arity("int", a, 0, 1);
    if (a->npos == 0) return value_int(0);
    Value v = a->pos[0];
    if (is_intlike(v)) return value_int(as_ll(v));
    if (v.type == VALUE_FLOAT) {
        double d = v.as.floating;
        if (isnan(d)) py_error("ValueError", "cannot convert float NaN to integer");
        if (isinf(d)) py_error("OverflowError", "cannot convert float infinity to integer");
        if (d >= 9.2233720368547758e18 || d <= -9.2233720368547758e18) py_error("OverflowError", "float too large for a 64-bit integer");
        return value_int((long long)d);
    }
    if (v.type == VALUE_STRING) {
        long long n;
        if (!parse_int_string(v.as.string, &n)) {
            char *r = repr_of(v);
            py_error("ValueError", "invalid literal for int() with base 10: %s", r);
        }
        return value_int(n);
    }
    py_error("TypeError", "int() argument must be a string, a bytes-like object or a real number, not '%s'", type_name(v));
}
static Value builtin_float(Args *a) {
    check_arity("float", a, 0, 1);
    if (a->npos == 0) return value_float(0.0);
    Value v = a->pos[0];
    if (is_number(v)) return value_float(as_dbl(v));
    if (v.type == VALUE_STRING) {
        double d;
        if (!parse_float_string(v.as.string, &d)) {
            char *r = repr_of(v);
            py_error("ValueError", "could not convert string to float: %s", r);
        }
        return value_float(d);
    }
    py_error("TypeError", "float() argument must be a string or a real number, not '%s'", type_name(v));
}

static Value builtin_sum(Args *a) {
    check_arity("sum", a, 1, 2);
    Value items = a->pos[0];
    if (items.type != VALUE_LIST && items.type != VALUE_STRING) py_error("TypeError", "'%s' object is not iterable", type_name(items));
    items = iterable_to_list(items);
    Value acc = a->npos == 2 ? a->pos[1] : value_int(0);
    if (acc.type == VALUE_STRING) py_error("TypeError", "sum() can't sum strings [use ''.join(seq) instead]");
    size_t i = 0, n = items.as.list->length;
    Value *it = items.as.list->items;
    while (i < n && is_intlike(acc) && is_intlike(it[i])) { acc = value_int(checked_add(as_ll(acc), as_ll(it[i]))); i++; }
    if (i < n && is_number(acc) && is_number(it[i])) {      /* float path with compensated summation */
        double f = as_dbl(acc), c = 0.0;
        while (i < n && is_number(it[i])) {
            double x = as_dbl(it[i]), t = f + x;
            if (fabs(f) >= fabs(x)) c += (f - t) + x; else c += (x - t) + f;
            f = t; i++;
        }
        if (c != 0.0 && isfinite(c)) f += c;
        acc = value_float(f);
    }
    for (; i < n; i++) acc = binary_op(OP_ADD, acc, it[i]);
    return acc;
}

static Value builtin_minmax(const char *fn, Args *a, bool want_max) {
    no_kwargs(fn, a);
    Value *vals; size_t n; Value tmp;
    if (a->npos == 0) py_error("TypeError", "%s expected at least 1 argument, got 0", fn);
    if (a->npos == 1) {
        tmp = iterable_to_list(a->pos[0]);
        vals = tmp.as.list->items; n = tmp.as.list->length;
        if (n == 0) py_error("ValueError", "%s() iterable argument is empty", fn);
    } else { vals = a->pos; n = a->npos; }
    Value best = vals[0];
    for (size_t i = 1; i < n; i++) {
        int c = want_max ? values_cmp(vals[i], best, ">") : values_cmp(vals[i], best, "<");
        if (want_max ? c > 0 : c < 0) best = vals[i];
    }
    return best;
}

static char *read_line_or_eof(void) {
    StrBuf b; sb_init(&b);
    int c; bool got = false;
    while ((c = getchar()) != EOF) { got = true; if (c == '\n') break; sb_add_char(&b, (char)c); }
    if (!got) py_error("EOFError", "EOF when reading a line");
    if (b.len && b.data[b.len - 1] == '\r') b.data[--b.len] = 0;
    return b.data;
}

/* Built-in functions looked up by name. Returns false when `name` is not a built-in. */
static bool call_builtin(const char *name, Args *a, Value *out) {
    if (!strcmp(name, "print")) {
        const char *sep = " ", *end = "\n";
        for (size_t i = 0; i < a->nkw; i++) {
            Value v = a->kwval[i];
            const char **slot = !strcmp(a->kwname[i], "sep") ? &sep : !strcmp(a->kwname[i], "end") ? &end : NULL;
            if (!slot) py_error("TypeError", "'%s' is an invalid keyword argument for print()", a->kwname[i]);
            if (v.type == VALUE_STRING) *slot = v.as.string;
            else if (v.type == VALUE_NONE) *slot = slot == &sep ? " " : "\n";
            else py_error("TypeError", "%s must be None or a string, not %s", a->kwname[i], type_name(v));
        }
        StrBuf b; sb_init(&b);
        for (size_t i = 0; i < a->npos; i++) { if (i) sb_add(&b, sep); append_str(&b, a->pos[i]); }
        sb_add(&b, end);
        fputs(b.data, stdout);
        free(b.data);
        *out = value_none();
    } else if (!strcmp(name, "input")) {
        check_arity("input", a, 0, 1);
        if (a->npos == 1) { StrBuf b; sb_init(&b); append_str(&b, a->pos[0]); fputs(b.data, stdout); free(b.data); }
        fflush(stdout);
        *out = value_string_owned(read_line_or_eof());
    } else if (!strcmp(name, "len")) {
        check_arity("len", a, 1, 1);
        Value v = a->pos[0];
        if (v.type == VALUE_STRING) *out = value_int((long long)strlen(v.as.string));
        else if (v.type == VALUE_LIST) *out = value_int((long long)v.as.list->length);
        else py_error("TypeError", "object of type '%s' has no len()", type_name(v));
    } else if (!strcmp(name, "range")) {
        long long start, stop, step;
        range_bounds("range", a, &start, &stop, &step);
        Value l = value_list();
        for (long long i = start; step > 0 ? i < stop : i > stop; i += step) list_push(l.as.list, value_int(i));
        *out = l;
    } else if (!strcmp(name, "int")) { *out = builtin_int(a);
    } else if (!strcmp(name, "float")) { *out = builtin_float(a);
    } else if (!strcmp(name, "str")) {
        check_arity("str", a, 0, 1);
        StrBuf b; sb_init(&b);
        if (a->npos == 1) append_str(&b, a->pos[0]);
        *out = value_string_owned(b.data);
    } else if (!strcmp(name, "bool")) {
        check_arity("bool", a, 0, 1);
        *out = value_bool(a->npos == 1 && truthy(a->pos[0]));
    } else if (!strcmp(name, "list")) {
        check_arity("list", a, 0, 1);
        *out = a->npos == 0 ? value_list() : iterable_to_list(a->pos[0]);
    } else if (!strcmp(name, "sorted")) {
        bool rev = kw_reverse("sorted", a);
        if (a->npos != 1) py_error("TypeError", "sorted expected 1 argument, got %zu", a->npos);
        Value l = iterable_to_list(a->pos[0]);
        sort_values(l.as.list->items, l.as.list->length, rev);
        *out = l;
    } else if (!strcmp(name, "abs")) {
        check_arity("abs", a, 1, 1);
        Value v = a->pos[0];
        if (is_intlike(v)) { long long n = as_ll(v); *out = value_int(n < 0 ? checked_sub(0, n) : n); }
        else if (v.type == VALUE_FLOAT) *out = value_float(fabs(v.as.floating));
        else py_error("TypeError", "bad operand type for abs(): '%s'", type_name(v));
    } else if (!strcmp(name, "min")) { *out = builtin_minmax("min", a, false);
    } else if (!strcmp(name, "max")) { *out = builtin_minmax("max", a, true);
    } else if (!strcmp(name, "sum")) { *out = builtin_sum(a);
    } else return false;
    return true;
}

/* ---------- methods ---------- */
static long long method_index(Value v) {
    if (!is_intlike(v)) py_error("TypeError", "'%s' object cannot be interpreted as an integer", type_name(v));
    return as_ll(v);
}
static bool is_space_char(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

static Value list_method(Value obj, const char *m, Args *a) {
    List *l = obj.as.list;
    if (!strcmp(m, "append")) { check_arity("append", a, 1, 1); list_push(l, a->pos[0]); return value_none(); }
    if (!strcmp(m, "pop")) {
        check_arity("pop", a, 0, 1);
        if (l->length == 0) py_error("IndexError", "pop from empty list");
        long long i = a->npos ? method_index(a->pos[0]) : (long long)l->length - 1, n = (long long)l->length;
        if (i < 0) i += n;
        if (i < 0 || i >= n) py_error("IndexError", "pop index out of range");
        Value r = l->items[i];
        memmove(l->items + i, l->items + i + 1, (size_t)(n - i - 1) * sizeof(Value));
        l->length--;
        return r;
    }
    if (!strcmp(m, "remove")) {
        check_arity("remove", a, 1, 1);
        for (size_t i = 0; i < l->length; i++)
            if (values_equal(l->items[i], a->pos[0])) {
                memmove(l->items + i, l->items + i + 1, (l->length - i - 1) * sizeof(Value));
                l->length--;
                return value_none();
            }
        py_error("ValueError", "list.remove(x): x not in list");
    }
    if (!strcmp(m, "reverse")) {
        check_arity("reverse", a, 0, 0);
        for (size_t i = 0; i < l->length / 2; i++) { Value t = l->items[i]; l->items[i] = l->items[l->length - 1 - i]; l->items[l->length - 1 - i] = t; }
        return value_none();
    }
    if (!strcmp(m, "sort")) {
        bool rev = kw_reverse("sort", a);
        if (a->npos) py_error("TypeError", "sort() takes no positional arguments");
        sort_values(l->items, l->length, rev);
        return value_none();
    }
    if (!strcmp(m, "index")) {
        check_arity("index", a, 1, 1);
        for (size_t i = 0; i < l->length; i++) if (values_equal(l->items[i], a->pos[0])) return value_int((long long)i);
        char *r = repr_of(a->pos[0]);
        py_error("ValueError", "%s is not in list", r);
    }
    if (!strcmp(m, "count")) {
        check_arity("count", a, 1, 1);
        long long c = 0;
        for (size_t i = 0; i < l->length; i++) if (values_equal(l->items[i], a->pos[0])) c++;
        return value_int(c);
    }
    if (!strcmp(m, "insert")) {
        check_arity("insert", a, 2, 2);
        long long i = method_index(a->pos[0]), n = (long long)l->length;
        if (i < 0) { i += n; if (i < 0) i = 0; }
        if (i > n) i = n;
        list_push(l, value_none());
        memmove(l->items + i + 1, l->items + i, (size_t)(n - i) * sizeof(Value));
        l->items[i] = a->pos[1];
        return value_none();
    }
    if (!strcmp(m, "extend")) {
        check_arity("extend", a, 1, 1);
        Value src = iterable_to_list(a->pos[0]);
        for (size_t i = 0; i < src.as.list->length; i++) list_push(l, src.as.list->items[i]);
        return value_none();
    }
    py_error("AttributeError", "'list' object has no attribute '%s'", m);
}

static Value string_method(Value obj, const char *m, Args *a) {
    const char *s = obj.as.string;
    size_t len = strlen(s);
    if (!strcmp(m, "upper") || !strcmp(m, "lower")) {
        check_arity(m, a, 0, 0);
        char *r = xstrdup(s);
        for (size_t i = 0; i < len; i++) {
            if (m[0] == 'u' && r[i] >= 'a' && r[i] <= 'z') r[i] -= 32;
            if (m[0] == 'l' && r[i] >= 'A' && r[i] <= 'Z') r[i] += 32;
        }
        return value_string_owned(r);
    }
    if (!strcmp(m, "strip")) {
        check_arity("strip", a, 0, 1);
        const char *chars = NULL;
        if (a->npos) { if (a->pos[0].type != VALUE_STRING) py_error("TypeError", "strip arg must be None or str"); chars = a->pos[0].as.string; }
        size_t b = 0, e = len;
        #define STRIPPABLE(c) (chars ? strchr(chars, (c)) != NULL : is_space_char(c))
        while (b < e && STRIPPABLE(s[b])) b++;
        while (e > b && STRIPPABLE(s[e - 1])) e--;
        #undef STRIPPABLE
        char *r = xmalloc(e - b + 1); memcpy(r, s + b, e - b); r[e - b] = 0;
        return value_string_owned(r);
    }
    if (!strcmp(m, "find") || !strcmp(m, "count") || !strcmp(m, "startswith") || !strcmp(m, "endswith")) {
        check_arity(m, a, 1, 1);
        if (a->pos[0].type != VALUE_STRING)
            py_error("TypeError", "%s() argument must be str, not %s", m, type_name(a->pos[0]));
        const char *sub = a->pos[0].as.string; size_t sl = strlen(sub);
        if (!strcmp(m, "find")) { const char *p = strstr(s, sub); return value_int(p ? (long long)(p - s) : -1); }
        if (!strcmp(m, "startswith")) return value_bool(sl <= len && !strncmp(s, sub, sl));
        if (!strcmp(m, "endswith")) return value_bool(sl <= len && !strcmp(s + len - sl, sub));
        if (sl == 0) return value_int((long long)len + 1);
        long long c = 0;
        for (const char *p = strstr(s, sub); p; p = strstr(p + sl, sub)) c++;
        return value_int(c);
    }
    if (!strcmp(m, "replace")) {
        check_arity("replace", a, 2, 2);
        if (a->pos[0].type != VALUE_STRING || a->pos[1].type != VALUE_STRING)
            py_error("TypeError", "replace() arguments must be str");
        const char *from = a->pos[0].as.string, *to = a->pos[1].as.string;
        size_t fl = strlen(from);
        StrBuf b; sb_init(&b);
        if (fl == 0) {
            for (size_t i = 0; i < len; i++) { sb_add(&b, to); sb_add_char(&b, s[i]); }
            sb_add(&b, to);
        } else {
            const char *p = s, *q;
            while ((q = strstr(p, from))) { sb_add_n(&b, p, (size_t)(q - p)); sb_add(&b, to); p = q + fl; }
            sb_add(&b, p);
        }
        return value_string_owned(b.data);
    }
    if (!strcmp(m, "split")) {
        check_arity("split", a, 0, 1);
        Value out = value_list();
        if (a->npos == 0 || a->pos[0].type == VALUE_NONE) {
            size_t i = 0;
            while (i < len) {
                while (i < len && is_space_char(s[i])) i++;
                if (i >= len) break;
                size_t st = i;
                while (i < len && !is_space_char(s[i])) i++;
                char *piece = xmalloc(i - st + 1); memcpy(piece, s + st, i - st); piece[i - st] = 0;
                list_push(out.as.list, value_string_owned(piece));
            }
            return out;
        }
        if (a->pos[0].type != VALUE_STRING) py_error("TypeError", "must be str or None, not %s", type_name(a->pos[0]));
        const char *sep = a->pos[0].as.string; size_t sl = strlen(sep);
        if (sl == 0) py_error("ValueError", "empty separator");
        const char *p = s, *q;
        while ((q = strstr(p, sep))) {
            char *piece = xmalloc((size_t)(q - p) + 1); memcpy(piece, p, (size_t)(q - p)); piece[q - p] = 0;
            list_push(out.as.list, value_string_owned(piece));
            p = q + sl;
        }
        list_push(out.as.list, value_string(p));
        return out;
    }
    if (!strcmp(m, "join")) {
        check_arity("join", a, 1, 1);
        Value items = iterable_to_list(a->pos[0]);
        StrBuf b; sb_init(&b);
        for (size_t i = 0; i < items.as.list->length; i++) {
            Value it = items.as.list->items[i];
            if (it.type != VALUE_STRING) py_error("TypeError", "sequence item %zu: expected str instance, %s found", i, type_name(it));
            if (i) sb_add(&b, s);
            sb_add(&b, it.as.string);
        }
        return value_string_owned(b.data);
    }
    py_error("AttributeError", "'str' object has no attribute '%s'", m);
}

static Value call_method(Value obj, const char *m, Args *a) {
    if (obj.type == VALUE_LIST) return list_method(obj, m, a);
    if (obj.type == VALUE_STRING) return string_method(obj, m, a);
    py_error("AttributeError", "'%s' object has no attribute '%s'", type_name(obj), m);
}

/* ---------- user-defined functions ---------- */
static Value return_value;
typedef enum { SIG_NONE, SIG_BREAK, SIG_CONTINUE, SIG_RETURN } Signal;
static Signal exec_block(Stmt *s);

static void join_names(char *buf, size_t cap, char **names, size_t n) {
    buf[0] = 0;
    for (size_t i = 0; i < n; i++) {
        char piece[128];
        snprintf(piece, sizeof piece, "%s'%s'%s", (i > 0 && i == n - 1 && n > 2) ? "and " : "", names[i],
                 i + 1 < n ? (n == 2 ? " and " : ", ") : "");
        strncat(buf, piece, cap - strlen(buf) - 1);
    }
}

/* Kept out of call_user so its large buffers do not use stack on every recursive call. */
#ifdef __GNUC__
__attribute__((noinline, noreturn))
#endif
static void report_missing_args(Function *f, const bool *set) {
    char *missing[64]; size_t n = 0;
    for (size_t i = 0; i < f->nparams && n < 64; i++) if (!set[i]) missing[n++] = f->param_names[i];
    char names[512]; join_names(names, sizeof names, missing, n);
    py_error("TypeError", "%s() missing %zu required positional argument%s: %s", f->name, n, n == 1 ? "" : "s", names);
}

static Value call_user(Function *f, Args *a) {
    if (a->npos > f->nparams)
        py_error("TypeError", "%s() takes %zu positional argument%s but %zu %s given", f->name, f->nparams,
                 f->nparams == 1 ? "" : "s", a->npos, a->npos == 1 ? "was" : "were");
    Value *bound = xmalloc((f->nparams ? f->nparams : 1) * sizeof(Value));
    bool *set = xcalloc(f->nparams ? f->nparams : 1, sizeof(bool));
    for (size_t i = 0; i < a->npos; i++) { bound[i] = a->pos[i]; set[i] = true; }
    for (size_t k = 0; k < a->nkw; k++) {
        size_t i = 0;
        while (i < f->nparams && strcmp(f->param_names[i], a->kwname[k])) i++;
        if (i == f->nparams) py_error("TypeError", "%s() got an unexpected keyword argument '%s'", f->name, a->kwname[k]);
        if (set[i]) py_error("TypeError", "%s() got multiple values for argument '%s'", f->name, a->kwname[k]);
        bound[i] = a->kwval[k]; set[i] = true;
    }
    for (size_t i = 0; i < f->nparams; i++) {
        if (set[i]) continue;
        if (f->has_default[i]) { bound[i] = f->defaults[i]; set[i] = true; }
    }
    for (size_t i = 0; i < f->nparams; i++) if (!set[i]) report_missing_args(f, set);
    if (call_depth >= MAX_RECURSION) py_error("RecursionError", "maximum recursion depth exceeded");

    Scope scope = { NULL, NULL };
    Scope *saved_scope = cur_scope;
    int saved_line = current_line;
    cur_scope = &scope;
    call_depth++;
    for (size_t i = 0; i < f->nparams; i++) assign_var(f->param_names[i], bound[i]);
    free(bound); free(set);

    Signal sig = exec_block(f->body);
    Value result = sig == SIG_RETURN ? return_value : value_none();
    return_value = value_none();

    call_depth--;
    cur_scope = saved_scope;
    current_line = saved_line;
    while (scope.vars) { Variable *v = scope.vars; scope.vars = v->next; free(v); }
    return result;
}

static Value eval_call(Expr *e) {
    Args a;
    Value callee = value_none();
    bool by_name = e->left->kind == EX_NAME;
    if (by_name) {
        Variable *var = lookup_var(e->left->name);
        if (var) callee = var->value;
        else {
            eval_args(e->args, &a);
            Value out;
            if (call_builtin(e->left->name, &a, &out)) { args_free(&a); return out; }
            py_error("NameError", "name '%s' is not defined", e->left->name);
        }
    } else callee = eval(e->left);
    eval_args(e->args, &a);
    if (callee.type != VALUE_FUNCTION) py_error("TypeError", "'%s' object is not callable", type_name(callee));
    Value r = call_user(callee.as.function, &a);
    args_free(&a);
    return r;
}

static Value eval(Expr *e) {
    switch (e->kind) {
    case EX_CONST: return e->value;
    case EX_NAME: {
        Variable *v = lookup_var(e->name);
        if (!v) py_error("NameError", "name '%s' is not defined", e->name);
        return v->value;
    }
    case EX_LIST: {
        Value out = value_list();
        for (ExprList *p = e->args; p; p = p->next) {
            if (p->expr->kind == EX_KWARG) py_error("SyntaxError", "keyword argument not allowed in a list");
            list_push(out.as.list, eval(p->expr));
        }
        return out;
    }
    case EX_UNARY: {
        Value v = eval(e->left);
        if (e->op == OP_NOT) return value_bool(!truthy(v));
        if (!is_number(v)) py_error("TypeError", "bad operand type for unary %s: '%s'", e->op == OP_NEG ? "-" : "+", type_name(v));
        if (e->op == OP_POS) return is_intlike(v) ? value_int(as_ll(v)) : v;
        if (is_intlike(v)) return value_int(checked_sub(0, as_ll(v)));
        return value_float(-v.as.floating);
    }
    case EX_BINARY: { Value a = eval(e->left); Value b = eval(e->right); return binary_op(e->op, a, b); }
    case EX_COMPARE: { Value a = eval(e->left); Value b = eval(e->right); return compare_op(e->op, a, b); }
    case EX_LOGICAL: {
        Value a = eval(e->left);
        if (e->op == OP_AND) return truthy(a) ? eval(e->right) : a;
        return truthy(a) ? a : eval(e->right);
    }
    case EX_CALL: return eval_call(e);
    case EX_METHOD: {
        Value obj = eval(e->left);
        Args a; eval_args(e->args, &a);
        Value r = call_method(obj, e->name, &a);
        args_free(&a);
        return r;
    }
    case EX_INDEX: { Value obj = eval(e->left); Value idx = eval(e->right); return index_value_of(obj, idx); }
    case EX_SLICE: {
        Value obj = eval(e->left);
        Value a = e->right ? eval(e->right) : value_none();
        Value b = e->extra ? eval(e->extra) : value_none();
        Value c = e->extra2 ? eval(e->extra2) : value_none();
        return slice_of(obj, a, b, c);
    }
    case EX_KWARG: py_error("SyntaxError", "keyword argument not allowed here");
    }
    return value_none();
}

/* ===================================================================== */
/*  Statements                                                           */
/* ===================================================================== */
static void assign_target(Expr *t, Value v) {
    if (t->kind == EX_NAME) { assign_var(t->name, v); return; }
    Value obj = eval(t->left), idx = eval(t->right);
    if (obj.type == VALUE_LIST) {
        long long i = index_value(idx, "list", "list"), n = (long long)obj.as.list->length;
        if (i < 0) i += n;
        if (i < 0 || i >= n) py_error("IndexError", "list assignment index out of range");
        obj.as.list->items[i] = v;
        return;
    }
    py_error("TypeError", "'%s' object does not support item assignment", type_name(obj));
}

static Signal after_iteration(Signal sig, bool *stop) {
    *stop = false;
    if (sig == SIG_BREAK) { *stop = true; return SIG_NONE; }
    if (sig == SIG_RETURN) { *stop = true; return SIG_RETURN; }
    return SIG_NONE;
}

static Signal exec_for(Stmt *s) {
    Expr *it = s->expr;
    bool stop; Signal out = SIG_NONE;
    bool lazy_range = it->kind == EX_CALL && it->left->kind == EX_NAME &&
                      !strcmp(it->left->name, "range") && lookup_var("range") == NULL;
    if (lazy_range) {                               /* iterate without building a list */
        Args a; eval_args(it->args, &a);
        long long start, stop_v, step;
        range_bounds("range", &a, &start, &stop_v, &step);
        args_free(&a);
        for (long long i = start; step > 0 ? i < stop_v : i > stop_v; i += step) {
            assign_var(s->name, value_int(i));
            out = after_iteration(exec_block(s->body), &stop);
            if (stop) break;
        }
        return out;
    }
    Value seq = eval(it);
    if (seq.type == VALUE_LIST) {
        for (size_t i = 0; i < seq.as.list->length; i++) {      /* live length, like Python */
            assign_var(s->name, seq.as.list->items[i]);
            out = after_iteration(exec_block(s->body), &stop);
            if (stop) break;
        }
        return out;
    }
    if (seq.type == VALUE_STRING) {
        const char *str = seq.as.string; size_t n = strlen(str);
        for (size_t i = 0; i < n; i++) {
            char t[2] = { str[i], 0 };
            assign_var(s->name, value_string(t));
            out = after_iteration(exec_block(s->body), &stop);
            if (stop) break;
        }
        return out;
    }
    py_error("TypeError", "'%s' object is not iterable", type_name(seq));
}

static Signal exec_stmt(Stmt *s) {
    current_line = s->line;
    switch (s->kind) {
    case ST_EXPR: (void)eval(s->expr); return SIG_NONE;
    case ST_ASSIGN: { Value v = eval(s->expr); assign_target(s->target, v); return SIG_NONE; }
    case ST_IF:
        if (truthy(eval(s->expr))) return exec_block(s->body);
        return exec_block(s->orelse);
    case ST_WHILE:
        for (;;) {
            current_line = s->line;
            if (!truthy(eval(s->expr))) break;
            bool stop; Signal r = after_iteration(exec_block(s->body), &stop);
            if (stop) return r;
        }
        return SIG_NONE;
    case ST_FOR: return exec_for(s);
    case ST_DEF: {
        Function *f = xcalloc(1, sizeof *f);
        f->name = s->name; f->body = s->body;
        for (ParamList *p = s->params; p; p = p->next) f->nparams++;
        f->param_names = xmalloc((f->nparams + 1) * sizeof(char *));
        f->defaults = xmalloc((f->nparams + 1) * sizeof(Value));
        f->has_default = xcalloc(f->nparams + 1, sizeof(bool));
        size_t i = 0;
        for (ParamList *p = s->params; p; p = p->next, i++) {
            f->param_names[i] = p->name;
            if (p->default_expr) { f->defaults[i] = eval(p->default_expr); f->has_default[i] = true; }
        }
        assign_var(s->name, value_function(f));
        return SIG_NONE;
    }
    case ST_RETURN: return_value = s->expr ? eval(s->expr) : value_none(); return SIG_RETURN;
    case ST_BREAK: return SIG_BREAK;
    case ST_CONTINUE: return SIG_CONTINUE;
    case ST_PASS: return SIG_NONE;
    case ST_GLOBAL:
        if (cur_scope)
            for (StringList *n = s->names; n; n = n->next) {
                StringList *g = xcalloc(1, sizeof *g);
                g->name = n->name; g->next = cur_scope->globals; cur_scope->globals = g;
            }
        return SIG_NONE;
    }
    return SIG_NONE;
}

static Signal exec_block(Stmt *s) {
    for (; s; s = s->next) {
        Signal sig = exec_stmt(s);
        if (sig != SIG_NONE) return sig;
    }
    return SIG_NONE;
}

/* ===================================================================== */
/*  Semantic check (before running): break/continue/return placement     */
/* ===================================================================== */
static int semantic_errors = 0;
static void semantic_error(int line, const char *msg) {
    fprintf(stderr, "ShakilScript error (line %d): SyntaxError: %s\n", line, msg);
    semantic_errors++;
}
static void check_block(Stmt *s, int in_loop, int in_func) {
    for (; s; s = s->next) {
        switch (s->kind) {
        case ST_BREAK: if (!in_loop) semantic_error(s->line, "'break' outside loop"); break;
        case ST_CONTINUE: if (!in_loop) semantic_error(s->line, "'continue' not properly in loop"); break;
        case ST_RETURN: if (!in_func) semantic_error(s->line, "'return' outside function"); break;
        case ST_IF: check_block(s->body, in_loop, in_func); check_block(s->orelse, in_loop, in_func); break;
        case ST_WHILE: case ST_FOR: check_block(s->body, 1, in_func); break;
        case ST_DEF: check_block(s->body, 0, 1); break;
        default: break;
        }
    }
}

static int run_state_check_only = 0;
int runtime_run(int check_only) {
    run_state_check_only = check_only;
    check_block(program, 0, 0);
    if (semantic_errors) return 1;
    if (run_state_check_only) { puts("OK: no syntax errors found"); return 0; }
    return_value = value_none();
    exec_block(program);
    fflush(stdout);
    return 0;
}
