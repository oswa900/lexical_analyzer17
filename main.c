#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/list.h"
#include "include/dfa.h"
#include "include/token.h"
#include "include/parser.h"
#include "include/error.h"
#include "include/semantic.h"

char* get_content(FILE* file);
void run(const char* raw);          

int main(int argc, char **argv) {
	if (argv[1]) {
		FILE* file = fopen(argv[1], "r");
		if (!file) { fprintf(stderr, "No se pudo abrir el archivo\n"); return 1; }
		char* heap_buf = get_content(file);
		fclose(file);
		run(heap_buf);
		free(heap_buf);
	} else {
		char line[1024];
		while (1) {
			printf(">>> ");
			fflush(stdout);
			if (!fgets(line, sizeof(line), stdin)) {
				printf("\n");   /* salto de línea al recibir EOF */
				break;
			}
			if (line[0] == '\n' || line[0] == '\0') continue;
			run(line);
		}
	}
	return 0;
}

/* ── lógica principal reutilizable ── */
void run(const char* raw) {
	ErrorStack* errors = error_stack_init();

	/* análisis léxico */
	List* tokens = tokenize(raw, errors);

	/* análisis sintáctico */
	Parser*  p   = parser_init(tokens, errors);
	ASTNode* ast = parse_program(p);

	/* imprimir AST */
	if (ast) {
		printf("=== AST ===\n");
		ast_print(ast, 0);
		printf("\n");
	} else {
		printf("[!] El parser no produjo un AST valido.\n\n");
	}

	/* análisis semántico */
	if (ast) {
		SemanticAnalyzer* sa = semantic_init(errors);
		if (sa) {
			semantic_analyze(sa, ast);
			printf("=== Tabla de simbolos ===\n");
			sym_table_print(sa->symbols);
			printf("\n");
			semantic_free(sa);
		}
	}

	/* reporte de errores */
	error_stack_print(errors);

	/* limpieza */
	ast_free(ast);
	parser_free(p);
	destoy_list(&tokens);
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
