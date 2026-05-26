#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/tui.h"

// Codigos ANSI
#define RST   "\033[0m"
#define BOLD  "\033[1m"
#define RED   "\033[31m"
#define GREEN "\033[32m"
#define YEL   "\033[33m"
#define BLUE  "\033[34m"
#define CYAN  "\033[36m"
#define GRAY  "\033[90m"

#define W 50

static void fill(char c, int n) { //para darle formato
    for (int i = 0; i < n; i++) putchar(c);
}

static void section_open(const char* title, const char* color) { //encabezado
    int used = 3 + (int)strlen(title) + 1;
    printf("\n%s── %s ", color, title);
    fill('-', W - used > 0 ? W - used : 2);
    printf(RST "\n");
}

static void section_close(void) { //separador entre secciones 
    printf(GRAY);
    fill('-', W);
    printf(RST "\n\n");
}

void tui_header(void) {
    printf(CYAN BOLD "\n");
    printf("  +");  fill('=', W - 4);  printf("+\n");
    printf("  |  KED Compiler  |  REPL");
    int pad = W - 4 - 26;
    fill(' ', pad > 0 ? pad : 1);
    printf("|\n");
    printf("  +");  fill('=', W - 4);  printf("+\n");
    printf(RST "\n");
}

void tui_prompt(void) {
    printf(GREEN BOLD ">>> " RST);
    fflush(stdout);
}

void tui_show_ast(ASTNode* ast) {
    section_open("AST", YEL);
    if (ast)
        ast_print(ast, 1);
    else
        printf(GRAY "  (sin AST valido)\n" RST);
    section_close();
}

void tui_show_symbols(const SymbolTable* st) {
    section_open("Tabla de simbolos", BLUE);
    if (!st || st->count == 0) {
        printf(GRAY "  (vacia)\n" RST);
    } else {
        printf(GRAY "  %-16s %-10s %s\n" RST, "Nombre", "Tipo", "Estado");
        for (int i = 0; i < SYM_TABLE_SIZE; i++) {
            for (SymbolEntry* e = st->buckets[i]; e; e = e->next) {
                printf("  " CYAN "%-16s" RST, e->name);
                printf("%-10s", e->kind == SYM_VAR ? "VAR" : "FUNCION");
                if (e->defined)
                    printf(GREEN "definida\n" RST);
                else
                    printf(RED "no definida\n" RST);
            }
        }
    }
    section_close();
}

static void print_error_line(const Error* e) {
    switch (e->code) {
        case E_LEX_01:
            printf("  " RED "[E-LEX-01]" RST " '%s' no es una funcion valida.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_02:
            printf("  " RED "[E-LEX-02]" RST " Cadena sin cerrar: %s\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_03:
            printf("  " RED "[E-LEX-03]" RST " Caracter no valido en RUTA: '%s'.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_04:
            printf("  " RED "[E-LEX-04]" RST " Token '%s' no reconocido.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_LEX_05:
            printf("  " RED "[E-LEX-05]" RST " Comillas dobles no soportadas: %s. Use comillas simples.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SIN_01:
            printf("  " RED "[E-SIN-01]" RST " Se esperaba '(' al inicio de expresion: '%s'.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SIN_02:
            printf("  " RED "[E-SIN-02]" RST " Se esperaba ')' al final de expresion.\n");
            break;
        case E_SIN_03:
            printf("  " RED "[E-SIN-03]" RST " Se esperaba una FUNCION despues de '('.\n");
            break;
        case E_SIN_04:
            printf("  " RED "[E-SIN-04]" RST " Orden invalido, FUNCION debe ir antes de ARGUMENTOS.\n");
            break;
        case E_SIN_05:
            printf("  " RED "[E-SIN-05]" RST " Expresion vacia, se esperaba FUNCION.\n");
            break;
        case E_SIN_06:
            printf("  " RED "[E-SIN-06]" RST " No se encontro ningun comando valido.\n");
            break;
        case E_SEM_01:
            printf("  " RED "[E-SEM-01]" RST " '%s' requiere al menos un argumento.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SEM_02:
            printf("  " RED "[E-SEM-02]" RST " '%s' no acepta mas de 1 argumento.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SEM_03:
            printf("  " RED "[E-SEM-03]" RST " Argumento debe ser RUTA, no CADENA.\n");
            break;
        case E_SEM_04:
            printf("  " RED "[E-SEM-04]" RST " La ruta '%s' no existe.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        case E_SEM_05:
            printf("  " RED "[E-SEM-05]" RST " Variable '%s' usada antes de ser asignada.\n",
                   e->lexeme ? e->lexeme : "?");
            break;
        default:
            printf("  " RED "[E-%03d]" RST "   %s\n",
                   (int)e->code, e->lexeme ? e->lexeme : "?");
            break;
    }
}

void tui_show_output(const Env* env, int hit_limit) {
    section_open("Resultado de ejecucion", GREEN);
    if (hit_limit)
        printf(YEL "  ! Limite de pasos alcanzado (posible bucle infinito)\n" RST);

    if (env && env->output_count > 0) {
        for (int i = 0; i < env->output_count; i++)
            printf("  " GREEN "%s\n" RST, env->output[i]);
        if (env->count > 0) printf("\n");
    }

    if (!env || env->count == 0) {
        if (!env || env->output_count == 0)
            printf(GRAY "  (sin variables)\n" RST);
    } else {
        int printed = 0;
        for (int i = 0; i < ENV_BUCKETS; i++) {
            for (EnvEntry* e = env->buckets[i]; e; e = e->next) {
                if (is_temp_name(e->name)) continue;
                printf("  " CYAN "%-16s" RST "= ", e->name);
                if (e->val.kind == VAL_INT)
                    printf("%d\n", e->val.ival);
                else if (e->val.kind == VAL_STR)
                    printf("%s\n", e->val.sval ? e->val.sval : "\"\"");
                else
                    printf(GRAY "(indefinido)\n" RST);
                printed++;
            }
        }
        if (!printed && env->output_count == 0)
            printf(GRAY "  (sin variables de usuario)\n" RST);
    }
    section_close();
}

void tui_show_code(const TACList* tac) { //imprime el codigo intermedio
    section_open("Codigo Intermedio (TAC)", "\033[35m");
    if (tac)
        tac_print(tac);
    else
        printf(GRAY "  (sin codigo generado)\n" RST);
    section_close();
}

void tui_show_errors(const ErrorStack* es) { //imprime los errores encontrados
    if (error_stack_empty(es)) {
        printf(GREEN "  Sin errores\n" RST "\n");
        return;
    }

    section_open("Errores", RED);

    // Imprimir en orden de insercion (mas antiguo primero)
    Error** arr = malloc((size_t)es->count * sizeof(Error*));
    if (!arr) return;
    int n = 0;
    for (Error* cur = es->top; cur; cur = cur->next)
        arr[n++] = cur;
    for (int i = n - 1; i >= 0; i--)
        print_error_line(arr[i]);
    free(arr);

    section_close();
}
