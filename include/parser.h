#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "list.h"
#include "error.h"

typedef enum NodeType {
    NODE_PROGRAM,
    NODE_ASSIGNMENT,
    NODE_IF_STATEMENT,
    NODE_CONDITION,
    NODE_EXPRESSION,
    NODE_TERM,
    NODE_FACTOR_ID,
    NODE_FACTOR_NUMBER,
    NODE_FACTOR_STRING,
    NODE_FACTOR_EXPR,
    NODE_BINOP,
} NodeType;

typedef struct ASTNode {
    NodeType type;

    char* value;

    struct ASTNode* left;
    struct ASTNode* right;

    struct ASTNode* condition;
    struct ASTNode* then_branch;
    struct ASTNode* else_branch;

    struct ASTNode* next;
} ASTNode;

typedef struct Parser {
    Token*      current;
    ErrorStack* errors;
    int         paren_depth;
} Parser;

Parser* parser_init(List* token_list, ErrorStack* errors);

void    parser_free(Parser* p);

ASTNode* parse_program(Parser* p);

void ast_print(ASTNode* node, int indent);

void ast_free(ASTNode* node);

#endif
