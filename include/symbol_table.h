#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

/* ---------------------------------------------------------------
 * Tabla de símbolos — análisis semántico
 *
 * Estructura: hash table de encadenamiento separado.
 *   - Clave  : nombre del símbolo (string)
 *   - Valor  : SymbolEntry con tipo y estado
 * --------------------------------------------------------------- */

#define SYM_TABLE_SIZE 64   /* debe ser potencia de 2 */

typedef enum {
    SYM_VAR,       /* variable declarada por asignación   */
    SYM_FUNCTION   /* comando/función reconocido por el DFA */
} SymbolKind;

typedef struct SymbolEntry {
    char*              name;
    SymbolKind         kind;
    int                defined;   /* 1 = ya fue asignada/definida     */
    struct SymbolEntry* next;     /* encadenamiento en el mismo bucket */
} SymbolEntry;

typedef struct {
    SymbolEntry* buckets[SYM_TABLE_SIZE];
    int          count;
} SymbolTable;

/* Ciclo de vida */
SymbolTable*  sym_table_init(void);
void          sym_table_free(SymbolTable* st);

/* Operaciones */
/* Inserta o actualiza un símbolo.  Devuelve la entrada. */
SymbolEntry*  sym_table_insert(SymbolTable* st, const char* name, SymbolKind kind);

/* Busca un símbolo; devuelve NULL si no existe. */
SymbolEntry*  sym_table_lookup(const SymbolTable* st, const char* name);

/* Debug */
void          sym_table_print(const SymbolTable* st);

#endif /* SYMBOL_TABLE_H */
