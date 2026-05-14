#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "../include/token.h"
#include "../include/list.h"
#include "../include/error.h"

static const char* keywords[] = {
    "if", "while", "for", "else", "then"
};

static const char* commands[] = {
    "ls", "mkdir", "touch", "edit", "rm", "help", "clear", "vim", "echo"
};

static int is_keyword(const char* str) {
    if (!str || *str == '\0') return 0;
    int n = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(str, keywords[i]) == 0) return 1;
    return 0;
}

int is_valid_function(const char* str) {
    if (!str || *str == '\0') return 0;
    int n = sizeof(commands) / sizeof(commands[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(str, commands[i]) == 0) return 1;
    return 0;
}

static int is_operation(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/'
        || c == '=' || c == '<' || c == '>';
}

static int is_paren(char c) {
    return c == '(' || c == ')';
}

static int is_valid_path_char(char c) {
    return isalnum((unsigned char)c) || c == '/' || c == '.' || c == '-' || c == '_';
}

static int is_path_start(const char* s) {
    if (*s == '/') return 1;
    if (*s == '.' && *(s+1) == '/') return 1;
    if (*s == '.' && *(s+1) == '.' && *(s+2) == '/') return 1;
    return 0;
}

static Token* get_next_token(const char* input) {
    if (!input || *input == '\0') return NULL;

    Token* t = malloc(sizeof(Token));
    if (!t) return NULL;
    t->next   = NULL;
    t->lexeme = NULL;
    t->type   = UNKNOWN;

    const char* str = input;

    if (isalpha((unsigned char)*str)) {
        const char* p = str;
        while (*p) {
            if (!isalpha((unsigned char)*p) && !isdigit((unsigned char)*p)) {
                t->lexeme = strdup(str);
                t->type   = UNKNOWN;
                return t;
            }
            p++;
        }
        t->lexeme = strdup(str);
        t->type   = is_keyword(str) ? KEYWORD : IDENTIFIER;
        return t;
    }

    if (isdigit((unsigned char)*str)) {
        const char* p = str;
        while (*p) {
            if (!isdigit((unsigned char)*p)) {
                t->lexeme = strdup(str);
                t->type   = UNKNOWN;
                return t;
            }
            p++;
        }
        t->lexeme = strdup(str);
        t->type   = NUMBER;
        return t;
    }

    if (is_operation(*str)) {
        if (*(str + 1) != '\0') {
            t->lexeme = strdup(str);
            t->type   = UNKNOWN;
            return t;
        }
        t->lexeme = strdup(str);
        t->type   = OPERATION;
        return t;
    }

    if (is_paren(*str)) {
        if (*(str + 1) != '\0') {
            t->lexeme = strdup(str);
            t->type   = UNKNOWN;
            return t;
        }
        t->lexeme = strdup(str);
        t->type   = SYMBOLS;
        return t;
    }

    if (*str == '\'') {
        const char* p = str + 1;
        while (*p && *p != '\'') p++;
        t->lexeme = strdup(str);
        t->type   = (*p == '\'') ? STRING : UNKNOWN;
        return t;
    }

    t->lexeme = strdup(str);
    t->type   = UNKNOWN;
    return t;
}

/* flush_buffer: tokenize whatever is in buffer and push it to the list.
   UNKNOWN tokens that don't start with ' (already handled as E-LEX-02)
   are reported as E-LEX-04. */
static void flush_buffer(List* l, char* buffer, int* i, ErrorStack* errors) {
    if (*i == 0) return;
    buffer[*i] = '\0';
    Token* t = get_next_token(buffer);
    if (t) {
        if (t->type == UNKNOWN && errors)
            error_push(errors, E_LEX_04, buffer);
        push_token(l, t);
    }
    memset(buffer, 0, *i + 1);
    *i = 0;
}

List* tokenize(const char* input, ErrorStack* errors) {
    if (!input) return NULL;

    List* l = init_list();
    if (!l) return NULL;

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int i = 0;

    const char* str = input;

    while (*str) {

        /* whitespace — flush accumulated word */
        if (*str == ' ' || *str == '\n') {
            flush_buffer(l, buffer, &i, errors);
            str++;
            continue;
        }

        /* E-LEX-02: double-quoted string (should use single quotes) */
        if (*str == '"') {
            flush_buffer(l, buffer, &i, errors);
            buffer[i++] = *str++;
            while (*str && *str != '"' && *str != '\n')
                buffer[i++] = *str++;
            if (*str == '"') buffer[i++] = *str++;
            buffer[i] = '\0';
            error_push(errors, E_LEX_02, buffer);
            Token* t = get_next_token("?");   /* placeholder UNKNOWN token */
            if (t) { free(t->lexeme); t->lexeme = strdup(buffer); push_token(l, t); }
            memset(buffer, 0, i + 1);
            i = 0;
            continue;
        }

        /* RUTA: absolute path (/…) or relative path (./… or ../…) */
        if (is_path_start(str)) {
            flush_buffer(l, buffer, &i, errors);
            int has_invalid = 0;
            while (*str && *str != ' ' && *str != '\n') {
                if (!is_valid_path_char(*str)) has_invalid = 1;
                buffer[i++] = *str++;
            }
            buffer[i] = '\0';
            if (has_invalid && errors)
                error_push(errors, E_LEX_03, buffer);
            Token* t = malloc(sizeof(Token));
            if (t) {
                t->next   = NULL;
                t->lexeme = strdup(buffer);
                t->type   = has_invalid ? UNKNOWN : RUTA;
                push_token(l, t);
            }
            memset(buffer, 0, i + 1);
            i = 0;
            continue;
        }

        /* parentheses */
        if (is_paren(*str)) {
            flush_buffer(l, buffer, &i, errors);
            char paren[2] = { *str, '\0' };
            Token* t = get_next_token(paren);
            if (t) push_token(l, t);
            str++;
            continue;
        }

        /* operators */
        if (is_operation(*str)) {
            flush_buffer(l, buffer, &i, errors);
            char op[2] = { *str, '\0' };
            Token* t = get_next_token(op);
            if (t) push_token(l, t);
            str++;
            continue;
        }

        /* single-quoted string; unclosed → E-LEX-02 */
        if (*str == '\'') {
            flush_buffer(l, buffer, &i, errors);
            buffer[i++] = *str++;
            while (*str && *str != '\'')
                buffer[i++] = *str++;
            if (*str == '\'') {
                buffer[i++] = *str++;
                flush_buffer(l, buffer, &i, errors);   /* will be STRING */
            } else {
                buffer[i] = '\0';
                error_push(errors, E_LEX_02, buffer);
                Token* t = malloc(sizeof(Token));
                if (t) {
                    t->next   = NULL;
                    t->lexeme = strdup(buffer);
                    t->type   = UNKNOWN;
                    push_token(l, t);
                }
                memset(buffer, 0, i + 1);
                i = 0;
            }
            continue;
        }

        buffer[i++] = *str++;
    }

    flush_buffer(l, buffer, &i, errors);
    return l;
}
