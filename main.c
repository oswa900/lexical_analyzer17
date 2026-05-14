#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/list.h"
#include "include/dfa.h"
#include "include/token.h"
#include "include/parser.h"
#include "include/error.h"

char* get_content(FILE* file);

int main(int argc, char **argv) {
    ErrorStack* errors = error_stack_init();

    const char* raw = NULL;
    char* heap_buf  = NULL;

    if (argv[1]) {
        FILE* file = fopen(argv[1], "r");
        if (!file) { fprintf(stderr, "No se pudo abrir el archivo\n"); return 1; }
        heap_buf = get_content(file);
        fclose(file);
        raw = heap_buf;
    } else {
        raw = "x = (20 + 20) / (20 + 20 / 7)";
    }

    List*    tokens = tokenize(raw, errors);
    Parser*  p      = parser_init(tokens, errors);
    ASTNode* ast    = parse_program(p);

    if (ast) ast_print(ast, 0);

    ast_free(ast);
    parser_free(p);
    error_stack_print(errors);
    error_stack_free(errors);
    free(heap_buf);

    return 0;
}

char* get_content(FILE* file) {
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    char* buffer = malloc(size + 1);
    if (!buffer) return NULL;
    size_t n = fread(buffer, 1, size, file);
    buffer[n] = '\0';
    return buffer;
}
