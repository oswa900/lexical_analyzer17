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
        || strcmp(p->current->lexeme, kw) != 0) {
        return 0;
    }
    advance(p);
    return 1;
}

static int expect_operation(Parser* p, const char* op) {
    if (!p->current || p->current->type != OPERATION
        || strcmp(p->current->lexeme, op) != 0) {
        return 0;
    }
    advance(p);
    return 1;
}

static ASTNode* parse_statement(Parser* p);
static ASTNode* parse_assignment(Parser* p);
static ASTNode* parse_if_statement(Parser* p);
static ASTNode* parse_condition(Parser* p);
static ASTNode* parse_expression(Parser* p);
static ASTNode* parse_term(Parser* p);
static ASTNode* parse_factor(Parser* p);

//productions

//PROGRAM -> STATEMENT+

ASTNode* parse_program(Parser* p) {
    if (!p->current) return NULL;

    ASTNode* program = make_node(NODE_PROGRAM);
    ASTNode* tail = NULL;

    while (p->current != NULL) {
        ASTNode* stmt = parse_statement(p);
        if (!stmt) {
            ast_free(program);
            return NULL;
        }
        if (!program->left) {
            program->left = stmt;   
            tail = stmt;
        } else {
            tail->next = stmt;      
            tail = stmt;
        }
    }

    return program;
}



//STATEMENT -> ASSIGNMENT | IF_STATEMENT | EXPRESSION
static ASTNode* parse_statement(Parser* p) {
    if (!p->current) return NULL;


      if (current_type(p) == IDENTIFIER && p->current->next
        && p->current->next->type == OPERATION
        && strcmp(p->current->next->lexeme, "=") == 0) {
        return parse_assignment(p);
    }

      if (current_type(p) == KEYWORD && strcmp(current_lexeme(p), "if") == 0) {
        return parse_if_statement(p);
    }

      return parse_expression(p);
}

// ASSIGNMENT -> IDENTIFIER "=" EXPRESSION
static ASTNode* parse_assignment(Parser* p) {
    ASTNode* node = make_node(NODE_ASSIGNMENT);

    /* IDENTIFIER */
    node->value = strdup(p->current->lexeme);
    advance(p);

    /* "=" */
    if (!expect_operation(p, "=")) { ast_free(node); return NULL; }

    /* EXPRESSION */
    node->left = parse_expression(p);
    if (!node->left) { ast_free(node); return NULL; }

    return node;
}


//IF_STATEMENT -> "if" CONDITION "then" STATEMENT ("else" STATEMENT)?

static ASTNode* parse_if_statement(Parser* p) {
    ASTNode* node = make_node(NODE_IF_STATEMENT);

    /* "if" */
    if (!expect_keyword(p, "if"))  { ast_free(node); return NULL; }

    /* CONDITION */
    node->condition = parse_condition(p);
    if (!node->condition) { ast_free(node); return NULL; }

    /* "then" */
    if (!expect_keyword(p, "then")) { ast_free(node); return NULL; }

    /* STATEMENT (rama then) */
    node->then_branch = parse_statement(p);
    if (!node->then_branch) { ast_free(node); return NULL; }

     /* ("else" STATEMENT)? */
    if (p->current && p->current->type == KEYWORD
        && strcmp(p->current->lexeme, "else") == 0) {
        advance(p);
        node->else_branch = parse_statement(p);
        if (!node->else_branch) { ast_free(node); return NULL; }
    }

    return node;
}

//CONDITION -> EXPRESSION CONDITIONAL EXPRESSION
// CONDITIONAL -> "<" | ">"

static ASTNode* parse_condition(Parser* p) {
    ASTNode* node = make_node(NODE_CONDITION);

    node->left = parse_expression(p);
    if (!node->left) { ast_free(node); return NULL; }

    if (!p->current || p->current->type != OPERATION
        || (strcmp(p->current->lexeme, "<") != 0
           && strcmp(p->current->lexeme, ">") != 0)) {
        ast_free(node);
        return NULL;
    }
    node->value = strdup(p->current->lexeme);
    advance(p);

    node->right = parse_expression(p);
    if (!node->right) { ast_free(node); return NULL; }

    return node;
}
//EXPRESSION -> TERM (( "+" | "-" ) TERM)*

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

 //TERM -> FACTOR (( "*" | "/" ) FACTOR)*

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

 /* FACTOR -> IDENTIFIER */
 /*         | NUMBER */
 /*         | STRING */
 /*         | "(" EXPRESSION ")" */

static ASTNode* parse_factor(Parser* p) {
    if (!p->current) return NULL;

    /* IDENTIFIER — E-SIN-01: funcion de shell usada sin parentesis de apertura */
    if (current_type(p) == IDENTIFIER) {
        if (p->paren_depth == 0 && is_valid_function(p->current->lexeme))
            error_push(p->errors, E_SIN_01, p->current->lexeme);
        ASTNode* n = make_node(NODE_FACTOR_ID);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }

    /* NUMBER */
    if (current_type(p) == NUMBER) {
        ASTNode* n = make_node(NODE_FACTOR_NUMBER);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }

    /* STRING */
    if (current_type(p) == STRING) {
        ASTNode* n = make_node(NODE_FACTOR_STRING);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }

    /* RUTA */
    if (current_type(p) == RUTA) {
        ASTNode* n = make_node(NODE_FACTOR_ID);
        n->value = strdup(p->current->lexeme);
        advance(p);
        return n;
    }

    /* "(" EXPRESSION ")" */
    if (current_type(p) == SYMBOLS && p->current->lexeme
        && strcmp(p->current->lexeme, "(") == 0) {
        p->paren_depth++;
        advance(p);

        ASTNode* inner = parse_expression(p);
        if (!inner) { p->paren_depth--; return NULL; }

        if (!p->current || strcmp(p->current->lexeme, ")") != 0) {
            ast_free(inner);
            p->paren_depth--;
            return NULL;
        }
        advance(p);
        p->paren_depth--;

        ASTNode* n = make_node(NODE_FACTOR_EXPR);
        n->left = inner;
        return n;
    }

    return NULL;
}

Parser* parser_init(List* token_list, ErrorStack* errors) {
    if (!token_list) return NULL;
    Parser* p = malloc(sizeof(Parser));
    if (!p) return NULL;
    p->current     = token_list->head;
    p->errors      = errors;
    p->paren_depth = 0;
    return p;
}

void parser_free(Parser* p) {
    free(p);
}

//debug
static const char* node_type_name(NodeType t) {
    switch(t) {
        case NODE_PROGRAM:      return "PROGRAM";
        case NODE_ASSIGNMENT:   return "ASSIGNMENT";
        case NODE_IF_STATEMENT: return "IF";
        case NODE_CONDITION:    return "CONDITION";
        case NODE_EXPRESSION:   return "EXPRESSION";
        case NODE_TERM:         return "TERM";
        case NODE_FACTOR_ID:    return "ID";
        case NODE_FACTOR_NUMBER:return "NUMBER";
        case NODE_FACTOR_STRING:return "STRING";
        case NODE_FACTOR_EXPR:  return "PAREN_EXPR";
        case NODE_BINOP:        return "BINOP";
        default:                return "UNKNOWN";
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
        case NODE_CONDITION:
        case NODE_BINOP:
            ast_print(node->left,  indent + 1);
            ast_print(node->right, indent + 1);
            break;
        case NODE_FACTOR_EXPR:
            ast_print(node->left, indent + 1);
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
            while (s) {
                ASTNode* tmp = s->next;
                ast_free(s);
                s = tmp;
            }
            break;
        }
        case NODE_IF_STATEMENT:
            ast_free(node->condition);
            ast_free(node->then_branch);
            ast_free(node->else_branch);
            break;
        default:
            ast_free(node->left);
            ast_free(node->right);
            break;
    }

    free(node);
}
