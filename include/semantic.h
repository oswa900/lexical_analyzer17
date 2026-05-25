#ifndef SEMANTIC_H
#define SEMANTIC_H

// Analizador semantico: recorre el AST, llena la tabla de simbolos
// y reporta errores E_SEM_01..E_SEM_05 via ErrorStack.

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
