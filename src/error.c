#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/error.h"

ErrorStack* error_stack_init(void) { //reserva de memoria para la pila de errores
	ErrorStack* es = malloc(sizeof(ErrorStack));
	if (!es) return NULL;
	es->top   = NULL;
	es->count = 0;
	return es;
}

void error_push(ErrorStack* es, ErrorCode code, const char* lexeme) {
	if (!es) return;
	Error* e = malloc(sizeof(Error));
	if (!e) return;
	e->code   = code;
	e->lexeme = lexeme ? strdup(lexeme) : NULL;
	e->next   = es->top;
	es->top   = e;
	es->count++;
}

Error* error_pop(ErrorStack* es) {
	if (!es || !es->top) return NULL;
	Error* e = es->top;
	es->top  = e->next;
	e->next  = NULL;
	es->count--;
	return e;
}

static void print_one(const Error* e) {
	switch (e->code) {
        case E_LEX_01:
		printf("Error lexico [E-LEX-01]: '%s' no es una funcion valida.\n",
		       e->lexeme ? e->lexeme : "?"); //valor por defecto si lexeme es NULL
		break;
        case E_LEX_02:
		printf("Error lexico [E-LEX-02]: Cadena sin cerrar: %s\n",
		       e->lexeme ? e->lexeme : "?");
		break;
        case E_LEX_03:
		printf("Error lexico [E-LEX-03]: Caracter no valido en RUTA: '%s'.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
        case E_LEX_04:
		printf("Error lexico [E-LEX-04]: Token '%s' no reconocido.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
        case E_LEX_05:
		printf("Error lexico [E-LEX-05]: Comillas dobles no soportadas: %s. Use comillas simples.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
        case E_SIN_01:
		printf("Error sintactico [E-SIN-01]: Se esperaba '(' al inicio de expresion: '%s'.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
        case E_SIN_02:
		printf("Error sintactico [E-SIN-02]: Se esperaba ')' al final de expresion.\n");
		break;
        case E_SIN_03:
		printf("Error sintactico [E-SIN-03]: Se esperaba una FUNCION despues de '('.\n");
		break;
        case E_SIN_04:
		printf("Error sintactico [E-SIN-04]: Orden invalido, FUNCION debe ir antes de ARGUMENTOS.\n");
		break;
        case E_SIN_05:
		printf("Error sintactico [E-SIN-05]: Expresion vacia, se esperaba FUNCION.\n");
		break;
        case E_SIN_06:
		printf("Error sintactico [E-SIN-06]: No se encontro ningun comando valido.\n");
		break;
	case E_SEM_01:
		printf("Error semantico [E-SEM-01]: '%s' requiere al menos un argumento.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
	case E_SEM_02:
		printf("Error semantico [E-SEM-02]: '%s' no acepta mas de 1 argumento.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
	case E_SEM_03:
		printf("Error semantico [E-SEM-03]: Argumento debe ser RUTA, no CADENA.\n");
		break;
	case E_SEM_04:
		printf("Error semantico [E-SEM-04]: La ruta '%s' no existe.\n",
		       e->lexeme ? e->lexeme : "?");
		break;
	case E_SEM_05:
		printf("Error semantico [E-SEM-05]: Variable '%s' usada sin declarar.\n",
		       e->lexeme ? e->lexeme : "?");
		break;

        default:
		printf("Error desconocido (codigo %d): '%s'\n",
		       (int)e->code, e->lexeme ? e->lexeme : "?");
		break;
	}
}

void error_stack_print(const ErrorStack* es) {
	if (!es || !es->top) {
		printf("No se encontraron errores.\n");
		return;
	}
	printf("=== Errores encontrados (%d) ===\n", es->count);

	/* Collect pointers to print in insertion order (oldest first). */
	Error** arr = malloc((size_t)es->count * sizeof(Error*));
	if (!arr) return;
	int n = 0;
	for (Error* cur = es->top; cur; cur = cur->next)
		arr[n++] = cur;
	for (int i = n - 1; i >= 0; i--)
		print_one(arr[i]);
	free(arr);
}

void error_stack_free(ErrorStack* es) {
	if (!es) return;
	Error* cur = es->top;
	while (cur) {
		Error* next = cur->next;
		free(cur->lexeme);
		free(cur);
		cur = next;
	}
	free(es);
}

int error_stack_empty(const ErrorStack* es) {
	return !es || !es->top; //devuelve 1 si es NULL o si no tiene errores, 0 si tiene errores
}
