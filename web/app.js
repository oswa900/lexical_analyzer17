
// AST Node types matching NodeType in C
const NodeType = {
    NODE_PROGRAM: 'PROGRAM',
    NODE_ASSIGNMENT: 'ASSIGNMENT',
    NODE_IF_STATEMENT: 'IF',
    NODE_CONDITION: 'CONDITION',
    NODE_EXPRESSION: 'EXPRESSION',
    NODE_TERM: 'TERM',
    NODE_FACTOR_ID: 'ID',
    NODE_FACTOR_NUMBER: 'NUMBER',
    NODE_FACTOR_STRING: 'STRING',
    NODE_FACTOR_EXPR: 'PAREN_EXPR',
    NODE_BINOP: 'BINOP',
    NODE_WHILE_STATEMENT: 'WHILE',
    NODE_FOR_STATEMENT: 'FOR',
    NODE_LOGICAL: 'LOGICAL',
    NODE_ECHO: 'ECHO',
    NODE_UNARY_NEG: 'NEG',
    NODE_SHELL_CMD: 'SHELL_CMD',
    NODE_FACTOR_RUTA: 'RUTA'
};

// Token types matching Token_type in C
const TokenType = {
    IDENTIFIER: 'IDENTIFIER',
    KEYWORD: 'KEYWORD',
    NUMBER: 'NUMBER',
    OPERATION: 'OPERATION',
    STRING: 'STRING',
    SYMBOLS: 'SYMBOLS',
    RUTA: 'RUTA',
    UNKNOWN: 'UNKNOWN'
};

// Error codes descriptions
const ErrorDescriptions = {
    'E-LEX-01': 'Léxico: el identificador no es un comando de sistema válido (ls, mkdir, touch, edit, rm, help, clear, vim)',
    'E-LEX-02': 'Léxico: cadena de texto sin cerrar — falta la comilla de cierre al final',
    'E-LEX-03': 'Léxico: formato de ruta inválido — contiene caracteres no permitidos en una ruta',
    'E-LEX-04': 'Léxico: token o carácter no reconocido por el analizador léxico',
    'E-SIN-01': 'Sintáctico: se esperaba \'(\' al inicio de la expresión de comando',
    'E-SIN-02': 'Sintáctico: se esperaba \')\' al final de la expresión de comando',
    'E-SIN-03': 'Sintáctico: se esperaba una FUNCION después de \'(\'',
    'E-SIN-04': 'Sintáctico: orden inválido — la FUNCION debe ir antes de los ARGUMENTOS',
    'E-SIN-05': 'Sintáctico: expresión vacía — se esperaba una FUNCION',
    'E-SIN-06': 'Sintáctico: no se encontró ningún comando válido en la sentencia',
    'E-SEM-01': 'Semántico: el comando requiere al menos un argumento y no recibió ninguno',
    'E-SEM-02': 'Semántico: el comando no acepta más de 1 argumento',
    'E-SEM-03': 'Semántico: el argumento debe ser una RUTA (ej. /ruta/archivo), no una CADENA entre comillas',
    'E-SEM-04': 'Semántico: la ruta especificada no existe en el sistema de archivos',
    'E-SEM-05': 'Semántico: variable usada antes de ser asignada — debe declararse con \'var = valor\' primero'
};

// Preloaded template codes
const CodeTemplates = {
    aritmetica: `a = 10
b = 3

suma = a + b
resta = a - b
producto = a * b
division = a / b

echo suma
echo resta
echo producto
echo division

negativo = -a
echo negativo

expresion = -a + b * 2
echo expresion`,

    condicionales: `x = 15

if x > 10 then echo x else echo -1

if x > 5 && x < 20 then grande = 1 else grande = 0
echo grande

if x == 15 then echo x

y = -3
if y < 0 then positivo = -y else positivo = y
echo positivo`,

    bucles: `suma = 0
for i = 1 to 10 do suma = suma + i
echo suma

potencia = 1
for i = 1 to 8 do potencia = potencia * 2
echo potencia

x = 100
pasos = 0
while x > 1 do x = x / 2
echo x`,

    factorial: `n = 5
fact = 1
for i = 1 to n do fact = fact * i
echo fact`,

    producciones: `(mkdir /tmp/kde_prueba)
(touch /tmp/kde_prueba/hola.txt)
(ls /tmp/kde_prueba)
(rm /tmp/kde_prueba/hola.txt)
(rm /tmp/kde_prueba)`,

    errores_lexicos: `a = "cadena invalida con comillas dobles"
b = 10@`,

    errores_semanticos: `(rm)
(mkdir /ruta1 /ruta2)
(ls 'archivo.txt')
echo variable_no_declarada`,

    ejemplo_general: `x = 10
y = 20
z = x + y

a = 2
b = 3
c = a * b + 4
d = c - a * 2

if x < y then z = x + 1 else z = y - 1

if z < 8 then w = z * 2

nombre = 'archivo.txt'
ruta = /home/user/docs

if a < b then resultado = a * 4 else resultado = b + 10

total = resultado + z + w

if total < 100 then final = total + 1 else final = total - 1

(ls /home/user/docs)
(mkdir ./nueva_carpeta)
(touch ./nueva_carpeta/archivo.txt)
(rm /home/user/docs/viejo.txt)
echo 'hola mundo'`,

    errores_complejos: `x = 10
y = "comillas dobles"

z = x + fantasma

if x < limite then z = 1

path = /home/user/docs con$simbolo

echo x
(ls /home)`
};

/* ==========================================================================
   1. ANALIZADOR LÉXICO (TOKENIZER)
   ========================================================================== */

const keywords = ["if", "while", "for", "else", "then", "do", "to", "echo"];
const commands = ["ls", "mkdir", "touch", "edit", "rm", "help", "clear", "vim"];

const CMD_RULES = {
    'ls':    { min: 0, max: 1,  expectRuta: true  },
    'mkdir': { min: 1, max: 1,  expectRuta: true  },
    'touch': { min: 1, max: 1,  expectRuta: true  },
    'edit':  { min: 1, max: 1,  expectRuta: true  },
    'rm':    { min: 1, max: 1,  expectRuta: true  },
    'help':  { min: 0, max: -1, expectRuta: false },
    'clear': { min: 0, max: -1, expectRuta: false },
    'vim':   { min: 1, max: 1,  expectRuta: true  },
};

function isKeyword(str) { return keywords.includes(str); }
function isValidFunction(str) { return commands.includes(str); }
function isOperationChar(c) { return ['+', '-', '*', '/', '=', '<', '>', '!', '&', '|'].includes(c); }
function isParen(c) { return ['(', ')'].includes(c); }
function isValidPathChar(c) { return /[a-zA-Z0-9/\.\-_]/.test(c); }

function isPathStart(input, idx) {
    const c = input[idx];
    const next = input[idx + 1] || '';
    const after = input[idx + 2] || '';
    if (c === '/' && /[a-zA-Z0-9\.~/]/.test(next)) return true;
    if (c === '.' && next === '/') return true;
    if (c === '.' && next === '.' && after === '/') return true;
    return false;
}

function tokenize(input) {
    const tokens = [];
    const errors = [];
    let idx = 0;
    const len = input.length;

    const pushError = (code, lexeme) => {
        errors.push({ code, lexeme });
    };

    const flushBuffer = (buffer) => {
        if (!buffer) return;
        if (/^[a-zA-Z][a-zA-Z0-9]*$/.test(buffer)) {
            tokens.push({
                type: isKeyword(buffer) ? TokenType.KEYWORD : TokenType.IDENTIFIER,
                lexeme: buffer
            });
        } else if (/^[0-9]+$/.test(buffer)) {
            tokens.push({
                type: TokenType.NUMBER,
                lexeme: buffer
            });
        } else {
            pushError('E-LEX-04', buffer);
            tokens.push({
                type: TokenType.UNKNOWN,
                lexeme: buffer
            });
        }
    };

    let buffer = '';

    while (idx < len) {
        const c = input[idx];

        // whitespace
        if (c === ' ' || c === '\n' || c === '\r' || c === '\t') {
            flushBuffer(buffer);
            buffer = '';
            idx++;
            continue;
        }

        // Double quoted string (Error E-LEX-02)
        if (c === '"') {
            flushBuffer(buffer);
            buffer = '';
            let lexeme = '"';
            idx++;
            while (idx < len && input[idx] !== '"' && input[idx] !== '\n' && input[idx] !== '\r') {
                lexeme += input[idx];
                idx++;
            }
            if (idx < len && input[idx] === '"') {
                lexeme += '"';
                idx++;
            }
            pushError('E-LEX-02', lexeme);
            tokens.push({ type: TokenType.UNKNOWN, lexeme });
            continue;
        }

        // Single quoted string (Valid String)
        if (c === '\'') {
            flushBuffer(buffer);
            buffer = '';
            let lexeme = '\'';
            idx++;
            while (idx < len && input[idx] !== '\'') {
                lexeme += input[idx];
                idx++;
            }
            if (idx < len && input[idx] === '\'') {
                lexeme += '\'';
                idx++;
                tokens.push({ type: TokenType.STRING, lexeme });
            } else {
                pushError('E-LEX-02', lexeme);
                tokens.push({ type: TokenType.UNKNOWN, lexeme });
            }
            continue;
        }

        // Path (RUTA)
        if (isPathStart(input, idx)) {
            flushBuffer(buffer);
            buffer = '';
            let lexeme = '';
            let hasInvalid = false;
            while (idx < len && input[idx] !== ' ' && input[idx] !== '\n' && input[idx] !== '\r' && input[idx] !== '(' && input[idx] !== ')') {
                if (!isValidPathChar(input[idx])) hasInvalid = true;
                lexeme += input[idx];
                idx++;
            }
            if (hasInvalid) {
                pushError('E-LEX-03', lexeme);
                tokens.push({ type: TokenType.UNKNOWN, lexeme });
            } else {
                tokens.push({ type: TokenType.RUTA, lexeme });
            }
            continue;
        }

        // Parentheses
        if (isParen(c)) {
            flushBuffer(buffer);
            buffer = '';
            tokens.push({ type: TokenType.SYMBOLS, lexeme: c });
            idx++;
            continue;
        }

        // Operators
        if (isOperationChar(c)) {
            flushBuffer(buffer);
            buffer = '';
            let op = c;
            const next = input[idx + 1] || '';
            if (((c === '<' || c === '>' || c === '=' || c === '!') && next === '=') ||
                (c === '&' && next === '&') ||
                (c === '|' && next === '|')) {
                op += next;
                idx += 2;
            } else {
                idx += 1;
            }
            tokens.push({ type: TokenType.OPERATION, lexeme: op });
            continue;
        }

        // Otherwise, accumulate in identifier/number buffer
        buffer += c;
        idx++;
    }

    flushBuffer(buffer);
    return { tokens, errors };
}

/* ==========================================================================
   2. ANALIZADOR SINTÁCTICO (PARSER)
   ========================================================================== */

class Parser {
    constructor(tokens, errors) {
        this.tokens = tokens;
        this.errors = errors;
        this.currentIdx = 0;
    }

    current() {
        return this.tokens[this.currentIdx] || null;
    }

    advance() {
        if (this.currentIdx < this.tokens.length) {
            this.currentIdx++;
        }
    }

    currentLexeme() {
        const t = this.current();
        return t ? t.lexeme : null;
    }

    currentType() {
        const t = this.current();
        return t ? t.type : TokenType.UNKNOWN;
    }

    expectKeyword(kw) {
        const t = this.current();
        if (t && t.type === TokenType.KEYWORD && t.lexeme === kw) {
            this.advance();
            return true;
        }
        return false;
    }

    expectOperation(op) {
        const t = this.current();
        if (t && t.type === TokenType.OPERATION && t.lexeme === op) {
            this.advance();
            return true;
        }
        return false;
    }

    makeNode(type, value = null) {
        return {
            type,
            value,
            left: null,
            right: null,
            condition: null,
            then_branch: null,
            else_branch: null,
            next: null
        };
    }

    parse() {
        if (this.tokens.length === 0) return null;
        const program = this.makeNode(NodeType.NODE_PROGRAM);
        let tail = null;

        while (this.current()) {
            const stmt = this.parseStatement();
            if (!stmt) return null;
            if (!program.left) {
                program.left = stmt;
                tail = stmt;
            } else {
                tail.next = stmt;
                tail = stmt;
            }
        }
        return program;
    }

    parseStatement() {
        const t = this.current();
        if (!t) return null;

        // ASSIGNMENT: Lookahead for '='
        if (t.type === TokenType.IDENTIFIER && this.tokens[this.currentIdx + 1] &&
            this.tokens[this.currentIdx + 1].type === TokenType.OPERATION &&
            this.tokens[this.currentIdx + 1].lexeme === '=') {
            return this.parseAssignment();
        }

        if (t.type === TokenType.KEYWORD) {
            if (t.lexeme === 'if') return this.parseIfStatement();
            if (t.lexeme === 'while') return this.parseWhileStatement();
            if (t.lexeme === 'for') return this.parseForStatement();
            if (t.lexeme === 'echo') return this.parseEcho();
        }

        // SHELL CMD: ( func args )
        if (t.type === TokenType.SYMBOLS && t.lexeme === '(') {
            const next = this.tokens[this.currentIdx + 1];
            if (next && next.type === TokenType.IDENTIFIER) {
                if (isValidFunction(next.lexeme)) {
                    return this.parseShellCmd();
                } else {
                    // Try recovery or error
                    const after = this.tokens[this.currentIdx + 2];
                    if (after && (after.type === TokenType.RUTA || after.type === TokenType.STRING ||
                        (after.type === TokenType.SYMBOLS && after.lexeme === ')'))) {
                        this.errors.push({ code: 'E-LEX-01', lexeme: next.lexeme });
                        // Consume until matching ')'
                        this.advance(); // '('
                        let depth = 1;
                        while (this.current() && depth > 0) {
                            if (this.current().type === TokenType.SYMBOLS && this.current().lexeme === '(') depth++;
                            if (this.current().type === TokenType.SYMBOLS && this.current().lexeme === ')') depth--;
                            this.advance();
                        }
                        return null;
                    }
                }
            }
        }

        return this.parseExpression();
    }

    parseAssignment() {
        const node = this.makeNode(NodeType.NODE_ASSIGNMENT);
        node.value = this.currentLexeme();
        this.advance(); // consume ID

        if (!this.expectOperation('=')) return null;
        node.left = this.parseExpression();
        if (!node.left) return null;
        return node;
    }

    parseIfStatement() {
        const node = this.makeNode(NodeType.NODE_IF_STATEMENT);
        if (!this.expectKeyword('if')) return null;

        const hasParen = (this.current() && this.current().type === TokenType.SYMBOLS && this.current().lexeme === '(');
        if (hasParen) this.advance();

        node.condition = this.parseLogical();
        if (!node.condition) return null;

        if (hasParen) {
            if (this.current() && this.current().type === TokenType.SYMBOLS && this.current().lexeme === ')') {
                this.advance();
            }
        }

        if (!this.expectKeyword('then')) return null;

        node.then_branch = this.parseStatement();
        if (!node.then_branch) return null;

        if (this.current() && this.current().type === TokenType.KEYWORD && this.current().lexeme === 'else') {
            this.advance();
            node.else_branch = this.parseStatement();
            if (!node.else_branch) return null;
        }

        return node;
    }

    parseWhileStatement() {
        const node = this.makeNode(NodeType.NODE_WHILE_STATEMENT);
        if (!this.expectKeyword('while')) return null;

        const hasParen = (this.current() && this.current().type === TokenType.SYMBOLS && this.current().lexeme === '(');
        if (hasParen) this.advance();

        node.condition = this.parseLogical();
        if (!node.condition) return null;

        if (hasParen) {
            if (this.current() && this.current().type === TokenType.SYMBOLS && this.current().lexeme === ')') {
                this.advance();
            }
        }

        if (!this.expectKeyword('do')) return null;

        node.then_branch = this.parseStatement();
        if (!node.then_branch) return null;

        return node;
    }

    parseForStatement() {
        const node = this.makeNode(NodeType.NODE_FOR_STATEMENT);
        if (!this.expectKeyword('for')) return null;

        const t = this.current();
        if (!t || t.type !== TokenType.IDENTIFIER) return null;
        node.value = t.lexeme;
        this.advance();

        if (!this.expectOperation('=')) return null;

        node.left = this.parseExpression();
        if (!node.left) return null;

        if (!this.expectKeyword('to')) return null;

        node.right = this.parseExpression();
        if (!node.right) return null;

        if (!this.expectKeyword('do')) return null;

        node.then_branch = this.parseStatement();
        if (!node.then_branch) return null;

        return node;
    }

    parseEcho() {
        const node = this.makeNode(NodeType.NODE_ECHO);
        this.advance(); // consume 'echo'
        node.left = this.parseExpression();
        if (!node.left) return null;
        return node;
    }

    parseArg() {
        const t = this.current();
        if (!t) return null;
        let type;
        switch (t.type) {
            case TokenType.STRING:
                type = NodeType.NODE_FACTOR_STRING;
                break;
            case TokenType.RUTA:
                type = NodeType.NODE_FACTOR_RUTA;
                break;
            case TokenType.IDENTIFIER:
                type = NodeType.NODE_FACTOR_ID;
                break;
            case TokenType.NUMBER:
                type = NodeType.NODE_FACTOR_NUMBER;
                break;
            default:
                return null;
        }
        const node = this.makeNode(type, t.lexeme);
        this.advance();
        return node;
    }

    parseShellCmd() {
        this.advance(); // consume '('
        const node = this.makeNode(NodeType.NODE_SHELL_CMD);
        node.value = this.currentLexeme(); // function name
        this.advance(); // consume function ID

        let tail = null;
        while (this.current() && !(this.current().type === TokenType.SYMBOLS && this.current().lexeme === ')')) {
            const arg = this.parseArg();
            if (!arg) return null;
            if (!node.left) {
                node.left = arg;
                tail = arg;
            } else {
                tail.next = arg;
                tail = arg;
            }
        }

        if (!this.current() || this.current().lexeme !== ')') {
            this.errors.push({ code: 'E-SIN-02', lexeme: node.value });
            return null;
        }
        this.advance(); // consume ')'
        return node;
    }

    parseCondition() {
        const left = this.parseExpression();
        if (!left) return null;

        const op = this.current();
        if (!op || op.type !== TokenType.OPERATION) return null;
        const l = op.lexeme;
        if (!['<', '>', '<=', '>=', '==', '!='].includes(l)) return null;

        const node = this.makeNode(NodeType.NODE_CONDITION, l);
        this.advance(); // consume op

        node.left = left;
        node.right = this.parseExpression();
        if (!node.right) return null;

        return node;
    }

    parseLogical() {
        let left = this.parseCondition();
        if (!left) return null;

        while (this.current() && this.current().type === TokenType.OPERATION &&
            (this.current().lexeme === '&&' || this.current().lexeme === '||')) {
            const node = this.makeNode(NodeType.NODE_LOGICAL, this.currentLexeme());
            this.advance();
            const right = this.parseCondition();
            if (!right) return null;
            node.left = left;
            node.right = right;
            left = node;
        }
        return left;
    }

    parseExpression() {
        let left = this.parseTerm();
        if (!left) return null;

        while (this.current() && this.current().type === TokenType.OPERATION &&
            (this.current().lexeme === '+' || this.current().lexeme === '-')) {
            const node = this.makeNode(NodeType.NODE_BINOP, this.currentLexeme());
            this.advance();
            const right = this.parseTerm();
            if (!right) return null;
            node.left = left;
            node.right = right;
            left = node;
        }
        return left;
    }

    parseTerm() {
        let left = this.parseFactor();
        if (!left) return null;

        while (this.current() && this.current().type === TokenType.OPERATION &&
            (this.current().lexeme === '*' || this.current().lexeme === '/')) {
            const node = this.makeNode(NodeType.NODE_BINOP, this.currentLexeme());
            this.advance();
            const right = this.parseFactor();
            if (!right) return null;
            node.left = left;
            node.right = right;
            left = node;
        }
        return left;
    }

    parseFactor() {
        const t = this.current();
        if (!t) return null;

        // Unary Negation '-'
        if (t.type === TokenType.OPERATION && t.lexeme === '-') {
            this.advance();
            const node = this.makeNode(NodeType.NODE_UNARY_NEG);
            node.left = this.parseFactor();
            if (!node.left) return null;
            return node;
        }

        if (t.type === TokenType.IDENTIFIER) {
            const node = this.makeNode(NodeType.NODE_FACTOR_ID, t.lexeme);
            this.advance();
            return node;
        }

        if (t.type === TokenType.NUMBER) {
            const node = this.makeNode(NodeType.NODE_FACTOR_NUMBER, t.lexeme);
            this.advance();
            return node;
        }

        if (t.type === TokenType.STRING) {
            const node = this.makeNode(NodeType.NODE_FACTOR_STRING, t.lexeme);
            this.advance();
            return node;
        }

        if (t.type === TokenType.RUTA) {
            const node = this.makeNode(NodeType.NODE_FACTOR_RUTA, t.lexeme);
            this.advance();
            return node;
        }

        // Parentheses (expr)
        if (t.type === TokenType.SYMBOLS && t.lexeme === '(') {
            this.advance();
            const inner = this.parseExpression();
            if (!inner) return null;
            if (!this.current() || this.current().lexeme !== ')') return null;
            this.advance(); // consume ')'
            const node = this.makeNode(NodeType.NODE_FACTOR_EXPR);
            node.left = inner;
            return node;
        }

        return null;
    }
}

/* ==========================================================================
   3. ANALIZADOR SEMÁNTICO
   ========================================================================== */

function analyzeSemantics(ast, errors) {
    const definedVars = new Set();

    function visit(node) {
        if (!node) return;

        switch (node.type) {
            case NodeType.NODE_PROGRAM:
                let curr = node.left;
                while (curr) {
                    visit(curr);
                    curr = curr.next;
                }
                break;

            case NodeType.NODE_ASSIGNMENT:
                visit(node.left); // check RHS first
                definedVars.add(node.value); // then RHS is defined
                break;

            case NodeType.NODE_IF_STATEMENT:
                visit(node.condition);
                visit(node.then_branch);
                if (node.else_branch) visit(node.else_branch);
                break;

            case NodeType.NODE_WHILE_STATEMENT:
                visit(node.condition);
                visit(node.then_branch);
                break;

            case NodeType.NODE_FOR_STATEMENT:
                // RHS is loop initializer, check it
                visit(node.left);
                visit(node.right);
                // Define loop variable inside loop
                definedVars.add(node.value);
                visit(node.then_branch);
                break;

            case NodeType.NODE_CONDITION:
            case NodeType.NODE_LOGICAL:
            case NodeType.NODE_BINOP:
                visit(node.left);
                visit(node.right);
                break;

            case NodeType.NODE_FACTOR_EXPR:
            case NodeType.NODE_ECHO:
            case NodeType.NODE_UNARY_NEG:
                visit(node.left);
                break;

            case NodeType.NODE_FACTOR_ID:
                if (!isValidFunction(node.value) && !definedVars.has(node.value)) {
                    errors.push({ code: 'E-SEM-05', lexeme: node.value });
                }
                break;

            case NodeType.NODE_SHELL_CMD: {
                const rule = CMD_RULES[node.value];
                if (rule) {
                    let argc = 0;
                    let a = node.left;
                    while (a) { argc++; a = a.next; }

                    if (argc < rule.min) {
                        errors.push({ code: 'E-SEM-01', lexeme: node.value });
                    } else if (rule.max >= 0 && argc > rule.max) {
                        errors.push({ code: 'E-SEM-02', lexeme: node.value });
                    } else {
                        let arg = node.left;
                        while (arg) {
                            if (rule.expectRuta && arg.type === NodeType.NODE_FACTOR_STRING) {
                                errors.push({ code: 'E-SEM-03', lexeme: arg.value });
                            } else if (arg.type === NodeType.NODE_FACTOR_ID) {
                                visit(arg);
                            }
                            arg = arg.next;
                        }
                    }
                } else {
                    let arg = node.left;
                    while (arg) {
                        visit(arg);
                        arg = arg.next;
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    visit(ast);
}

/* ==========================================================================
   4. GENERACIÓN DE CÓDIGO INTERMEDIO (TAC)
   ========================================================================== */

class TACGenerator {
    constructor() {
        this.instructions = [];
        this.tempCount = 0;
        this.labelCount = 0;
    }

    newTemp() {
        return `t${this.tempCount++}`;
    }

    newLabel() {
        return `L${this.labelCount++}`;
    }

    emit(op, result, arg1, arg2) {
        this.instructions.push({ op, result, arg1, arg2 });
    }

    generate(ast) {
        if (!ast) return [];
        this.genStmt(ast);
        return this.instructions;
    }

    genExpr(node) {
        if (!node) return '?';

        switch (node.type) {
            case NodeType.NODE_FACTOR_NUMBER:
            case NodeType.NODE_FACTOR_STRING:
            case NodeType.NODE_FACTOR_ID:
                return node.value;

            case NodeType.NODE_FACTOR_EXPR:
                return this.genExpr(node.left);

            case NodeType.NODE_UNARY_NEG: {
                const arg = this.genExpr(node.left);
                const t = this.newTemp();
                this.emit('TAC_NEG', t, arg, null);
                return t;
            }

            case NodeType.NODE_BINOP: {
                const l = this.genExpr(node.left);
                const r = this.genExpr(node.right);
                const t = this.newTemp();
                let op;
                if (node.value === '+') op = 'TAC_ADD';
                else if (node.value === '-') op = 'TAC_SUB';
                else if (node.value === '*') op = 'TAC_MUL';
                else op = 'TAC_DIV';
                this.emit(op, t, l, r);
                return t;
            }

            default:
                return '?';
        }
    }

    genCond(node) {
        if (!node) return '?';

        if (node.type === NodeType.NODE_LOGICAL) {
            const l = this.genCond(node.left);
            const r = this.genCond(node.right);
            const t = this.newTemp();
            const op = (node.value === '&&') ? 'TAC_AND' : 'TAC_OR';
            this.emit(op, t, l, r);
            return t;
        }

        if (node.type === NodeType.NODE_CONDITION) {
            const l = this.genExpr(node.left);
            const r = this.genExpr(node.right);
            const t = this.newTemp();
            let op;
            if (node.value === '<') op = 'TAC_LT';
            else if (node.value === '>') op = 'TAC_GT';
            else if (node.value === '<=') op = 'TAC_LE';
            else if (node.value === '>=') op = 'TAC_GE';
            else if (node.value === '==') op = 'TAC_EQ';
            else op = 'TAC_NE';
            this.emit(op, t, l, r);
            return t;
        }

        return this.genExpr(node);
    }

    genStmt(node) {
        if (!node) return;

        switch (node.type) {
            case NodeType.NODE_PROGRAM:
                let s = node.left;
                while (s) {
                    this.genStmt(s);
                    s = s.next;
                }
                break;

            case NodeType.NODE_ASSIGNMENT: {
                const r = this.genExpr(node.left);
                this.emit('TAC_ASSIGN', node.value, r, null);
                break;
            }

            case NodeType.NODE_IF_STATEMENT: {
                const cond = this.genCond(node.condition);
                const lend = this.newLabel();

                if (node.else_branch) {
                    const lelse = this.newLabel();
                    this.emit('TAC_IFNOT_GOTO', lelse, cond, null);
                    this.genStmt(node.then_branch);
                    this.emit('TAC_GOTO', lend, null, null);
                    this.emit('TAC_LABEL', lelse, null, null);
                    this.genStmt(node.else_branch);
                } else {
                    this.emit('TAC_IFNOT_GOTO', lend, cond, null);
                    this.genStmt(node.then_branch);
                }

                this.emit('TAC_LABEL', lend, null, null);
                break;
            }

            case NodeType.NODE_WHILE_STATEMENT: {
                const lstart = this.newLabel();
                const lend = this.newLabel();
                this.emit('TAC_LABEL', lstart, null, null);
                const cond = this.genCond(node.condition);
                this.emit('TAC_IFNOT_GOTO', lend, cond, null);
                this.genStmt(node.then_branch);
                this.emit('TAC_GOTO', lstart, null, null);
                this.emit('TAC_LABEL', lend, null, null);
                break;
            }

            case NodeType.NODE_FOR_STATEMENT: {
                const init = this.genExpr(node.left);
                this.emit('TAC_ASSIGN', node.value, init, null);

                const lstart = this.newLabel();
                const lend = this.newLabel();
                this.emit('TAC_LABEL', lstart, null, null);

                const limit = this.genExpr(node.right);
                const t = this.newTemp();
                this.emit('TAC_LE', t, node.value, limit);
                this.emit('TAC_IFNOT_GOTO', lend, t, null);

                this.genStmt(node.then_branch);

                const t2 = this.newTemp();
                this.emit('TAC_ADD', t2, node.value, '1');
                this.emit('TAC_ASSIGN', node.value, t2, null);

                this.emit('TAC_GOTO', lstart, null, null);
                this.emit('TAC_LABEL', lend, null, null);
                break;
            }

            case NodeType.NODE_SHELL_CMD: {
                let cmd = node.value;
                let arg = node.left;
                while (arg) {
                    let lex = arg.value || '';
                    if (arg.type === NodeType.NODE_FACTOR_STRING && lex.startsWith('\'') && lex.endsWith('\'')) {
                        lex = lex.slice(1, -1); // strip single quotes
                    }
                    cmd += ' ' + lex;
                    arg = arg.next;
                }
                this.emit('TAC_SYSCALL', null, cmd, null);
                break;
            }

            case NodeType.NODE_ECHO: {
                const val = this.genExpr(node.left);
                this.emit('TAC_PRINT', null, val, null);
                break;
            }

            default:
                break;
        }
    }
}

/* ==========================================================================
   5. MÁQUINA VIRTUAL DE INTERPRETACIÓN (VM)
   ========================================================================== */

class Environment {
    constructor() {
        this.variables = {}; // name -> { kind: 'VAL_INT' | 'VAL_STR', value }
        this.stdout = [];
    }

    set(name, kind, value) {
        this.variables[name] = { kind, value };
    }

    get(name) {
        return this.variables[name] || { kind: 'VAL_UNDEF', value: null };
    }

    clearTemps() {
        for (const name in this.variables) {
            if (name.startsWith('t') && /^[0-9]+$/.test(name.slice(1))) {
                delete this.variables[name];
            }
        }
    }
}

function executeTAC(tac, env) {
    if (!tac || tac.length === 0) return 0;

    env.stdout = [];
    env.variables = {}; // Limpia las variables de ejecuciones anteriores para que solo aparezcan las del código actual

    const resolveValue = (s) => {
        if (!s) return { kind: 'VAL_UNDEF', value: null };
        if (/^[0-9\-]+$/.test(s)) {
            return { kind: 'VAL_INT', value: parseInt(s, 10) };
        }
        if (s.startsWith('\'') && s.endsWith('\'')) {
            return { kind: 'VAL_STR', value: s.slice(1, -1) };
        }
        return env.get(s);
    };

    const isTruthy = (v) => {
        if (v.kind === 'VAL_INT') return v.value !== 0;
        if (v.kind === 'VAL_STR') return v.value && v.value !== '';
        return false;
    };

    const findLabelIndex = (label) => {
        for (let i = 0; i < tac.length; i++) {
            if (tac[i].op === 'TAC_LABEL' && tac[i].result === label) {
                return i;
            }
        }
        return -1;
    };

    let pc = 0;
    let steps = 0;
    const MAX_STEPS = 50000;
    let hitLimit = 0;

    while (pc < tac.length) {
        if (steps++ >= MAX_STEPS) {
            hitLimit = 1;
            break;
        }

        const instr = tac[pc];

        switch (instr.op) {
            case 'TAC_ASSIGN': {
                const val = resolveValue(instr.arg1);
                env.set(instr.result, val.kind, val.value);
                pc++;
                break;
            }

            case 'TAC_ADD':
            case 'TAC_SUB':
            case 'TAC_MUL':
            case 'TAC_DIV': {
                const a = resolveValue(instr.arg1);
                const b = resolveValue(instr.arg2);
                let resVal = null;
                if (a.kind === 'VAL_INT' && b.kind === 'VAL_INT') {
                    if (instr.op === 'TAC_ADD') resVal = a.value + b.value;
                    else if (instr.op === 'TAC_SUB') resVal = a.value - b.value;
                    else if (instr.op === 'TAC_MUL') resVal = a.value * b.value;
                    else resVal = b.value !== 0 ? Math.floor(a.value / b.value) : 0;
                }
                env.set(instr.result, 'VAL_INT', resVal);
                pc++;
                break;
            }

            case 'TAC_LT':
            case 'TAC_GT':
            case 'TAC_LE':
            case 'TAC_GE':
            case 'TAC_EQ':
            case 'TAC_NE': {
                const a = resolveValue(instr.arg1);
                const b = resolveValue(instr.arg2);
                let resVal = 0;
                if (a.kind === 'VAL_INT' && b.kind === 'VAL_INT') {
                    if (instr.op === 'TAC_LT') resVal = a.value < b.value ? 1 : 0;
                    else if (instr.op === 'TAC_GT') resVal = a.value > b.value ? 1 : 0;
                    else if (instr.op === 'TAC_LE') resVal = a.value <= b.value ? 1 : 0;
                    else if (instr.op === 'TAC_GE') resVal = a.value >= b.value ? 1 : 0;
                    else if (instr.op === 'TAC_EQ') resVal = a.value === b.value ? 1 : 0;
                    else resVal = a.value !== b.value ? 1 : 0;
                }
                env.set(instr.result, 'VAL_INT', resVal);
                pc++;
                break;
            }

            case 'TAC_AND': {
                const a = resolveValue(instr.arg1);
                const b = resolveValue(instr.arg2);
                const resVal = (isTruthy(a) && isTruthy(b)) ? 1 : 0;
                env.set(instr.result, 'VAL_INT', resVal);
                pc++;
                break;
            }

            case 'TAC_OR': {
                const a = resolveValue(instr.arg1);
                const b = resolveValue(instr.arg2);
                const resVal = (isTruthy(a) || isTruthy(b)) ? 1 : 0;
                env.set(instr.result, 'VAL_INT', resVal);
                pc++;
                break;
            }

            case 'TAC_NEG': {
                const a = resolveValue(instr.arg1);
                let resVal = null;
                if (a.kind === 'VAL_INT') resVal = -a.value;
                env.set(instr.result, 'VAL_INT', resVal);
                pc++;
                break;
            }

            case 'TAC_PRINT': {
                const v = resolveValue(instr.arg1);
                if (v.kind === 'VAL_INT') {
                    env.stdout.push(v.value.toString());
                } else if (v.kind === 'VAL_STR') {
                    env.stdout.push(v.value);
                } else {
                    env.stdout.push('(indefinido)');
                }
                pc++;
                break;
            }

            case 'TAC_SYSCALL': {
                // Simulate syscall command execution
                const fullCmd = instr.arg1;
                env.stdout.push(`[SYSCALL] Ejecutando: ${fullCmd}`);
                const parts = fullCmd.split(' ');
                const cmdName = parts[0];
                if (cmdName === 'mkdir') {
                    env.stdout.push(`  → Directorio creado: ${parts[1] || ''}`);
                } else if (cmdName === 'touch') {
                    env.stdout.push(`  → Archivo creado: ${parts[1] || ''}`);
                } else if (cmdName === 'ls') {
                    env.stdout.push(`  → Listando carpeta: ${parts[1] || '.'}`);
                    env.stdout.push(`    - hola.txt\n    - main.c\n    - Makefile`);
                } else if (cmdName === 'rm') {
                    env.stdout.push(`  → Eliminado: ${parts[1] || ''}`);
                } else {
                    env.stdout.push(`  → Comando completado con éxito.`);
                }
                pc++;
                break;
            }

            case 'TAC_LABEL':
                pc++;
                break;

            case 'TAC_GOTO': {
                const dest = findLabelIndex(instr.result);
                pc = (dest >= 0) ? dest : pc + 1;
                break;
            }

            case 'TAC_IFNOT_GOTO': {
                const cond = resolveValue(instr.arg1);
                if (!isTruthy(cond)) {
                    const dest = findLabelIndex(instr.result);
                    pc = (dest >= 0) ? dest : pc + 1;
                } else {
                    pc++;
                }
                break;
            }

            default:
                pc++;
                break;
        }
    }

    env.clearTemps();
    return hitLimit;
}

/* ==========================================================================
   6. UI CONTROLLER & RENDERERS
   ========================================================================== */

const escapeHtml = (text) => {
    return text
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
};

// Global Environment instance persisting across edits (REPL behavior)
const globalEnv = new Environment();

// Syntax highlighting renderer
function updateSyntaxHighlighting(code) {
    const placeholders = [];
    let tempCode = code;

    const addPlaceholder = (rawText, className) => {
        const id = `___KED_TOKEN_${placeholders.length}___`;
        placeholders.push({
            id,
            html: `<span class="${className}">${escapeHtml(rawText)}</span>`
        });
        return id;
    };

    // 1. Double quotes (unknown/invalid strings)
    tempCode = tempCode.replace(/"[^"\r\n]*"/g, (match) => {
        return addPlaceholder(match, 'syntax-error-token');
    });

    // 2. Single quotes (valid strings)
    tempCode = tempCode.replace(/'[^'\r\n]*'/g, (match) => {
        return addPlaceholder(match, 'syntax-string');
    });

    // 3. Comments (if supported)
    tempCode = tempCode.replace(/\/\/.*/g, (match) => {
        return addPlaceholder(match, 'syntax-comment');
    });

    // 4. Keywords
    keywords.forEach(kw => {
        const regex = new RegExp(`\\b${kw}\\b`, 'g');
        tempCode = tempCode.replace(regex, (match) => {
            return addPlaceholder(match, 'syntax-keyword');
        });
    });

    // 5. Commands
    commands.forEach(cmd => {
        const regex = new RegExp(`\\b${cmd}\\b`, 'g');
        tempCode = tempCode.replace(regex, (match) => {
            return addPlaceholder(match, 'syntax-command');
        });
    });

    // 6. Paths (RUTA) - e.g. /tmp/foo, ./bar
    tempCode = tempCode.replace(/(?:\B\/|\B\.\/|\B\.\.\/)[a-zA-Z0-9/\.\-_]+/g, (match) => {
        return addPlaceholder(match, 'syntax-identifier');
    });

    // 7. Numbers
    tempCode = tempCode.replace(/\b[0-9]+\b/g, (match) => {
        return addPlaceholder(match, 'syntax-number');
    });

    // 8. Operators: +, -, *, /, =, <, >, <=, >=, ==, !=, &&, ||
    tempCode = tempCode.replace(/[+\-*/=<>&|!]+/g, (match) => {
        return addPlaceholder(match, 'syntax-operation');
    });

    // 9. Symbols: ( )
    tempCode = tempCode.replace(/[()]/g, (match) => {
        return addPlaceholder(match, 'syntax-symbol');
    });

    // Escape the remaining content (which is just whitespace and the safe placeholders)
    let escaped = escapeHtml(tempCode);

    // Replace placeholders back with their glowing HTML!
    placeholders.forEach(p => {
        escaped = escaped.replace(p.id, p.html);
    });

    document.getElementById('syntax-highlight').innerHTML = escaped;
}

// Render AST tree recursively
function renderASTNode(node) {
    if (!node) return '';

    const typeClass = node.type;
    const valText = node.value ? escapeHtml(node.value) : '';

    const cardHtml = `
        <div class="ast-node-card ${typeClass}">
            <div class="ast-node-type">${typeClass}</div>
            <div class="ast-node-val">${valText}</div>
        </div>
    `;

    const children = [];
    if (node.type === NodeType.NODE_PROGRAM) {
        let curr = node.left;
        while (curr) {
            children.push(curr);
            curr = curr.next;
        }
    } else {
        if (node.condition) children.push(node.condition);
        if (node.left) children.push(node.left);
        if (node.right) children.push(node.right);
        if (node.then_branch) children.push(node.then_branch);
        if (node.else_branch) children.push(node.else_branch);
    }

    let childrenHtml = '';
    if (children.length > 0) {
        childrenHtml = `
            <div class="ast-children">
                ${children.map(child => `
                    <div class="ast-child">
                        ${renderASTNode(child)}
                    </div>
                `).join('')}
            </div>
        `;
    }

    return `
        <div class="ast-node">
            ${cardHtml}
            ${childrenHtml}
        </div>
    `;
}

// Render TAC formatting
function renderTAC(tacList) {
    const container = document.getElementById('tac-render');
    if (!tacList || tacList.length === 0) {
        container.innerHTML = `<pre><code class="language-tac">  (sin instrucciones TAC generadas)</code></pre>`;
        return;
    }

    let html = '<pre><code class="language-tac">';
    tacList.forEach(i => {
        let line = '  ';
        const wrapVar = (name) => `<span class="tac-var">${name}</span>`;
        const wrapTemp = (name) => `<span class="tac-temp">${name}</span>`;
        const wrapOp = (op) => `<span class="tac-op">${op}</span>`;
        const formatArg = (arg) => {
            if (!arg) return '';
            if (arg.startsWith('t') && /^[0-9]+$/.test(arg.slice(1))) return wrapTemp(arg);
            if (/^[a-zA-Z]/.test(arg)) return wrapVar(arg);
            return escapeHtml(arg);
        };

        const r = i.result;
        const a1 = i.arg1;
        const a2 = i.arg2;

        switch (i.op) {
            case 'TAC_ASSIGN':
                line += `${formatArg(r)} = ${formatArg(a1)}`;
                break;
            case 'TAC_ADD':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('+')} ${formatArg(a2)}`;
                break;
            case 'TAC_SUB':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('-')} ${formatArg(a2)}`;
                break;
            case 'TAC_MUL':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('*')} ${formatArg(a2)}`;
                break;
            case 'TAC_DIV':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('/')} ${formatArg(a2)}`;
                break;
            case 'TAC_LT':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('<')} ${formatArg(a2)}`;
                break;
            case 'TAC_GT':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('>')} ${formatArg(a2)}`;
                break;
            case 'TAC_LE':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('<=')} ${formatArg(a2)}`;
                break;
            case 'TAC_GE':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('>=')} ${formatArg(a2)}`;
                break;
            case 'TAC_EQ':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('==')} ${formatArg(a2)}`;
                break;
            case 'TAC_NE':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('!=')} ${formatArg(a2)}`;
                break;
            case 'TAC_AND':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('&&')} ${formatArg(a2)}`;
                break;
            case 'TAC_OR':
                line += `${formatArg(r)} = ${formatArg(a1)} ${wrapOp('||')} ${formatArg(a2)}`;
                break;
            case 'TAC_NEG':
                line += `${formatArg(r)} = -${formatArg(a1)}`;
                break;
            case 'TAC_PRINT':
                line += `print ${formatArg(a1)}`;
                break;
            case 'TAC_SYSCALL':
                line += `syscall ${escapeHtml(a1)}`;
                break;
            case 'TAC_LABEL':
                line = `<span class="tac-label">${r}:</span>`;
                break;
            case 'TAC_GOTO':
                line += `goto ${r}`;
                break;
            case 'TAC_IFNOT_GOTO':
                line += `ifnot ${formatArg(a1)} goto ${r}`;
                break;
        }

        html += `<span class="tac-line">${line}</span>`;
    });
    html += '</code></pre>';
    container.innerHTML = html;
}

// Render token stream badges
function renderTokensList(tokens) {
    const container = document.getElementById('tokens-render');
    if (!tokens || tokens.length === 0) {
        container.innerHTML = `<div class="empty-state"><p>Sin tokens detectados.</p></div>`;
        return;
    }

    container.innerHTML = tokens.map(t => `
        <div class="token-badge ${t.type}">
            <div class="token-type-label">${t.type}</div>
            <div class="token-lexeme-val" title="${escapeHtml(t.lexeme)}">${escapeHtml(t.lexeme)}</div>
        </div>
    `).join('');
}

// Render active Symbol Table in the VM
function renderSymbolTable(env) {
    const container = document.getElementById('symbols-render');
    const vars = Object.keys(env.variables);

    if (vars.length === 0) {
        container.innerHTML = `<tr><td colspan="4" class="empty-table">Sin variables en memoria</td></tr>`;
        return;
    }

    container.innerHTML = vars.map(name => {
        const item = env.variables[name];
        const displayType = item.kind === 'VAL_INT' ? 'entero' : 'cadena';
        const typeBadgeClass = item.kind === 'VAL_INT' ? 'int' : 'str';
        const displayVal = item.value === null ? '(indefinido)' : item.value;
        return `
            <tr>
                <td><span class="var-badge">${escapeHtml(name)}</span></td>
                <td><span class="type-badge ${typeBadgeClass}">${displayType}</span></td>
                <td><span class="val-view">${escapeHtml(displayVal.toString())}</span></td>
                <td><span class="state-badge">definida</span></td>
            </tr>
        `;
    }).join('');
}

// Main compiler orchestrator pipeline
function runCompiler() {
    const code = document.getElementById('code-input').value;

    // Updates editors states
    document.getElementById('char-count').innerText = `${code.length} caracteres`;
    const linesCount = code.split('\n').length;
    document.getElementById('line-count').innerText = `${linesCount} línea${linesCount > 1 ? 's' : ''}`;

    // Update line numbers list
    let numbersHtml = '';
    for (let i = 1; i <= linesCount; i++) {
        numbersHtml += `<span>${i}</span>`;
    }
    document.getElementById('line-numbers').innerHTML = numbersHtml;

    updateSyntaxHighlighting(code);

    const pipeline = {
        lexer: document.getElementById('status-lexer'),
        parser: document.getElementById('status-parser'),
        semantic: document.getElementById('status-semantic'),
        tac: document.getElementById('status-tac'),
        vm: document.getElementById('status-vm')
    };

    // Reset status nodes
    Object.values(pipeline).forEach(node => {
        node.className = 'status-node active';
    });

    const errorDetails = [];

    // 1. LEXICAL PIPELINE
    const { tokens, errors: lexErrors } = tokenize(code);
    renderTokensList(tokens);

    if (lexErrors.length > 0) {
        pipeline.lexer.className = 'status-node error';
        lexErrors.forEach(e => {
            errorDetails.push({ ...e, stage: 'LÉXICO' });
        });
    }

    // 2. PARSE PIPELINE
    let ast = null;
    if (errorDetails.length === 0) {
        const parseErrors = [];
        const parserInstance = new Parser(tokens, parseErrors);
        try {
            ast = parserInstance.parse();
        } catch (err) {
            console.error(err);
        }

        if (parseErrors.length > 0 || !ast) {
            pipeline.parser.className = 'status-node error';
            parseErrors.forEach(e => {
                errorDetails.push({ ...e, stage: 'SINTÁCTICO' });
            });
        }
    } else {
        pipeline.parser.className = 'status-node';
    }

    // 3. SEMANTIC PIPELINE
    if (errorDetails.length === 0 && ast) {
        const semErrors = [];
        analyzeSemantics(ast, semErrors);
        if (semErrors.length > 0) {
            pipeline.semantic.className = 'status-node error';
            semErrors.forEach(e => {
                errorDetails.push({ ...e, stage: 'SEMÁNTICO' });
            });
        }
    } else {
        pipeline.semantic.className = 'status-node';
    }

    // Render AST Tree
    const astContainer = document.getElementById('ast-render');
    if (ast && errorDetails.length === 0) {
        astContainer.innerHTML = renderASTNode(ast);
    } else {
        astContainer.innerHTML = `<div class="empty-state"><p>Corrige los errores de compilación para ver el AST.</p></div>`;
    }

    // 4. INTERMEDIATE CODE (TAC) PIPELINE
    let tacList = [];
    if (errorDetails.length === 0 && ast) {
        const tacGen = new TACGenerator();
        tacList = tacGen.generate(ast);
        renderTAC(tacList);
    } else {
        pipeline.tac.className = 'status-node';
        renderTAC([]);
    }

    // 5. INTERPRETER VIRTUAL MACHINE PIPELINE
    const consoleContainer = document.getElementById('console-render');
    if (errorDetails.length === 0 && tacList.length > 0) {
        const hitLimit = executeTAC(tacList, globalEnv);
        renderSymbolTable(globalEnv);

        // Terminal text builder
        let terminalLines = `
            <div class="terminal-line"><span class="prompt">$</span> ./kde compile_input.ked</div>
            <div class="terminal-line system-out">Iniciando máquina virtual KED...</div>
        `;
        globalEnv.stdout.forEach(line => {
            if (line.startsWith('[SYSCALL]')) {
                terminalLines += `<div class="terminal-line syscall-out">${escapeHtml(line)}</div>`;
            } else {
                terminalLines += `<div class="terminal-line exec-out">${escapeHtml(line)}</div>`;
            }
        });
        if (hitLimit) {
            terminalLines += `<div class="terminal-line system-out" style="color: var(--error);">ERROR: Límite de pasos alcanzado en la VM (posible bucle infinito)</div>`;
            pipeline.vm.className = 'status-node error';
        }
        consoleContainer.innerHTML = terminalLines;
    } else {
        pipeline.vm.className = 'status-node';
        renderSymbolTable(globalEnv);
        consoleContainer.innerHTML = `
            <div class="terminal-line"><span class="prompt">$</span> ./kde compile_input.ked</div>
            <div class="terminal-line system-out" style="color: var(--error);">Fallo en la compilación. Revisa la barra inferior para ver los detalles.</div>
        `;
    }

    // 6. RENDER COMPILATION STATUS FOOTER
    const errorBar = document.getElementById('error-bar');
    const summary = document.getElementById('error-summary');
    const list = document.getElementById('error-details-list');

    if (errorDetails.length === 0) {
        summary.className = 'error-summary green';
        summary.innerHTML = `<span class="error-icon">✓</span><span class="error-text">Compilación exitosa. Sin errores léxicos, sintácticos ni semánticos en el código.</span>`;
        list.style.display = 'none';
    } else {
        summary.className = 'error-summary red';
        summary.innerHTML = `<span class="error-icon">✗</span><span class="error-text">Fallo de compilación: Se encontraron ${errorDetails.length} error${errorDetails.length > 1 ? 'es' : ''}. Haz clic aquí para ver detalles.</span>`;

        list.innerHTML = errorDetails.map(err => {
            const desc = ErrorDescriptions[err.code] || 'Detalle no clasificado.';
            return `
                <div class="error-card">
                    <div class="error-card-main">
                        <span class="error-code">${err.code}</span>
                        <span class="error-msg">${desc}</span>
                    </div>
                    <div class="error-lexeme-zone">
                        Elemento: <span class="error-lexeme">${escapeHtml(err.lexeme)}</span> [Etapa: <b>${err.stage}</b>]
                    </div>
                </div>
            `;
        }).join('');
        list.style.display = 'flex';
    }
}

// Invoke the C Compiler through the local Python Bridge server
function runCompilerOnC() {
    const code = document.getElementById('code-input').value;
    const runBtn = document.getElementById('run-btn');

    // UI Loading state
    const originalText = runBtn.innerHTML;
    runBtn.innerHTML = '<span class="pulse" style="background: var(--accent); display:inline-block; width:8px; height:8px; border-radius:50%; margin-right:8px;"></span> C...';
    runBtn.disabled = true;

    fetch('/api/compile', {
        method: 'POST',
        headers: {
            'Content-Type': 'text/plain'
        },
        body: code
    })
        .then(res => res.json())
        .then(data => {
            runBtn.innerHTML = originalText;
            runBtn.disabled = false;

            if (!data.success) {
                alert("Error ejecutando compilador de C: " + data.error);
                return;
            }

            // 1. Render AST (C format)
            const astContainer = document.getElementById('ast-render');
            astContainer.innerHTML = `<pre class="c-txt-output" style="width:100%; text-align:left; font-family:var(--font-mono); font-size:13px; line-height:1.4; color:var(--warning); overflow:auto; margin:0; padding:10px;"><code>${escapeHtml(data.ast)}</code></pre>`;

            // 2. Render TAC (C format)
            const tacContainer = document.getElementById('tac-render');
            tacContainer.innerHTML = `<pre class="c-txt-output" style="width:100%; text-align:left; font-family:var(--font-mono); font-size:13px; line-height:1.4; color:#bb9af7; overflow:auto; margin:0; padding:10px;"><code>${escapeHtml(data.tac)}</code></pre>`;

            // 3. Render Virtual Terminal / Console Output
            const consoleContainer = document.getElementById('console-render');
            let terminalLines = `
            <div class="terminal-line"><span class="prompt">$</span> wsl ./kde compile_input.ked</div>
            <div class="terminal-line system-out" style="color: var(--accent);">Iniciando compilador de C nativo...</div>
        `;

            if (data.output) {
                data.output.split('\n').forEach(line => {
                    terminalLines += `<div class="terminal-line exec-out" style="color: var(--success); font-family:var(--font-mono);">${escapeHtml(line)}</div>`;
                });
            }
            consoleContainer.innerHTML = terminalLines;

            // 4. Render Symbols Table (C format)
            const symbolsContainer = document.getElementById('symbols-render');
            if (data.symbols) {
                let html = '';
                const lines = data.symbols.split('\n');
                const dataLines = lines.filter(l => l.trim() !== '' && !l.includes('Nombre') && !l.includes('Tabla de simbolos') && !l.includes('---------'));
                if (dataLines.length === 0 || dataLines[0].includes('(vacia)')) {
                    symbolsContainer.innerHTML = `<tr><td colspan="4" class="empty-table">Sin variables en memoria C</td></tr>`;
                } else {
                    dataLines.forEach(line => {
                        const tokens = line.trim().split(/\s+/);
                        if (tokens.length >= 3) {
                            const name = tokens[0];
                            const type = tokens[1];
                            const state = tokens[2];
                            const typeBadgeClass = type === 'VAR' ? 'int' : 'str';
                            const displayType = type === 'VAR' ? 'entero' : 'función';
                            const stateClass = state === 'definida' ? 'state-badge' : 'state-badge error';
                            html += `
                            <tr>
                                <td><span class="var-badge">${escapeHtml(name)}</span></td>
                                <td><span class="type-badge ${typeBadgeClass}">${escapeHtml(displayType)}</span></td>
                                <td><span class="val-view">-</span></td>
                                <td><span class="${stateClass}">${escapeHtml(state)}</span></td>
                            </tr>
                        `;
                        }
                    });
                    symbolsContainer.innerHTML = html || `<tr><td colspan="4" class="empty-table">Sin variables en memoria C</td></tr>`;
                }
            }

            // 5. Update Status pipeline nodes based on C errors
            const pipeline = {
                lexer: document.getElementById('status-lexer'),
                parser: document.getElementById('status-parser'),
                semantic: document.getElementById('status-semantic'),
                tac: document.getElementById('status-tac'),
                vm: document.getElementById('status-vm')
            };

            // Reset
            Object.values(pipeline).forEach(node => {
                node.className = 'status-node active';
            });

            // 6. Update Compiler Status Footer
            const summary = document.getElementById('error-summary');
            const list = document.getElementById('error-details-list');

            if (data.errors.includes("Sin errores")) {
                summary.className = 'error-summary green';
                summary.innerHTML = `<span class="error-icon">✓</span><span class="error-text">Compilación C exitosa. Código nativo validado sin errores.</span>`;
                list.style.display = 'none';
            } else {
                summary.className = 'error-summary red';
                summary.innerHTML = `<span class="error-icon">✗</span><span class="error-text">Fallo de compilación en C: Clic aquí para ver los detalles.</span>`;

                let errorsHtml = '';
                data.errors.split('\n').forEach(line => {
                    if (!line.trim()) return;
                    const match = line.match(/\[(E-[A-Z]+-\d+)\](.*)/);
                    const code = match ? match[1] : "ERROR";
                    const msg = match ? match[2] : line.trim();

                    if (code.includes("LEX")) pipeline.lexer.className = 'status-node error';
                    if (code.includes("SIN")) pipeline.parser.className = 'status-node error';
                    if (code.includes("SEM")) pipeline.semantic.className = 'status-node error';

                    errorsHtml += `
                    <div class="error-card">
                        <div class="error-card-main">
                            <span class="error-code">${escapeHtml(code)}</span>
                            <span class="error-msg">${escapeHtml(msg)}</span>
                        </div>
                        <div class="error-lexeme-zone">
                            Origen: <b>Compilador Nativo C (WSL)</b>
                        </div>
                    </div>
                `;
                });
                list.innerHTML = errorsHtml;
                list.style.display = 'flex';
            }
        })
        .catch(err => {
            runBtn.innerHTML = originalText;
            runBtn.disabled = false;
            console.error("C Bridge falló, usando compilación JS:", err);
            runCompiler();
        });
}

// Debounce timer for keystrokes in the editor
let debounceTimer;
function onCodeChange() {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(runCompiler, 150);
}

/* ==========================================================================
   INITIALIZATION & BINDING
   ========================================================================== */

document.addEventListener('DOMContentLoaded', () => {
    const inputTextArea = document.getElementById('code-input');
    const runBtn = document.getElementById('run-btn');
    const select = document.getElementById('example-select');

    // Live typing real-time compiler
    inputTextArea.addEventListener('input', onCodeChange);

    // Sync scrolls between overlay highlighter and actual textarea
    inputTextArea.addEventListener('scroll', () => {
        const highlight = document.getElementById('syntax-highlight');
        highlight.scrollTop = inputTextArea.scrollTop;
        highlight.scrollLeft = inputTextArea.scrollLeft;
    });

    // Run button trigger (Bridge to C if running on local server, else run client-side JS)
    runBtn.addEventListener('click', () => {
        if (window.location.protocol === 'http:' || window.location.protocol === 'https:') {
            runCompilerOnC();
        } else {
            runCompiler();
        }
    });

    // Dropdown Example selector trigger
    select.addEventListener('change', (e) => {
        const key = e.target.value;
        if (CodeTemplates[key]) {
            inputTextArea.value = CodeTemplates[key];
            // Clear environment when changing files to prevent leakage
            globalEnv.variables = {};
            runCompiler();
        }
    });

    // Error details toggling
    document.getElementById('error-summary').addEventListener('click', () => {
        const list = document.getElementById('error-details-list');
        if (list.childNodes.length > 0 && list.style.display === 'none') {
            list.style.display = 'flex';
        } else {
            list.style.display = 'none';
        }
    });

    // Tab buttons active swapping
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.addEventListener('click', () => {
            document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
            document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));

            btn.classList.add('active');
            const tabId = btn.getAttribute('data-tab');
            document.getElementById(tabId).classList.add('active');
        });
    });

    // Initialize with a simple greeting template
    inputTextArea.value = CodeTemplates['aritmetica'];
    runCompiler();
});
