#ifndef TOKEN_H
#define TOKEN_H

typedef enum Token_type{
	IDENTIFIER = 1,
	KEYWORD = 2,
	NUMBER = 3,
	OPERATION = 4,
	STRING = 5,
	SYMBOLS = 6,
	RUTA = 7,
	UNKNOWN = -1
}Token_type;

typedef struct Token{
	struct Token* next;
	char* lexeme;
	Token_type type;
}Token;

Token* init_token();
void print_token(Token* t);

#endif
