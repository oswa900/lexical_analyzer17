#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/list.h"
#include "../include/dfa.h"

static ASTNode* make_node(NodeType type) {
    ASTNode* n = calloc(1, sizeof(ASTNode));
    if (!n) { fprintf(stderr, "[parser] out of memory\n"); exit(1); }
    n->type = type;
    return n;
}

static void advance(Parser* p) {
    if (p->current && p->current->next)
        p->current = p->current->next;
    else
        p->current = NULL;
}

static const char* current_lexeme(Parser* p) {
    return (p->current) ? p->current->lexeme : NULL;
}

static Token_type current_type(Parser* p) {
    return (p->current) ? p->current->type : UNKNOWN;
}

static int expect_keyword(Parser* p, const char* kw) {
    if (!p->current || p->current->type != KEYWORD
        || strcmp(p->current->lexeme, kw) != 0)
        return 0;
    advance(p);
    return 1;
}

static int expect_operation(Parser* p, const char* op) {
    if (!p->current || p->current->type != OPERATION
        || strcmp(p->current->lexeme, op) != 0)
        return 0;
    advance(p);
    return 1;
}

static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_assignment(Parser* p);
static ASTNode* parse_if_statement(Parser* p);
static ASTNode* parse_while_statement(Parser* p);
static ASTNode* parse_for_statement(Parser* p);
static ASTNode* parse_echo(Parser* p);
static ASTNode* parse_shell_cmd(Parser* p);
static ASTNode* parse_arg(Parser* p);
static ASTNode* parse_logical(Parser* p);
static ASTNode* parse_condition(Parser* p);
static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_term(Parser* p);
static ASTNode* parse_factor(Parser* p);

// ─── producciones ────────────────────────────────────────────────────────────

// PROGRAM -> STATEMENT+
ASTNode* parse_program(Parser* p) {
    if (!p->current) return NULL;
    ASTNode* program = make_node(NODE_PROGRAM);
    ASTNode* tail = NULL;
    while (p->current) {
        ASTNode* stmt = parse_statement(p);
        if (!stmt) { ast_free(program); return NULL; }
        if (!program->left) { program->left = stmt; tail = stmt; }
        else                { tail->next = stmt;    tail = stmt; }
    }
    return program;
}

// STATEMENT -> ASSIGNMENT | IF | WHILE | FOR | EXPRESSION
static ASTNode* parse_statement(Parser* p) {
    if (!p->current) return NULL;

    if (current_type(p) == IDENTIFIER && p->current->next
        && p->current->next->type == OPERATION
        && strcmp(p->current->next->lexeme, "=") == 0)
        return parse_assignment(p);

    if (current_type(p) == KEYWORD && strcmp(current_lexeme(p), "if")    == 0)
        return parse_if_statement(p);
    if (current_type(p) == KEYWORD && strcmp(current_lexeme(p), "while") == 0)
        return parse_while_statement(p);
    if (current_type(p) == KEYWORD && strcmp(current_lexeme(p), "for")   == 0)
        return parse_for_statement(p);
    if (current_type(p) == KEYWORD && strcmp(current_lexeme(p), "echo")  == 0)
        return parse_echo(p);

    /* EXPRESION_S → ( FUNCION ARGUMENTOS )
       Lookahead: '(' seguido de un identificador de funcion valida. */
    if (current_type(p) == SYMBOLS && strcmp(current_lexeme(p), "(") == 0
        && p->current->next && p->current->next->type == IDENTIFIER) {
        Token* next = p->current->next;
        if (is_valid_function(next->lexeme))
            return parse_shell_cmd(p);
        /* E-LEX-01: identificador en posicion de FUNCION que no es valido.
           Solo se activa si el siguiente token parece argumento (RUTA, STRING o ')'),
           para no confundirlo con expresiones matematicas como (x + 5). */
        Token* after = next->next;
        if (after && (after->type == RUTA || after->type == STRING
            || (after->type == SYMBOLS && strcmp(after->lexeme, ")") == 0))) {
            error_push(p->errors, E_LEX_01, next->lexeme);
            /* recuperacion: consumir hasta ')' */
            advance(p);  /* '(' */
            int depth = 1;
            while (p->current && depth > 0) {
                if (p->current->type == SYMBOLS && strcmp(p->current->lexeme, "(") == 0) depth++;
                if (p->current->type == SYMBOLS && strcmp(p->current->lexeme, ")") == 0) depth--;
                advance(p);
            }
            return NULL;
        }
    }

    return parse_expression(p);
}

// ASSIGNMENT -> IDENTIFIER "=" EXPRESSION
static ASTNode* parse_assignment(Parser* p) {
    ASTNode* node = make_node(NODE_ASSIGNMENT);
    node->value = strdup(p->current->lexeme);
    advance(p);
    if (!expect_operation(p, "="))  { ast_free(node); return NULL; }
    node->left = parse_expression(p);
    if (!node->left)                { ast_free(node); return NULL; }
    return node;
}

// IF_STATEMENT -> "if" "("? LOGICAL ")"? "then" STATEMENT ("else" STATEMENT)?
static ASTNode* parse_if_statement(Parser* p) {
    ASTNode* node = make_node(NODE_IF_STATEMENT);
    if (!expect_keyword(p, "if"))   { ast_free(node); return NULL; }

    int has_paren = (p->current && p->current->type == SYMBOLS
                     && strcmp(p->current->lexeme, "(") == 0);
    if (has_paren) { p->paren_depth++; advance(p); }

    node->condition = parse_logical(p);
    if (!node->condition)           { ast_free(node); return NULL; }

    if (has_paren) {
        if (p->current && p->current->type == SYMBOLS
            && strcmp(p->current->lexeme, ")") == 0)
            advance(p);
        p->paren_depth--;
    }

    if (!expect_keyword(p, "then")) { ast_free(node); return NULL; }

    node->then_branch = parse_statement(p);
    if (!node->then_branch)         { ast_free(node); return NULL; }

    if (p->current && p->current->type == KEYWORD
        && strcmp(p->current->lexeme, "else") == 0) {
        advance(p);
        node->else_branch = parse_statement(p);
        if (!node->else_branch)     { ast_free(node); return NULL; }
    }
    return node;
}

// WHILE_STATEMENT -> "while" "("? LOGICAL ")"? "do" STATEMENT
static ASTNode* parse_while_statement(Parser* p) {
    ASTNode* node = make_node(NODE_WHILE_STATEMENT);
    if (!expect_keyword(p, "while")) { ast_free(node); return NULL; }

    int has_paren = (p->current && p->current->type == SYMBOLS
                     && strcmp(p->current->lexeme, "(") == 0);
    if (has_paren) { p->paren_depth++; advance(p); }

    node->condition = parse_logical(p);
    if (!node->condition)            { ast_free(node); return NULL; }

    if (has_paren) {
        if (p->current && p->current->type == SYMBOLS
            && strcmp(p->current->lexeme, ")") == 0)
            advance(p);
        p->paren_depth--;
    }

    if (!expect_keyword(p, "do"))    { ast_free(node); return NULL; }

    node->then_branch = parse_statement(p);
    if (!node->then_branch)          { ast_free(node); return NULL; }
    return node;
}

// FOR_STATEMENT -> "for" IDENTIFIER "=" EXPRESSION "to" EXPRESSION "do" STATEMENT
static ASTNode* parse_for_statement(Parser* p) {
    ASTNode* node = make_node(NODE_FOR_STATEMENT);
    if (!expect_keyword(p, "for"))   { ast_free(node); return NULL; }

    if (!p->current || current_type(p) != IDENTIFIER) { ast_free(node); return NULL; }
    node->value = strdup(p->current->lexeme);
    advance(p);

    if (!expect_operation(p, "="))   { ast_free(node); return NULL; }

    node->left = parse_expression(p);
    if (!node->left)                 { ast_free(node); return NULL; }

    if (!expect_keyword(p, "to"))    { ast_free(node); return NULL; }

    node->right = parse_expression(p);
    if (!node->right)                { ast_free(node); return NULL; }

    if (!expect_keyword(p, "do"))    { ast_free(node); return NULL; }

    node->then_branch = parse_statement(p);
    if (!node->then_branch)          { ast_free(node); return NULL; }
    return node;
}

// ECHO -> "echo" EXPRESSION
static ASTNode* parse_echo(Parser* p) {
    ASTNode* node = make_node(NODE_ECHO);
    advance(p);  /* consume "echo" */
    node->left = parse_expression(p);
    if (!node->left) { ast_free(node); return NULL; }
    return node;
}

/* ATOMO → CADENA | RUTA | IDENTIFIER | NUMBER (argumento de comando shell)
   RUTA y CADENA se almacenan como NODE_FACTOR_STRING (literales, sin verificacion
   semantica). IDENTIFIER como NODE_FACTOR_ID (variable, se verifica). */
static ASTNode* parse_arg(Parser* p) {
    if (!p->current) return NULL;
    NodeType t;
    switch (current_type(p)) {
        case STRING:
        case RUTA:       t = NODE_FACTOR_STRING; break;
        case IDENTIFIER: t = NODE_FACTOR_ID;     break;
        case NUMBER:     t = NODE_FACTOR_NUMBER; break;
        default: return NULL;
    }
    ASTNode* n = make_node(t);
    n->value = strdup(p->current->lexeme);
    advance(p);
    return n;
}

// EXPRESION_S → ( FUNCION ARGUMENTOS )
// ARGUMENTOS  → EXPRESION ARGUMENTOS | ε
static ASTNode* parse_shell_cmd(Parser* p) {
    advance(p);  /* consume '(' */

    ASTNode* node = make_node(NODE_SHELL_CMD);
    node->value = strdup(p->current->lexeme);  /* nombre de la funcion */
    advance(p);  /* consume FUNCION */

    /* ARGUMENTOS: cero o mas atomos hasta ')' */
    ASTNode* tail = NULL;
    while (p->current
           && !(p->current->type == SYMBOLS && strcmp(p->current->lexeme, ")") == 0)) {
        ASTNode* arg = parse_arg(p);
        if (!arg) { ast_free(node); return NULL; }
        if (!node->left) { node->left = arg; tail = arg; }
        else             { tail->next = arg; tail = arg; }
    }

    /* ')' */
    if (!p->current || strcmp(p->current->lexeme, ")") != 0) {
        error_push(p->errors, E_SIN_01, node->value);
        ast_free(node); return NULL;
    }
    advance(p);
    return node;
}

// CONDITION -> EXPRESSION RELOP EXPRESSION
// RELOP -> "<" | ">" | "<=" | ">=" | "==" | "!="
static ASTNode* parse_condition(Parser* p) {
    ASTNode* node = make_node(NODE_CONDITION);

    node->left = parse_expression(p);
    if (!node->left) { ast_free(node); return NULL; }

    if (!p->current || p->current->type != OPERATION) { ast_free(node); return NULL; }
    const char* op = p->current->lexeme;
    if (strcmp(op, "<")  != 0 && strcmp(op, ">")  != 0 &&
        strcmp(op, "<=") != 0 && strcmp(op, ">=") != 0 &&
        strcmp(op, "==") != 0 && strcmp(op, "!=") != 0) {
        ast_free(node);
        return NULL;
    }
    node->value = strdup(op);
    advance(p);

    node->right = parse_expression(p);
    if (!node->right) { ast_free(node); return NULL; }
    return node;
}

// LOGICAL -> CONDITION (( "&&" | "||" ) CONDITION)*
static ASTNode* parse_logical(Parser* p) {
    ASTNode* left = parse_condition(p);
    if (!left) return NULL;

    while (p->current && p->current->type == OPERATION
           && (strcmp(p->current->lexeme, "&&") == 0
               || strcmp(p->current->lexeme, "||") == 0)) {
        ASTNode* op_node = make_node(NODE_LOGICAL);
        op_node->value = strdup(p->current->lexeme);
        advance(p);
        ASTNode* right = parse_condition(p);
        if (!right) { ast_free(op_node); ast_free(left); return NULL; }
        op_node->left  = left;
        op_node->right = right;
        left = op_node;
    }
    return left;
}

// EXPRESSION -> TERM (( "+" | "-" ) TERM)*
static ASTNode* parse_expression(Parser* p) {
    ASTNode* left = parse_term(p);
    if (!left) return NULL;

    while (p->current && p->current->type == OPERATION
           && (strcmp(p->current->lexeme, "+") == 0
               || strcmp(p->current->lexeme, "-") == 0)) {
        ASTNode* op_node = make_node(NODE_BINOP);
        op_node->value = strdup(p->current->lexeme);
        advance(p);
        ASTNode* right = parse_term(p);
        if (!right) { ast_free(op_node); ast_free(left); return NULL; }
        op_node->left  = left;
        op_node->right = right;
        left = op_node;
    }
    return left;
}

// TERM -> FACTOR (( "*" | "/" ) FACTOR)*
static ASTNode* parse_term(Parser* p) {
    ASTNode* left = parse_factor(p);
    if (!left) return NULL;

    while (p->current && p->current->type == OPERATION
           && (strcmp(p->current->lexeme, "*") == 0
               || strcmp(p->current->lexeme, "/") == 0)) {
        ASTNode* op_node = make_node(NODE_BINOP);
        op_node->value = strdup(p->current->lexeme);
        advance(p);
        ASTNode* right = parse_factor(p);
        if (!right) { ast_free(op_node); ast_free(left); return NULL; }
        op_node->left  = left;
        op_node->right = right;
        left = op_node;
    }
    return left;
}

// FACTOR -> "-" FACTOR | IDENTIFIER | NUMBER | STRING | RUTA | "(" EXPRESSION ")"
static ASTNode* parse_factor(Parser* p) {
    if (!p->current) return NULL;

    /* negacion unaria */
    if (current_type(p) == OPERATION && strcmp(p->current->lexeme, "-") == 0) {
        advance(p);
        ASTNode* n = make_node(NODE_UNARY_NEG);
        n->left = parse_factor(p);
        if (!n->left) { ast_free(n); return NULL; }
        return n;
    }

    if (current_type(p) == IDENTIFIER) {
        ASTNode* n = make_node(NODE_FACTOR_ID);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }
    if (current_type(p) == NUMBER) {
        ASTNode* n = make_node(NODE_FACTOR_NUMBER);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }
    if (current_type(p) == STRING) {
        ASTNode* n = make_node(NODE_FACTOR_STRING);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }
    if (current_type(p) == RUTA) {
        ASTNode* n = make_node(NODE_FACTOR_ID);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }
    if (current_type(p) == SYMBOLS
        && strcmp(p->current->lexeme, "(") == 0) {
        p->paren_depth++;
        advance(p);
        ASTNode* inner = parse_expression(p);
        if (!inner) { p->paren_depth--; return NULL; }
        if (!p->current || strcmp(p->current->lexeme, ")") != 0) {
            ast_free(inner); p->paren_depth--; return NULL;
        }
        advance(p);
        p->paren_depth--;
        ASTNode* n = make_node(NODE_FACTOR_EXPR);
        n->left = inner;
        return n;
    }
    return NULL;
}

// ─── ciclo de vida ────────────────────────────────────────────────────────────

Parser* parser_init(List* token_list, ErrorStack* errors) {
    if (!token_list) return NULL;
    Parser* p = malloc(sizeof(Parser));
    if (!p) return NULL;
    p->current     = token_list->head;
    p->errors      = errors;
    p->paren_depth = 0;
    return p;
}

void parser_free(Parser* p) { free(p); }

// ─── debug ────────────────────────────────────────────────────────────────────

static const char* node_type_name(NodeType t) {
    switch (t) {
        case NODE_PROGRAM:         return "PROGRAM";
        case NODE_ASSIGNMENT:      return "ASSIGNMENT";
        case NODE_IF_STATEMENT:    return "IF";
        case NODE_WHILE_STATEMENT: return "WHILE";
        case NODE_FOR_STATEMENT:   return "FOR";
        case NODE_CONDITION:       return "CONDITION";
        case NODE_LOGICAL:         return "LOGICAL";
        case NODE_EXPRESSION:      return "EXPRESSION";
        case NODE_TERM:            return "TERM";
        case NODE_FACTOR_ID:       return "ID";
        case NODE_FACTOR_NUMBER:   return "NUMBER";
        case NODE_FACTOR_STRING:   return "STRING";
        case NODE_FACTOR_EXPR:     return "PAREN_EXPR";
        case NODE_BINOP:           return "BINOP";
        case NODE_ECHO:            return "ECHO";
        case NODE_UNARY_NEG:       return "NEG";
        case NODE_SHELL_CMD:       return "SHELL_CMD";
        default:                   return "UNKNOWN";
    }
}

void ast_print(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    printf("[%s]", node_type_name(node->type));
    if (node->value) printf(" \"%s\"", node->value);
    printf("\n");

    switch (node->type) {
        case NODE_PROGRAM:
            for (ASTNode* s = node->left; s; s = s->next)
                ast_print(s, indent + 1);
            break;
        case NODE_ASSIGNMENT:
            ast_print(node->left, indent + 1);
            break;
        case NODE_IF_STATEMENT:
            ast_print(node->condition,   indent + 1);
            ast_print(node->then_branch, indent + 1);
            if (node->else_branch)
                ast_print(node->else_branch, indent + 1);
            break;
        case NODE_WHILE_STATEMENT:
            ast_print(node->condition,   indent + 1);
            ast_print(node->then_branch, indent + 1);
            break;
        case NODE_FOR_STATEMENT:
            ast_print(node->left,        indent + 1);
            ast_print(node->right,       indent + 1);
            ast_print(node->then_branch, indent + 1);
            break;
        case NODE_CONDITION:
        case NODE_LOGICAL:
        case NODE_BINOP:
            ast_print(node->left,  indent + 1);
            ast_print(node->right, indent + 1);
            break;
        case NODE_FACTOR_EXPR:
        case NODE_ECHO:
        case NODE_UNARY_NEG:
            ast_print(node->left, indent + 1);
            break;
        case NODE_SHELL_CMD:
            for (ASTNode* a = node->left; a; a = a->next)
                ast_print(a, indent + 1);
            break;
        default:
            break;
    }
}

void ast_free(ASTNode* node) {
    if (!node) return;
    free(node->value);

    switch (node->type) {
        case NODE_PROGRAM: {
            ASTNode* s = node->left;
            while (s) { ASTNode* tmp = s->next; ast_free(s); s = tmp; }
            break;
        }
        case NODE_IF_STATEMENT:
            ast_free(node->condition);
            ast_free(node->then_branch);
            ast_free(node->else_branch);
            break;
        case NODE_WHILE_STATEMENT:
            ast_free(node->condition);
            ast_free(node->then_branch);
            break;
        case NODE_FOR_STATEMENT:
            ast_free(node->left);
            ast_free(node->right);
            ast_free(node->then_branch);
            break;
        case NODE_SHELL_CMD: {
            ASTNode* a = node->left;
            while (a) { ASTNode* nx = a->next; a->next = NULL; ast_free(a); a = nx; }
            break;
        }
        case NODE_LOGICAL:
        case NODE_CONDITION:
        case NODE_BINOP:
        default:
            ast_free(node->left);
            ast_free(node->right);
            break;
    }
    free(node);
}
