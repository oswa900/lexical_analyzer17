#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/list.h"
#include "include/dfa.h"
#include "include/token.h"
#include "include/parser.h"
#include "include/error.h"
#include "include/semantic.h"
#include "include/codegen.h"
#include "include/tui.h"

char* get_content(FILE* file);
void  run(const char* raw, SemanticAnalyzer* sa);

int main(int argc, char **argv) {
    /* SemanticAnalyzer persiste durante toda la sesion
       para que la tabla de simbolos sobreviva entre lineas del REPL. */
    SemanticAnalyzer* sa = semantic_init(NULL);

    if (argv[1]) {
        /* modo archivo */
        FILE* file = fopen(argv[1], "r");
        if (!file) { fprintf(stderr, "No se pudo abrir el archivo\n"); return 1; }
        char* buf = get_content(file);
        fclose(file);
        run(buf, sa);
        free(buf);
    } else {
        /* modo REPL */
        tui_header();
        char line[1024];
        while (1) {
            tui_prompt();
            if (!fgets(line, sizeof(line), stdin)) {
                printf("\n");
                break;
            }
            if (line[0] == '\n' || line[0] == '\0') continue;
            run(line, sa);
        }
    }

    semantic_free(sa);
    return 0;
}

void run(const char* raw, SemanticAnalyzer* sa) {
    ErrorStack* errors = error_stack_init();

    /* Apuntar el SA al stack de errores de esta iteracion.
       La tabla de simbolos (sa->symbols) no se toca — persiste. */
    sa->errors = errors;

    /* analisis lexico */
    List* tokens = tokenize(raw, errors);

    /* analisis sintactico */
    Parser*  p   = parser_init(tokens, errors);
    ASTNode* ast = parse_program(p);

    /* analisis semantico — usa la tabla acumulada */
    if (ast) semantic_analyze(sa, ast);

    /* generacion de codigo intermedio (TAC) */
    TACList* tac = error_stack_empty(errors) ? tac_generate(ast) : NULL;

    /* salida TUI */
    tui_show_ast(ast);
    tui_show_code(tac);
    tui_show_symbols(sa->symbols);
    tui_show_errors(errors);

    /* limpieza parcial — NO liberar sa ni sa->symbols */
    tac_free(tac);
    ast_free(ast);
    parser_free(p);
    destoy_list(&tokens);
    sa->errors = NULL;
    error_stack_free(errors);
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
