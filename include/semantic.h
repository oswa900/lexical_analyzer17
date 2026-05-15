#ifndef SEMANTIC_H
#define SEMANTIC_H

/* ---------------------------------------------------------------
 * Analizador semántico
 *
 * Recorre el AST producido por parser.c, llena la tabla de
 * símbolos y reporta errores semánticos via ErrorStack.
 *
 * Errores semánticos definidos:
 *   E_SEM_01 — variable usada antes de ser asignada
 * --------------------------------------------------------------- */

#include "../include/parser.h"
#include "../include/error.h"
#include "../include/symbol_table.h"

typedef struct {
    SymbolTable* symbols;
    ErrorStack*  errors;
} SemanticAnalyzer;

/* Ciclo de vida */
SemanticAnalyzer* semantic_init(ErrorStack* errors);
void              semantic_free(SemanticAnalyzer* sa);

/* Punto de entrada: analiza todo el programa.
   Devuelve 1 si no hubo errores semánticos, 0 si hubo alguno. */
int semantic_analyze(SemanticAnalyzer* sa, ASTNode* program);

#endif /* SEMANTIC_H */
