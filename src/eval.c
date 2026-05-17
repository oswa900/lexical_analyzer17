#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/eval.h"

#define MAX_STEPS 100000

/* ── hash ─────────────────────────────────────────── */

static unsigned int hash_str(const char* s) {
    unsigned int h = 5381;
    while (*s) h = ((h << 5) + h) ^ (unsigned char)*s++;
    return h & (ENV_BUCKETS - 1);
}

/* ── Env ──────────────────────────────────────────── */

Env* env_init(void) {
    return calloc(1, sizeof(Env));
}

static Value env_get(const Env* env, const char* name) {
    Value undef = {VAL_UNDEF, 0, NULL};
    if (!env || !name) return undef;
    unsigned int idx = hash_str(name);
    for (EnvEntry* e = env->buckets[idx]; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e->val;
    return undef;
}

static void val_store(Value* dst, Value src) {
    if (dst->kind == VAL_STR) free(dst->sval);
    *dst = src;
    if (src.kind == VAL_STR && src.sval)
        dst->sval = strdup(src.sval);
}

static void env_set(Env* env, const char* name, Value val) {
    if (!env || !name) return;
    unsigned int idx = hash_str(name);
    for (EnvEntry* e = env->buckets[idx]; e; e = e->next) {
        if (strcmp(e->name, name) == 0) { val_store(&e->val, val); return; }
    }
    EnvEntry* e = malloc(sizeof(EnvEntry));
    e->name = strdup(name);
    e->val  = (Value){VAL_UNDEF, 0, NULL};
    val_store(&e->val, val);
    e->next = env->buckets[idx];
    env->buckets[idx] = e;
    env->count++;
}

void env_free(Env* env) {
    if (!env) return;
    for (int i = 0; i < ENV_BUCKETS; i++) {
        EnvEntry* e = env->buckets[i];
        while (e) {
            EnvEntry* next = e->next;
            free(e->name);
            if (e->val.kind == VAL_STR) free(e->val.sval);
            free(e);
            e = next;
        }
    }
    free(env);
}

/* ── utilidades ───────────────────────────────────── */

int is_temp_name(const char* name) {
    if (!name || name[0] != 't' || !name[1]) return 0;
    for (int i = 1; name[i]; i++)
        if (!isdigit((unsigned char)name[i])) return 0;
    return 1;
}

/* Elimina temporales del entorno (se llama al final de cada eval_run) */
static void env_purge_temps(Env* env) {
    for (int i = 0; i < ENV_BUCKETS; i++) {
        EnvEntry** pp = &env->buckets[i];
        while (*pp) {
            if (is_temp_name((*pp)->name)) {
                EnvEntry* dead = *pp;
                *pp = dead->next;
                free(dead->name);
                if (dead->val.kind == VAL_STR) free(dead->val.sval);
                free(dead);
                env->count--;
            } else {
                pp = &(*pp)->next;
            }
        }
    }
}

/* Convierte un string del TAC en un Value:
   - literal entero  → VAL_INT
   - literal string  → VAL_STR (borrow; env_set hace strdup)
   - nombre variable → buscar en env                            */
static Value resolve(const Env* env, const char* s) {
    Value undef = {VAL_UNDEF, 0, NULL};
    if (!s) return undef;
    char* end;
    long  ival = strtol(s, &end, 10);
    if (end != s && *end == '\0')
        return (Value){VAL_INT, (int)ival, NULL};
    if (s[0] == '\'')
        return (Value){VAL_STR, 0, (char*)s};
    return env_get(env, s);
}

static int val_truthy(Value v) {
    if (v.kind == VAL_INT) return v.ival != 0;
    if (v.kind == VAL_STR) return v.sval && v.sval[0] != '\0';
    return 0;
}

/* Busca la posicion de una etiqueta en el array de instrucciones */
static int find_label(TACInstr** arr, int n, const char* label) {
    for (int i = 0; i < n; i++)
        if (arr[i]->op == TAC_LABEL && strcmp(arr[i]->result, label) == 0)
            return i;
    return -1;
}

/* ── evaluador ────────────────────────────────────── */

int eval_run(const TACList* tac, Env* env) {
    if (!tac || !tac->head || !env) return 0;

    /* convertir lista enlazada a array para acceso por indice (saltos) */
    int n = 0;
    for (TACInstr* i = tac->head; i; i = i->next) n++;
    TACInstr** arr = malloc((size_t)n * sizeof(TACInstr*));
    if (!arr) return 0;
    int k = 0;
    for (TACInstr* i = tac->head; i; i = i->next) arr[k++] = i;

    int pc        = 0;
    int steps     = 0;
    int hit_limit = 0;

    while (pc < n) {
        if (steps++ >= MAX_STEPS) { hit_limit = 1; break; }

        TACInstr* in = arr[pc];

        switch (in->op) {

            /* result = arg1 */
            case TAC_ASSIGN: {
                env_set(env, in->result, resolve(env, in->arg1));
                pc++;
                break;
            }

            /* result = arg1 OP arg2  (aritmetica entera) */
            case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV: {
                Value a = resolve(env, in->arg1);
                Value b = resolve(env, in->arg2);
                Value r = {VAL_UNDEF, 0, NULL};
                if (a.kind == VAL_INT && b.kind == VAL_INT) {
                    r.kind = VAL_INT;
                    switch (in->op) {
                        case TAC_ADD: r.ival = a.ival + b.ival; break;
                        case TAC_SUB: r.ival = a.ival - b.ival; break;
                        case TAC_MUL: r.ival = a.ival * b.ival; break;
                        case TAC_DIV: r.ival = b.ival ? a.ival / b.ival : 0; break;
                        default: break;
                    }
                }
                env_set(env, in->result, r);
                pc++;
                break;
            }

            /* result = arg1 RELOP arg2  → 0 o 1 */
            case TAC_LT: case TAC_GT: case TAC_LE:
            case TAC_GE: case TAC_EQ: case TAC_NE: {
                Value a = resolve(env, in->arg1);
                Value b = resolve(env, in->arg2);
                Value r = {VAL_INT, 0, NULL};
                if (a.kind == VAL_INT && b.kind == VAL_INT) {
                    switch (in->op) {
                        case TAC_LT: r.ival = a.ival <  b.ival; break;
                        case TAC_GT: r.ival = a.ival >  b.ival; break;
                        case TAC_LE: r.ival = a.ival <= b.ival; break;
                        case TAC_GE: r.ival = a.ival >= b.ival; break;
                        case TAC_EQ: r.ival = a.ival == b.ival; break;
                        case TAC_NE: r.ival = a.ival != b.ival; break;
                        default: break;
                    }
                }
                env_set(env, in->result, r);
                pc++;
                break;
            }

            /* result = arg1 && arg2 */
            case TAC_AND: {
                Value a = resolve(env, in->arg1);
                Value b = resolve(env, in->arg2);
                Value r = {VAL_INT, val_truthy(a) && val_truthy(b), NULL};
                env_set(env, in->result, r);
                pc++;
                break;
            }

            /* result = arg1 || arg2 */
            case TAC_OR: {
                Value a = resolve(env, in->arg1);
                Value b = resolve(env, in->arg2);
                Value r = {VAL_INT, val_truthy(a) || val_truthy(b), NULL};
                env_set(env, in->result, r);
                pc++;
                break;
            }

            case TAC_LABEL:
                pc++;
                break;

            /* goto label */
            case TAC_GOTO: {
                int t = find_label(arr, n, in->result);
                pc = (t >= 0) ? t : pc + 1;
                break;
            }

            /* ifnot arg1 goto label */
            case TAC_IFNOT_GOTO: {
                Value cond = resolve(env, in->arg1);
                if (!val_truthy(cond)) {
                    int t = find_label(arr, n, in->result);
                    pc = (t >= 0) ? t : pc + 1;
                } else {
                    pc++;
                }
                break;
            }
        }
    }

    free(arr);
    env_purge_temps(env);
    return hit_limit;
}
