#ifndef TUI_H
#define TUI_H

#include "parser.h"
#include "symbol_table.h"
#include "error.h"
#include "codegen.h"
#include "eval.h"

void tui_header(void);
void tui_prompt(void);
void tui_show_ast(ASTNode* ast);
void tui_show_symbols(const SymbolTable* st);
void tui_show_errors(const ErrorStack* es);
void tui_show_code(const TACList* tac);
void tui_show_output(const Env* env, int hit_limit);

#endif