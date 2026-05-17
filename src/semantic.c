#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/semantic.h"
#include "../include/dfa.h"      /* is_valid_function() */

/* ---------------------------------------------------------------
 * Prototipos internos (un visitor por tipo de nodo)
 * --------------------------------------------------------------- */
static void analyze_node(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_program(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_assignment(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_if(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_while(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_for(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_condition(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_binop(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_factor_id(SemanticAnalyzer* sa, ASTNode* node);

/* ---------------------------------------------------------------
 * Ciclo de vida
 * --------------------------------------------------------------- */
SemanticAnalyzer* semantic_init(ErrorStack* errors) {
    SemanticAnalyzer* sa = malloc(sizeof(SemanticAnalyzer));
    if (!sa) return NULL;
    sa->symbols = sym_table_init();
    sa->errors  = errors;
    if (!sa->symbols) { free(sa); return NULL; }
    return sa;
}

void semantic_free(SemanticAnalyzer* sa) {
    if (!sa) return;
    sym_table_free(sa->symbols);
    free(sa);
}

/* ---------------------------------------------------------------
 * Punto de entrada público
 * --------------------------------------------------------------- */
int semantic_analyze(SemanticAnalyzer* sa, ASTNode* program) {
    if (!sa || !program) return 0;
    int errors_before = sa->errors ? sa->errors->count : 0;
    analyze_node(sa, program);
    int errors_after  = sa->errors ? sa->errors->count : 0;
    return (errors_after == errors_before);   /* 1 = sin errores nuevos */
}

/* ---------------------------------------------------------------
 * Dispatcher central
 * --------------------------------------------------------------- */
static void analyze_node(SemanticAnalyzer* sa, ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case NODE_PROGRAM:         analyze_program(sa, node);    break;
        case NODE_ASSIGNMENT:      analyze_assignment(sa, node); break;
        case NODE_IF_STATEMENT:    analyze_if(sa, node);         break;
        case NODE_WHILE_STATEMENT: analyze_while(sa, node);      break;
        case NODE_FOR_STATEMENT:   analyze_for(sa, node);        break;
        case NODE_CONDITION:       analyze_condition(sa, node);  break;
        /* LOGICAL tiene la misma estructura que CONDITION (left/right) */
        case NODE_LOGICAL:         analyze_condition(sa, node);  break;
        case NODE_BINOP:           analyze_binop(sa, node);      break;
        case NODE_FACTOR_ID:       analyze_factor_id(sa, node);  break;
        case NODE_FACTOR_EXPR:     analyze_node(sa, node->left); break;
        case NODE_ECHO:            analyze_node(sa, node->left); break;
        case NODE_UNARY_NEG:       analyze_node(sa, node->left); break;
        /* NUMBER y STRING son literales — no requieren verificacion */
        default: break;
    }
}

/* ---------------------------------------------------------------
 * Visitors
 * --------------------------------------------------------------- */

/* PROGRAM: recorre cada sentencia en orden */
static void analyze_program(SemanticAnalyzer* sa, ASTNode* node) {
    for (ASTNode* stmt = node->left; stmt; stmt = stmt->next)
        analyze_node(sa, stmt);
}

/* ASSIGNMENT: primero analiza la expresión (RHS), luego declara la variable.
   Esto detecta usos del tipo  x = x + 1  cuando x no fue declarada antes. */
static void analyze_assignment(SemanticAnalyzer* sa, ASTNode* node) {
    /* RHS */
    analyze_node(sa, node->left);

    /* Declarar / actualizar la variable en la tabla */
    if (node->value)
        sym_table_insert(sa->symbols, node->value, SYM_VAR);
}

/* IF: analiza condicion, rama then y rama else (si existe) */
static void analyze_if(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->condition);
    analyze_node(sa, node->then_branch);
    if (node->else_branch)
        analyze_node(sa, node->else_branch);
}

/* WHILE: analiza condicion y cuerpo */
static void analyze_while(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->condition);
    analyze_node(sa, node->then_branch);
}

/* FOR: registra la variable de iteracion como definida ANTES de analizar
   el cuerpo, para que "for i = 1 to 5 do ... i ..." no genere E-SEM-01. */
static void analyze_for(SemanticAnalyzer* sa, ASTNode* node) {
    /* expresion inicial — puede referenciar variables ya definidas */
    analyze_node(sa, node->left);
    /* la variable de iteracion queda definida por el for */
    if (node->value)
        sym_table_insert(sa->symbols, node->value, SYM_VAR);
    /* expresion limite */
    analyze_node(sa, node->right);
    /* cuerpo */
    analyze_node(sa, node->then_branch);
}

/* CONDITION: analiza los dos operandos */
static void analyze_condition(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    analyze_node(sa, node->right);
}

/* BINOP: analiza los dos operandos */
static void analyze_binop(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    analyze_node(sa, node->right);
}

/* FACTOR_ID: el caso más importante.
   - Si es un comando conocido → registrar como SYM_FUNCTION (ya fue
     chequeado sintácticamente en parser.c; aquí solo lo catalogamos).
   - Si es un identificador normal → verificar que haya sido declarado
     antes; si no, E_SEM_01. */
static void analyze_factor_id(SemanticAnalyzer* sa, ASTNode* node) {
    if (!node->value) return;

    if (is_valid_function(node->value)) {
        /* El lexer/parser ya emitió E_SIN_01 si faltaba '('.
           Aquí solo nos aseguramos de que esté en la tabla. */
        sym_table_insert(sa->symbols, node->value, SYM_FUNCTION);
        return;
    }

    /* Variable: debe haber sido asignada antes */
    SymbolEntry* entry = sym_table_lookup(sa->symbols, node->value);
    if (!entry || !entry->defined) {
        error_push(sa->errors, E_SEM_01, node->value);
    }
}
