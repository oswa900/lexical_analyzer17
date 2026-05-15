#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/symbol_table.h"

/* ---------------------------------------------------------------
 * djb2 hash — rápido y suficientemente uniforme para identificadores
 * --------------------------------------------------------------- */
static unsigned int hash(const char* s) {
    unsigned int h = 5381;
    while (*s)
        h = ((h << 5) + h) ^ (unsigned char)*s++;
    return h & (SYM_TABLE_SIZE - 1);
}

SymbolTable* sym_table_init(void) {
    SymbolTable* st = calloc(1, sizeof(SymbolTable));
    return st;   /* calloc pone buckets[] en NULL */
}

void sym_table_free(SymbolTable* st) {
    if (!st) return;
    for (int i = 0; i < SYM_TABLE_SIZE; i++) {
        SymbolEntry* e = st->buckets[i];
        while (e) {
            SymbolEntry* next = e->next;
            free(e->name);
            free(e);
            e = next;
        }
    }
    free(st);
}

SymbolEntry* sym_table_insert(SymbolTable* st, const char* name, SymbolKind kind) {
    if (!st || !name) return NULL;
    unsigned int idx = hash(name);

    /* ¿ya existe? — actualizar y devolver */
    for (SymbolEntry* e = st->buckets[idx]; e; e = e->next) {
        if (strcmp(e->name, name) == 0) {
            e->kind    = kind;
            e->defined = 1;
            return e;
        }
    }

    /* entrada nueva */
    SymbolEntry* e = malloc(sizeof(SymbolEntry));
    if (!e) return NULL;
    e->name    = strdup(name);
    e->kind    = kind;
    e->defined = 1;
    e->next    = st->buckets[idx];
    st->buckets[idx] = e;
    st->count++;
    return e;
}

SymbolEntry* sym_table_lookup(const SymbolTable* st, const char* name) {
    if (!st || !name) return NULL;
    unsigned int idx = hash(name);
    for (SymbolEntry* e = st->buckets[idx]; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e;
    return NULL;
}

void sym_table_print(const SymbolTable* st) {
    if (!st) return;
    printf("=== Tabla de simbolos (%d entradas) ===\n", st->count);
    for (int i = 0; i < SYM_TABLE_SIZE; i++) {
        for (SymbolEntry* e = st->buckets[i]; e; e = e->next) {
            printf("  [%-12s]  kind=%-10s  defined=%d\n",
                   e->name,
                   e->kind == SYM_VAR ? "VAR" : "FUNCTION",
                   e->defined);
        }
    }
}
