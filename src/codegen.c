#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/codegen.h"

/* ── helpers internos ─────────────────────────────── */

static char* new_temp(TACList* t) {
    char buf[16];
    snprintf(buf, sizeof(buf), "t%d", t->temp_count++);
    return strdup(buf);
}

static char* new_label(TACList* t) {
    char buf[16];
    snprintf(buf, sizeof(buf), "L%d", t->label_count++);
    return strdup(buf);
}

/* Emite una instruccion al final de la lista.
   Todos los strings se duplican; la responsabilidad de liberar
   los originales queda en el llamador. */
static void emit(TACList* t, TACOp op,
                 const char* result, const char* arg1, const char* arg2) {
    TACInstr* instr = malloc(sizeof(TACInstr));
    if (!instr) return;
    instr->op     = op;
    instr->result = result ? strdup(result) : NULL;
    instr->arg1   = arg1   ? strdup(arg1)   : NULL;
    instr->arg2   = arg2   ? strdup(arg2)   : NULL;
    instr->next   = NULL;
    if (!t->head) t->head = t->tail = instr;
    else        { t->tail->next = instr; t->tail = instr; }
}

/* ── prototipos internos ──────────────────────────── */

static char* gen_expr(TACList* tac, ASTNode* node);
static char* gen_cond(TACList* tac, ASTNode* node);
static void  gen_stmt(TACList* tac, ASTNode* node);

/* ── generacion de expresiones ────────────────────── */

/* Retorna el nombre del resultado (temporal o literal).
   El llamador es responsable de free() sobre el string devuelto. */
static char* gen_expr(TACList* tac, ASTNode* node) {
    if (!node) return strdup("?");

    switch (node->type) {
        case NODE_FACTOR_NUMBER:
        case NODE_FACTOR_STRING:
        case NODE_FACTOR_ID:
            return strdup(node->value);

        case NODE_FACTOR_EXPR:
            return gen_expr(tac, node->left);

        case NODE_UNARY_NEG: {
            char* arg = gen_expr(tac, node->left);
            char* t   = new_temp(tac);
            emit(tac, TAC_NEG, t, arg, NULL);
            free(arg);
            return t;
        }

        case NODE_BINOP: {
            char* l = gen_expr(tac, node->left);
            char* r = gen_expr(tac, node->right);
            char* t = new_temp(tac);
            TACOp op;
            if      (strcmp(node->value, "+") == 0) op = TAC_ADD;
            else if (strcmp(node->value, "-") == 0) op = TAC_SUB;
            else if (strcmp(node->value, "*") == 0) op = TAC_MUL;
            else                                     op = TAC_DIV;
            emit(tac, op, t, l, r);
            free(l); free(r);
            return t;   /* el llamador libera t */
        }

        default:
            return strdup("?");
    }
}

/* ── generacion de condiciones ────────────────────── */

static char* gen_cond(TACList* tac, ASTNode* node) {
    if (!node) return strdup("?");

    /* LOGICAL: && / || */
    if (node->type == NODE_LOGICAL) {
        char* l = gen_cond(tac, node->left);
        char* r = gen_cond(tac, node->right);
        char* t = new_temp(tac);
        TACOp op = (strcmp(node->value, "&&") == 0) ? TAC_AND : TAC_OR;
        emit(tac, op, t, l, r);
        free(l); free(r);
        return t;
    }

    /* CONDITION: < > <= >= == != */
    if (node->type == NODE_CONDITION) {
        char* l = gen_expr(tac, node->left);
        char* r = gen_expr(tac, node->right);
        char* t = new_temp(tac);
        TACOp op;
        if      (strcmp(node->value, "<")  == 0) op = TAC_LT;
        else if (strcmp(node->value, ">")  == 0) op = TAC_GT;
        else if (strcmp(node->value, "<=") == 0) op = TAC_LE;
        else if (strcmp(node->value, ">=") == 0) op = TAC_GE;
        else if (strcmp(node->value, "==") == 0) op = TAC_EQ;
        else                                      op = TAC_NE;
        emit(tac, op, t, l, r);
        free(l); free(r);
        return t;
    }

    /* fallback: tratar como expresion */
    return gen_expr(tac, node);
}

/* ── generacion de sentencias ─────────────────────── */

static void gen_stmt(TACList* tac, ASTNode* node) {
    if (!node) return;

    switch (node->type) {

        /* PROGRAMA: iterar sentencias encadenadas */
        case NODE_PROGRAM:
            for (ASTNode* s = node->left; s; s = s->next)
                gen_stmt(tac, s);
            break;

        /* x = expr */
        case NODE_ASSIGNMENT: {
            char* r = gen_expr(tac, node->left);
            emit(tac, TAC_ASSIGN, node->value, r, NULL);
            free(r);
            break;
        }

        /* if cond then S1 [else S2]
           Con else:
             ifnot cond goto L_else
             S1
             goto L_end
           L_else:
             S2
           L_end:

           Sin else:
             ifnot cond goto L_end
             S1
           L_end:                         */
        case NODE_IF_STATEMENT: {
            char* cond = gen_cond(tac, node->condition);
            char* lend = new_label(tac);

            if (node->else_branch) {
                char* lelse = new_label(tac);
                emit(tac, TAC_IFNOT_GOTO, lelse, cond, NULL);
                free(cond);
                gen_stmt(tac, node->then_branch);
                emit(tac, TAC_GOTO,       lend,  NULL, NULL);
                emit(tac, TAC_LABEL,      lelse, NULL, NULL);
                gen_stmt(tac, node->else_branch);
                free(lelse);
            } else {
                emit(tac, TAC_IFNOT_GOTO, lend, cond, NULL);
                free(cond);
                gen_stmt(tac, node->then_branch);
            }

            emit(tac, TAC_LABEL, lend, NULL, NULL);
            free(lend);
            break;
        }

        /* while cond do S
           L_start:
             ifnot cond goto L_end
             S
             goto L_start
           L_end:                         */
        case NODE_WHILE_STATEMENT: {
            char* lstart = new_label(tac);
            char* lend   = new_label(tac);
            emit(tac, TAC_LABEL, lstart, NULL, NULL);
            char* cond = gen_cond(tac, node->condition);
            emit(tac, TAC_IFNOT_GOTO, lend,   cond, NULL);
            free(cond);
            gen_stmt(tac, node->then_branch);
            emit(tac, TAC_GOTO,  lstart, NULL, NULL);
            emit(tac, TAC_LABEL, lend,   NULL, NULL);
            free(lstart); free(lend);
            break;
        }

        /* for var = init to limit do S
             var = init
           L_start:
             t = var <= limit
             ifnot t goto L_end
             S
             t2 = var + 1
             var = t2
             goto L_start
           L_end:                         */
        case NODE_FOR_STATEMENT: {
            char* init = gen_expr(tac, node->left);
            emit(tac, TAC_ASSIGN, node->value, init, NULL);
            free(init);

            char* lstart = new_label(tac);
            char* lend   = new_label(tac);
            emit(tac, TAC_LABEL, lstart, NULL, NULL);

            char* limit = gen_expr(tac, node->right);
            char* t = new_temp(tac);
            emit(tac, TAC_LE, t, node->value, limit);
            free(limit);
            emit(tac, TAC_IFNOT_GOTO, lend, t, NULL);
            free(t);

            gen_stmt(tac, node->then_branch);

            char* t2 = new_temp(tac);
            emit(tac, TAC_ADD,    t2,          node->value, "1");
            emit(tac, TAC_ASSIGN, node->value, t2,          NULL);
            free(t2);

            emit(tac, TAC_GOTO,  lstart, NULL, NULL);
            emit(tac, TAC_LABEL, lend,   NULL, NULL);
            free(lstart); free(lend);
            break;
        }

        /* ( FUNCION ARGUMENTOS ) — construir string de comando */
        case NODE_SHELL_CMD: {
            char cmd[1024];
            strncpy(cmd, node->value, sizeof(cmd) - 1);
            cmd[sizeof(cmd) - 1] = '\0';
            for (ASTNode* arg = node->left; arg; arg = arg->next) {
                strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
                if (!arg->value) continue;
                if (arg->type == NODE_FACTOR_STRING) {
                    /* quitar comillas simples: 'path' → path */
                    const char* s = arg->value;
                    size_t len = strlen(s);
                    if (len >= 2 && s[0] == '\'' && s[len-1] == '\'') {
                        size_t inner = len - 2;
                        size_t space = sizeof(cmd) - strlen(cmd) - 1;
                        strncat(cmd, s + 1, inner < space ? inner : space);
                    } else {
                        strncat(cmd, s, sizeof(cmd) - strlen(cmd) - 1);
                    }
                } else {
                    /* RUTA, IDENTIFIER o NUMBER — usar tal cual */
                    strncat(cmd, arg->value, sizeof(cmd) - strlen(cmd) - 1);
                }
            }
            emit(tac, TAC_SYSCALL, NULL, cmd, NULL);
            break;
        }

        /* echo expr */
        case NODE_ECHO: {
            char* val = gen_expr(tac, node->left);
            emit(tac, TAC_PRINT, NULL, val, NULL);
            free(val);
            break;
        }

        default:
            break;
    }
}

/* ── API publica ──────────────────────────────────── */

TACList* tac_generate(ASTNode* program) {
    if (!program) return NULL;
    TACList* tac = calloc(1, sizeof(TACList));
    if (!tac) return NULL;
    gen_stmt(tac, program);
    return tac;
}

static const char* op_str(TACOp op) {
    switch (op) {
        case TAC_ADD: return "+";
        case TAC_SUB: return "-";
        case TAC_MUL: return "*";
        case TAC_DIV: return "/";
        case TAC_LT:  return "<";
        case TAC_GT:  return ">";
        case TAC_LE:  return "<=";
        case TAC_GE:  return ">=";
        case TAC_EQ:  return "==";
        case TAC_NE:  return "!=";
        case TAC_AND: return "&&";
        case TAC_OR:  return "||";
        default:      return "?";
    }
}

void tac_print(const TACList* tac) {
    if (!tac || !tac->head) {
        printf("  (sin instrucciones)\n");
        return;
    }
    for (TACInstr* i = tac->head; i; i = i->next) {
        switch (i->op) {
            case TAC_ASSIGN:
                printf("  %s = %s\n", i->result, i->arg1);
                break;
            case TAC_ADD: case TAC_SUB: case TAC_MUL: case TAC_DIV:
            case TAC_LT:  case TAC_GT:  case TAC_LE:  case TAC_GE:
            case TAC_EQ:  case TAC_NE:  case TAC_AND: case TAC_OR:
                printf("  %s = %s %s %s\n",
                       i->result, i->arg1, op_str(i->op), i->arg2);
                break;
            case TAC_SYSCALL:
                printf("  syscall %s\n", i->arg1);
                break;
            case TAC_NEG:
                printf("  %s = -%s\n", i->result, i->arg1);
                break;
            case TAC_PRINT:
                printf("  print %s\n", i->arg1);
                break;
            case TAC_LABEL:
                printf("%s:\n", i->result);
                break;
            case TAC_GOTO:
                printf("  goto %s\n", i->result);
                break;
            case TAC_IFNOT_GOTO:
                printf("  ifnot %s goto %s\n", i->arg1, i->result);
                break;
        }
    }
}

void tac_free(TACList* tac) {
    if (!tac) return;
    TACInstr* cur = tac->head;
    while (cur) {
        TACInstr* next = cur->next;
        free(cur->result);
        free(cur->arg1);
        free(cur->arg2);
        free(cur);
        cur = next;
    }
    free(tac);
}
