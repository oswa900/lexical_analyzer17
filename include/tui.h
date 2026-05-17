#ifndef TUI_H
#define TUI_H

#include "parser.h"
#include "symbol_table.h"
#include "error.h"

void tui_header(void);
void tui_prompt(void);
void tui_show_ast(ASTNode* ast);
void tui_show_symbols(const SymbolTable* st);
void tui_show_errors(const ErrorStack* es);

#endif