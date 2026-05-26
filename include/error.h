#ifndef ERROR_H
#define ERROR_H

typedef enum ErrorCode {
    E_LEX_01 = 101,
    E_LEX_02 = 102,
    E_LEX_03 = 103,
    E_LEX_04 = 104,
    E_LEX_05 = 105,   /* Comillas dobles no soportadas              */
    E_SIN_01 = 201,
    E_SIN_02 = 202,
    E_SIN_03 = 203,
    E_SIN_04 = 204,
    E_SIN_05 = 205,
    E_SIN_06 = 206,
    E_SEM_01 = 500,   /* Comando sin argumentos requeridos  */
    E_SEM_02 = 501,   /* Demasiados argumentos              */
    E_SEM_03 = 502,   /* Tipo de argumento incorrecto       */
    E_SEM_04 = 503,   /* Ruta inexistente                   */
    E_SEM_05 = 504    /* Variable usada antes de ser asignada */
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
