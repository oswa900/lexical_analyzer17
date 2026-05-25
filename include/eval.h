#ifndef EVAL_H
#define EVAL_H

// Evaluador de Codigo Intermedio (TAC): ejecuta instrucciones manteniendo
// un entorno de variables en tiempo de ejecucion (Env).
// El Env persiste entre llamadas (REPL); los temporales (t0, t1, ...) se
// eliminan al final de cada eval_run.

#include "codegen.h"

typedef enum { VAL_INT, VAL_STR, VAL_UNDEF } ValKind;

typedef struct {
    ValKind kind;
    int     ival;   /* VAL_INT */
    char*   sval;   /* VAL_STR — heap-allocated */
} Value;

#define ENV_BUCKETS 64

typedef struct EnvEntry {
    char*            name;
    Value            val;
    struct EnvEntry* next;
} EnvEntry;

typedef struct Env {
    EnvEntry* buckets[ENV_BUCKETS];
    int       count;
    char**    output;       /* lineas producidas por echo */
    int       output_count;
    int       output_cap;
} Env;

Env*  env_init(void);
void  env_free(Env* env);

// Devuelve 1 si el nombre es un temporal generado por el compilador (t0, t1, ...)
int   is_temp_name(const char* name);

// Ejecuta tac actualizando env. Devuelve 1 si se alcanzo el limite de pasos.
int eval_run(const TACList* tac, Env* env);

#endif
