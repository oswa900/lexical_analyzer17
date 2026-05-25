#ifndef CODEGEN_H
#define CODEGEN_H

// Generacion de Codigo Intermedio de Tres Direcciones (TAC)

#include "parser.h"

typedef enum {
    TAC_ASSIGN,      /* result = arg1            */
    TAC_ADD,         /* result = arg1 + arg2     */
    TAC_SUB,         /* result = arg1 - arg2     */
    TAC_MUL,         /* result = arg1 * arg2     */
    TAC_DIV,         /* result = arg1 / arg2     */
    TAC_LT,          /* result = arg1 < arg2     */
    TAC_GT,          /* result = arg1 > arg2     */
    TAC_LE,          /* result = arg1 <= arg2    */
    TAC_GE,          /* result = arg1 >= arg2    */
    TAC_EQ,          /* result = arg1 == arg2    */
    TAC_NE,          /* result = arg1 != arg2    */
    TAC_AND,         /* result = arg1 && arg2    */
    TAC_OR,          /* result = arg1 || arg2    */
    TAC_NEG,         /* result = -arg1           */
    TAC_PRINT,       /* print arg1               */
    TAC_SYSCALL,     /* syscall arg1 (cmd str)   */
    TAC_LABEL,       /* result:                  */
    TAC_GOTO,        /* goto result              */
    TAC_IFNOT_GOTO,  /* ifnot arg1 goto result   */
} TACOp;

typedef struct TACInstr {
    TACOp             op;
    char*             result;
    char*             arg1;
    char*             arg2;
    struct TACInstr*  next;
} TACInstr;

typedef struct {
    TACInstr* head;
    TACInstr* tail;
    int       temp_count;
    int       label_count;
} TACList;

TACList* tac_generate(ASTNode* program);
void     tac_print(const TACList* tac);
void     tac_free(TACList* tac);

#endif
