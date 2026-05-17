#ifndef EVAL_H
#define EVAL_H

/* ---------------------------------------------------------------
 * Evaluador de Codigo Intermedio (TAC)
 *
 * Ejecuta el TACList instruccion a instruccion manteniendo un
 * entorno de variables en tiempo de ejecucion (Env).
 *
 * El Env puede ser persistente entre llamadas (REPL): las variables
 * de usuario sobreviven entre lineas; los temporales (t0, t1, ...)
 * se eliminan automaticamente al final de cada eval_run.
 * --------------------------------------------------------------- */

#include "codegen.h"

/* ── Valores en tiempo de ejecucion ── */

typedef enum { VAL_INT, VAL_STR, VAL_UNDEF } ValKind;

typedef struct {
    ValKind kind;
    int     ival;   /* VAL_INT  */
    char*   sval;   /* VAL_STR — heap-allocated */
} Value;

/* ── Entorno de variables ── */

#define ENV_BUCKETS 64

typedef struct EnvEntry {
    char*            name;   /* heap-allocated */
    Value            val;
    struct EnvEntry* next;
} EnvEntry;

typedef struct Env {
    EnvEntry* buckets[ENV_BUCKETS];
    int       count;
} Env;

Env*  env_init(void);
void  env_free(Env* env);

/* 1 si el nombre es un temporal generado por el compilador (t0, t1, ...) */
int   is_temp_name(const char* name);

/* ── API del evaluador ── */

/* Ejecuta tac actualizando env.
   Devuelve 1 si se alcanzo el limite de pasos (posible bucle infinito). */
int eval_run(const TACList* tac, Env* env);

#endif
