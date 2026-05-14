CC = cc
SRC = src/list.c src/dfa.c src/token.c src/parser.c src/error.c
FLAGS = -std=c99 -pedantic
MAIN = main.c

run:
	$(CC) $(MAIN) $(SRC) -o kde

