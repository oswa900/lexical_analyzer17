#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/semantic.h"
#include "../include/dfa.h"

// Reglas semanticas por comando: aridad y tipo de argumento esperado
typedef struct {
    const char* name;
    int min_args;     /* E_SEM_01 si recibe menos */
    int max_args;     /* E_SEM_02 si recibe mas; -1 = ilimitado */
    int expect_ruta;  /* 1=RUTA obligatorio */
    int check_exists; /* 1=la ruta debe existir en el FS (E_SEM_04) */
} CmdRule;

static const CmdRule CMD_RULES[] = {
    { "ls",    0, 1,  1,  1 },
    { "mkdir", 1, 1,  1,  0 },
    { "touch", 1, 1,  1,  0 },
    { "edit",  1, 1,  1,  1 },
    { "rm",    1, 1,  1,  1 },
    { "help",  0, 0, -1,  0 },
    { "clear", 0, 0, -1,  0 },
    { "vim",   1, 1,  1,  0 },
};
#define N_CMD_RULES (int)(sizeof(CMD_RULES) / sizeof(CMD_RULES[0]))

// Prototipos internos
static void analyze_node(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_program(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_assignment(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_if(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_while(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_for(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_condition(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_binop(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_factor_id(SemanticAnalyzer* sa, ASTNode* node);
static void analyze_shell_cmd(SemanticAnalyzer* sa, ASTNode* node);

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

int semantic_analyze(SemanticAnalyzer* sa, ASTNode* program) {
    if (!sa || !program) return 0;
    int errors_before = sa->errors ? sa->errors->count : 0;
    analyze_node(sa, program);
    int errors_after  = sa->errors ? sa->errors->count : 0;
    return (errors_after == errors_before);
}

static void analyze_node(SemanticAnalyzer* sa, ASTNode* node) {
    if (!node) return;
    switch (node->type) {
        case NODE_PROGRAM:         analyze_program(sa, node);    break;
        case NODE_ASSIGNMENT:      analyze_assignment(sa, node); break;
        case NODE_IF_STATEMENT:    analyze_if(sa, node);         break;
        case NODE_WHILE_STATEMENT: analyze_while(sa, node);      break;
        case NODE_FOR_STATEMENT:   analyze_for(sa, node);        break;
        case NODE_CONDITION:       analyze_condition(sa, node);  break;
        case NODE_LOGICAL:         analyze_condition(sa, node);  break;
        case NODE_BINOP:           analyze_binop(sa, node);      break;
        case NODE_FACTOR_ID:       analyze_factor_id(sa, node);  break;
        case NODE_FACTOR_EXPR:     analyze_node(sa, node->left); break;
        case NODE_ECHO:            analyze_node(sa, node->left); break;
        case NODE_UNARY_NEG:       analyze_node(sa, node->left); break;
        case NODE_SHELL_CMD:
            sym_table_insert(sa->symbols, node->value, SYM_FUNCTION);
            analyze_shell_cmd(sa, node);
            break;
        default: break;
    }
}

static void analyze_program(SemanticAnalyzer* sa, ASTNode* node) {
    for (ASTNode* stmt = node->left; stmt; stmt = stmt->next)
        analyze_node(sa, stmt);
}

// RHS se analiza antes de declarar la variable para detectar x = x + 1 cuando x no existe
static void analyze_assignment(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    if (node->value)
        sym_table_insert(sa->symbols, node->value, SYM_VAR);
}

static void analyze_if(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->condition);
    analyze_node(sa, node->then_branch);
    if (node->else_branch)
        analyze_node(sa, node->else_branch);
}

static void analyze_while(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->condition);
    analyze_node(sa, node->then_branch);
}

// La variable de iteracion se registra antes del cuerpo para que sea valida dentro del for
static void analyze_for(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    if (node->value)
        sym_table_insert(sa->symbols, node->value, SYM_VAR);
    analyze_node(sa, node->right);
    analyze_node(sa, node->then_branch);
}

static void analyze_condition(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    analyze_node(sa, node->right);
}

static void analyze_binop(SemanticAnalyzer* sa, ASTNode* node) {
    analyze_node(sa, node->left);
    analyze_node(sa, node->right);
}

static void analyze_shell_cmd(SemanticAnalyzer* sa, ASTNode* node) {
    if (!node || !node->value) return;

    const CmdRule* rule = NULL;
    for (int i = 0; i < N_CMD_RULES; i++) {
        if (strcmp(CMD_RULES[i].name, node->value) == 0) {
            rule = &CMD_RULES[i];
            break;
        }
    }
    if (!rule) return;

    int argc = 0;
    for (ASTNode* a = node->left; a; a = a->next) argc++;

    if (argc < rule->min_args) {
        error_push(sa->errors, E_SEM_01, node->value);
        return;
    }
    if (rule->max_args >= 0 && argc > rule->max_args) {
        error_push(sa->errors, E_SEM_02, node->value);
        return;
    }

    for (ASTNode* a = node->left; a; a = a->next) {
        if (a->type == NODE_FACTOR_ID) {
            analyze_factor_id(sa, a);
            continue;
        }
        if (rule->expect_ruta == 1 && a->type == NODE_FACTOR_STRING) {
            error_push(sa->errors, E_SEM_03, a->value);
            continue;
        }
        if (rule->check_exists && a->type == NODE_FACTOR_RUTA && a->value) {
            if (access(a->value, F_OK) != 0)
                error_push(sa->errors, E_SEM_04, a->value);
        }
    }
}

static void analyze_factor_id(SemanticAnalyzer* sa, ASTNode* node) {
    if (!node->value) return;

    if (is_valid_function(node->value)) {
        sym_table_insert(sa->symbols, node->value, SYM_FUNCTION);
        return;
    }

    // sym_table_insert siempre marca defined=1; NULL significa variable no declarada
    if (!sym_table_lookup(sa->symbols, node->value))
        error_push(sa->errors, E_SEM_05, node->value);
}
