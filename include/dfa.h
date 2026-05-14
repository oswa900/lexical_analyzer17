#ifndef DFA_H
#define DFA_H
#include "token.h"
#include "list.h"
#include "error.h"

List* tokenize(const char* input, ErrorStack* errors);
int   is_valid_function(const char* name);

#endif
