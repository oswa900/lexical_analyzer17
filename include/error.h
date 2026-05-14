#ifndef ERROR_H
#define ERROR_H

typedef enum ErrorCode {
    E_LEX_01 = 101,
    E_LEX_02 = 102,
    E_LEX_03 = 103,
    E_LEX_04 = 104,
    E_SIN_01 = 201,
} ErrorCode;

typedef struct Error {
    ErrorCode     code;
    char*         lexeme;
    struct Error* next;
} Error;

typedef struct ErrorStack {
    Error* top;
    int    count;
} ErrorStack;

ErrorStack* error_stack_init(void);
void        error_push(ErrorStack* es, ErrorCode code, const char* lexeme);
Error*      error_pop(ErrorStack* es);
void        error_stack_print(const ErrorStack* es);
void        error_stack_free(ErrorStack* es);
int         error_stack_empty(const ErrorStack* es);

#endif
