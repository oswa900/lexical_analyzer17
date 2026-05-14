#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/error.h"

ErrorStack* error_stack_init(void) {
    ErrorStack* es = malloc(sizeof(ErrorStack));
    if (!es) return NULL;
    es->top   = NULL;
    es->count = 0;
    return es;
}

void error_push(ErrorStack* es, ErrorCode code, const char* lexeme) {
    if (!es) return;
    Error* e = malloc(sizeof(Error));
    if (!e) return;
    e->code   = code;
    e->lexeme = lexeme ? strdup(lexeme) : NULL;
    e->next   = es->top;
    es->top   = e;
    es->count++;
}

Error* error_pop(ErrorStack* es) {
    if (!es || !es->top) return NULL;
    Error* e = es->top;
    es->top  = e->next;
    e->next  = NULL;
    es->count--;
    return e;
}

static void print_one(const Error* e) {
    switch (e->code) {
        case E_LEX_01:
            printf("Error lexico [E-LEX-01]: '%s' no es una funcion valida.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_02:
            printf("Error lexico [E-LEX-02]: Cadena sin cerrar: %s\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_03:
            printf("Error lexico [E-LEX-03]: Caracter no valido en RUTA: '%s'.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_04:
            printf("Error lexico [E-LEX-04]: Token '%s' no reconocido.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SIN_01:
            printf("Error sintactico [E-SIN-01]: Se esperaba '(' antes de '%s'.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        default:
            printf("Error desconocido (codigo %d): '%s'\n",
                   (int)e->code, e->lexeme ? e->lexeme : "?");
            break;
    }
}

void error_stack_print(const ErrorStack* es) {
    if (!es || !es->top) {
        printf("No se encontraron errores.\n");
        return;
    }
    printf("=== Errores encontrados (%d) ===\n", es->count);

    /* Collect pointers to print in insertion order (oldest first). */
    Error** arr = malloc((size_t)es->count * sizeof(Error*));
    if (!arr) return;
    int n = 0;
    for (Error* cur = es->top; cur; cur = cur->next)
        arr[n++] = cur;
    for (int i = n - 1; i >= 0; i--)
        print_one(arr[i]);
    free(arr);
}

void error_stack_free(ErrorStack* es) {
    if (!es) return;
    Error* cur = es->top;
    while (cur) {
        Error* next = cur->next;
        free(cur->lexeme);
        free(cur);
        cur = next;
    }
    free(es);
}

int error_stack_empty(const ErrorStack* es) {
    return !es || !es->top;
}
