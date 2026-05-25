#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

// Tabla de simbolos: hash table con encadenamiento separado (djb2)

#define SYM_TABLE_SIZE 64

typedef enum {
    SYM_VAR,       /* variable declarada por asignacion */
    SYM_FUNCTION   /* comando reconocido por el DFA     */
} SymbolKind;

typedef struct SymbolEntry {
    char*              name;
    SymbolKind         kind;
    int                defined;
    struct SymbolEntry* next;
} SymbolEntry;

typedef struct {
    SymbolEntry* buckets[SYM_TABLE_SIZE];
    int          count;
} SymbolTable;

SymbolTable*  sym_table_init(void);
void          sym_table_free(SymbolTable* st);
SymbolEntry*  sym_table_insert(SymbolTable* st, const char* name, SymbolKind kind);
SymbolEntry*  sym_table_lookup(const SymbolTable* st, const char* name);
void          sym_table_print(const SymbolTable* st);

#endif
