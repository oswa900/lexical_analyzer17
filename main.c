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
#include "include/eval.h"
#include "include/tui.h"

char* get_content(FILE* file);
void  run(const char* raw, SemanticAnalyzer* sa, Env* env);

int main(int argc, char **argv) {
    /* Estado persistente durante toda la sesion:
       - sa->symbols : tabla de simbolos (analisis estatico)
       - env         : valores en tiempo de ejecucion       */
    SemanticAnalyzer* sa  = semantic_init(NULL);
    Env*              env = env_init();

    if (argv[1]) {
        FILE* file = fopen(argv[1], "r");
        if (!file) { fprintf(stderr, "No se pudo abrir el archivo\n"); return 1; }
        char* buf = get_content(file);
        fclose(file);
        run(buf, sa, env);
        free(buf);
    } else {
        tui_header();
        char line[1024];
        while (1) {
            tui_prompt();
            if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }
            if (line[0] == '\n' || line[0] == '\0') continue;
            run(line, sa, env);
        }
    }

    semantic_free(sa);
    env_free(env);
    return 0;
}

void run(const char* raw, SemanticAnalyzer* sa, Env* env) {
    ErrorStack* errors = error_stack_init();
    sa->errors = errors;

    /* 1. analisis lexico */
    List* tokens = tokenize(raw, errors);

    /* 2. analisis sintactico */
    Parser*  p   = parser_init(tokens, errors);
    ASTNode* ast = parse_program(p);

    /* 3. analisis semantico */
    if (ast) semantic_analyze(sa, ast);

    /* 4. generacion de codigo intermedio */
    TACList* tac = error_stack_empty(errors) ? tac_generate(ast) : NULL;

    /* 5. evaluacion — solo si el TAC se genero correctamente */
    int hit_limit = 0;
    if (tac) hit_limit = eval_run(tac, env);

    /* 6. salida TUI */
    tui_show_ast(ast);
    tui_show_code(tac);
    tui_show_output(env, hit_limit);
    tui_show_symbols(sa->symbols);
    tui_show_errors(errors);

    /* limpieza — NO tocar sa ni env (persisten entre iteraciones) */
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
