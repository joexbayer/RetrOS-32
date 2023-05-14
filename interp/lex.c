/**
 * @file lex.c
 * @author Joe Bayer (joexbayer)
 * @brief Lexer for the minmial C interpreter
 * @version 0.1
 * @date 2023-05-14
 * @see https://github.com/rswier/c4
 * @see https://github.com/lotabout/write-a-C-interpreter
 * @copyright Copyright (c) 2023
 * 
 */

#include "lex.h"
#include "vm.h"
#include "interpreter.h"

#define int long

struct identifier {
    int token;
    int hash;
    char * name;
    int class;
    int type;
    long value;
    int Bclass;
    int Btype;
    int Bvalue;
};

struct lexer {
    int token;
    int token_val;

    struct identifier symbols[LEX_MAX_SYMBOLS];
    struct identifier* current_id;
    struct identifier* main;

    int basetype;    // the type of a declaration, make it global for convenience
    int expr_type;   // the type of an expression

    int index_of_bp; // index of bp pointer on stack

} lexer_context;

char *src;
int line = 1;
char* data;
int* text;
struct identifier* idmain;

int basetype;    // the type of a declaration, make it global for convenience
int expr_type;   // the type of an expression

// function frame
//
// 0: arg 1
// 1: arg 2
// 2: arg 3
// 3: return address
// 4: old bp pointer  <- index_of_bp
// 5: local var 1
// 6: local var 2
int index_of_bp; // index of bp pointer on stack

struct identifier* current_id;
struct identifier symbols[LEX_MAX_SYMBOLS];

void match(int tk) {
    if (lexer_context.token == tk) {
        next();
        return;
    }
    printf("%d: expected lexer_context.token: %d\n", line, tk);
    exit(-1);
}

void expression(int level) {
    DEBUG_PRINT("expression\n");
    // expressions have various format.
    // but majorly can be divided into two parts: unit and operator
    // for example `(char) *a[10] = (int *) func(b > 0 ? 10 : 20);
    // `a[10]` is an unit while `*` is an operator.
    // `func(...)` in total is an unit.
    // so we should first parse those unit and unary operators
    // and then the binary ones
    //
    // also the expression can be in the following types:
    //
    // 1. unit_unary ::= unit | unit unary_op | unary_op unit
    // 2. expr ::= unit_unary (bin_op unit_unary ...)

    // unit_unary()
    struct identifier* id;
    int tmp;
    int *addr;
    {
        if (!lexer_context.token) {
            printf("%d: unexpected lexer_context.lexer_context.token EOF of expression\n", line);
            exit(-1);
        }
        if (lexer_context.token == Num) {
            match(Num);
            DEBUG_PRINT("number %d\n", lexer_context.token_val);

            // emit code
            *++text = IMM;
            *++text = lexer_context.token_val;
            expr_type = INT;
        }
        else if (lexer_context.token == '"') {
            // continous string "abc" "abc"
            // emit code
            *++text = IMM;
            *++text = lexer_context.token_val;

            //DEBUG_PRINT("expression string %s\n", lexer_context.token_val);

            match('"');
            // store the rest strings
            while (lexer_context.token == '"') {
                DEBUG_PRINT("expression string %c\n", lexer_context.token);
                match('"');
            }

            // append the end of string character '\0', all the data are default
            // to 0, so just move data one position forward.
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
            expr_type = PTR;
        } else if (lexer_context.token == Sizeof) {
              DEBUG_PRINT("Sizeof\n");
            // sizeof is actually an unary operator
            // now only `sizeof(int)`, `sizeof(char)` and `sizeof(*...)` are
            // supported.
            match(Sizeof);
            match('(');
            expr_type = INT;

            if (lexer_context.token == Int) {
                match(Int);
            } else if (lexer_context.token == Char) {
                match(Char);
                expr_type = CHAR;
            }

            while (lexer_context.token == Mul) {
                match(Mul);
                expr_type = expr_type + PTR;
            }

            match(')');

            // emit code
            *++text = IMM;
            *++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);

            expr_type = INT;
        } else if (lexer_context.token == Id) {
            // there are several type when occurs to Id
            // but this is unit, so it can only be
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable
            match(Id);

            id = current_id;

            if (lexer_context.token == '(') {
                // function call
                match('(');

                // pass in arguments
                tmp = 0; // number of arguments
                while (lexer_context.token != ')') {
                    expression(Assign);
                    *++text = PUSH;
                    tmp ++;

                    if (lexer_context.token == ',') {
                        match(',');
                    }

                }
                match(')');

                DEBUG_PRINT("New function\n");

                // emit code
                if (id->class == Sys) {
                    // system functions
                    *++text = id->value;
                }
                else if (id->class == Fun) {
                    // function call
                    *++text = CALL;
                    *++text = id->value;
                }
                else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }

                // clean the stack for arguments
                if (tmp > 0) {
                    *++text = ADJ;
                    *++text = tmp;
                }
                expr_type = id->type;
            }
            else if (id->class == Num) {
                // enum variable
                *++text = IMM;
                *++text = id->value;
                expr_type = INT;
            }
            else {
                // variable
                if (id->class == Loc) {
                    *++text = LEA;
                    *++text = index_of_bp - (int)id->value;
                }
                else if (id->class == Glo) {
                    *++text = IMM;
                    *++text = id->value;
                }
                else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }

                // emit code, default behaviour is to load the value of the
                // address which is stored in `ax`
                expr_type = id->type;
                *++text = (expr_type == CHAR) ? LC : LI;
            }
        }
        else if (lexer_context.token == '(') {
            // cast or parenthesis
            match('(');
            if (lexer_context.token == Int || lexer_context.token == Char) {
                tmp = (lexer_context.token == Char) ? CHAR : INT; // cast type
                match(lexer_context.token);
                while (lexer_context.token == Mul) {
                    match(Mul);
                    tmp = tmp + PTR;
                }

                match(')');

                expression(Inc); // cast has precedence as Inc(++)

                expr_type  = tmp;
            } else {
                // normal parenthesis
                expression(Assign);
                match(')');
            }
        }
        else if (lexer_context.token == Mul) {
            // dereference *<addr>
            match(Mul);
            expression(Inc); // dereference has the same precedence as Inc(++)

            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            *++text = (expr_type == CHAR) ? LC : LI;
        }
        else if (lexer_context.token == And) {
            // get the address of
            match(And);
            expression(Inc); // get the address of
            if (*text == LC || *text == LI) {
                text --;
            } else {
                printf("%d: bad address of\n", line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        }
        else if (lexer_context.token == '!') {
            // not
            match('!');
            expression(Inc);

            // emit code, use <expr> == 0
            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            expr_type = INT;
        }
        else if (lexer_context.token == '~') {
            // bitwise not
            match('~');
            expression(Inc);

            // emit code, use <expr> XOR -1
            *++text = PUSH;
            *++text = IMM;
            *++text = -1;
            *++text = XOR;

            expr_type = INT;
        }
        else if (lexer_context.token == Add) {
            // +var, do nothing
            match(Add);
            expression(Inc);

            expr_type = INT;
        }
        else if (lexer_context.token == Sub) {
            // -var
            match(Sub);

            if (lexer_context.token == Num) {
                *++text = IMM;
                *++text = -lexer_context.token_val;
                match(Num);
            } else {

                *++text = IMM;
                *++text = -1;
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
            }

            expr_type = INT;
        }
        else if (lexer_context.token == Inc || lexer_context.token == Dec) {
            tmp = lexer_context.token;
            match(lexer_context.token);
            expression(Inc);
            if (*text == LC) {
                *text = PUSH;  // to duplicate the address
                *++text = LC;
            } else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line);
                exit(-1);
            }
            *++text = PUSH;
            *++text = IMM;
            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        }
        else {
            printf("%d: bad expression\n", line);
            exit(-1);
        }
    }

    // binary operator and postfix operators.
    {
        while (lexer_context.token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (lexer_context.token == Assign) {
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) {
                    *text = PUSH; // save the lvalue's pointer
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }
                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
            }
            else if (lexer_context.token == Cond) {
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (lexer_context.token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            }
            else if (lexer_context.token == Lor) {
                // logic or
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (lexer_context.token == Lan) {
                // logic and
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
            }
            else if (lexer_context.token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;
            }
            else if (lexer_context.token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;
            }
            else if (lexer_context.token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;
            }
            else if (lexer_context.token == Eq) {
                // equal ==
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;
            }
            else if (lexer_context.token == Ne) {
                // not equal !=
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;
            }
            else if (lexer_context.token == Lt) {
                // less than
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;
            }
            else if (lexer_context.token == Gt) {
                // greater than
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;
            }
            else if (lexer_context.token == Le) {
                // less than or equal to
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;
            }
            else if (lexer_context.token == Ge) {
                // greater than or equal to
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;
            }
            else if (lexer_context.token == Shl) {
                // shift left
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;
            }
            else if (lexer_context.token == Shr) {
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
            }
            else if (lexer_context.token == Add) {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
            }
            else if (lexer_context.token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }
            }
            else if (lexer_context.token == Mul) {
                // multiply
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            }
            else if (lexer_context.token == Div) {
                // divide
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
            }
            else if (lexer_context.token == Mod) {
                // Modulo
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;
            }
            else if (lexer_context.token == Inc || lexer_context.token == Dec) {
                // postfix inc(++) and dec(--)
                // we will increase the value to the variable and decrease it
                // on `ax` to get its original value.
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (lexer_context.token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (lexer_context.token == Inc) ? SUB : ADD;
                match(lexer_context.token);
            }
            else if (lexer_context.token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) {
                    // pointer, `not char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }
                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;
            }
            else {
                printf("%d: compiler error, lexer_context.token = %d\n", line, lexer_context.token);
                exit(-1);
            }
        }
    }
}


void next() {
    char *last_pos;
    int hash;

    DEBUG_PRINT("next lexer_context.token parsing... %c %c\n", lexer_context.token, *src);
    while (lexer_context.token = *src) {
        ++src;
        DEBUG_PRINT("Token %c\n", lexer_context.token);

        // parse lexer_context.token here
        if (lexer_context.token == '\n') {
            ++line;
            DEBUG_PRINT("new line\n");
        }
        else if (lexer_context.token == '#') {
            // skip macro, because we will not support it
            
            DEBUG_PRINT("skip # macro\n");
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        else if ((lexer_context.token >= 'a' && lexer_context.token <= 'z') || (lexer_context.token >= 'A' && lexer_context.token <= 'Z') || (lexer_context.token == '_')) {

            DEBUG_PRINT("Parsing new identifier\n");
            // parse identifier
            last_pos = src - 1;
            hash = lexer_context.token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }
            DEBUG_PRINT("hash %d %d\n", hash, src - last_pos);

            // look for existing identifier, linear search
            current_id = &symbols[0];
            while (current_id->token) {
                if (current_id->hash == hash && !memcmp((char *)current_id->name, last_pos, src - last_pos)) {
                    //found one, return
                    lexer_context.token = current_id->token;
                    //DEBUG_PRINT("Found identifier %s\n", current_id->name);
                    return;
                }
                current_id += 1;
            }



            // store new ID
            current_id->name = last_pos;
            current_id->hash = hash;
            lexer_context.token = current_id->token = Id;
            //DEBUG_PRINT("new identifier %s\n", current_id->name);
            return;
        }
        else if (lexer_context.token >= '0' && lexer_context.token <= '9') {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            lexer_context.token_val = lexer_context.token - '0';
            if (lexer_context.token_val > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    lexer_context.token_val = lexer_context.token_val*10 + *src++ - '0';
                }
            } else {
                // starts with 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    lexer_context.token = *++src;
                    while ((lexer_context.token >= '0' && lexer_context.token <= '9') || (lexer_context.token >= 'a' && lexer_context.token <= 'f') || (lexer_context.token >= 'A' && lexer_context.token <= 'F')) {
                        lexer_context.token_val = lexer_context.token_val * 16 + (lexer_context.token & 15) + (lexer_context.token >= 'A' ? 9 : 0);
                        lexer_context.token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        lexer_context.token_val = lexer_context.token_val*8 + *src++ - '0';
                    }
                }
            }

            lexer_context.token = Num;
            return;
        }
        else if (lexer_context.token == '"' || lexer_context.token == '\'') {
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into data.
            last_pos = data;
            while (*src != 0 && *src != lexer_context.token) {
                lexer_context.token_val = *src++;
                if (lexer_context.token_val == '\\') {
                    // escape character
                    lexer_context.token_val = *src++;
                    if (lexer_context.token_val == 'n') {
                        lexer_context.token_val = '\n';
                    }
                }

                if (lexer_context.token == '"') {
                    *data++ = lexer_context.token_val;
                }
            }

            src++;
            // if it is a single character, return Num lexer_context.token
            if (lexer_context.token == '"') {
                lexer_context.token_val = (int)last_pos;
            } else {
                lexer_context.token = Num;
            }

            return;
        }
        else if (lexer_context.token == '/') {
            if (*src == '/') {
                // skip comments
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                // divide operator
                lexer_context.token = Div;
                return;
            }
        }
        else if (lexer_context.token == '=') {
            // parse '==' and '='
            if (*src == '=') {
                src ++;
                lexer_context.token = Eq;
            } else {
                lexer_context.token = Assign;
            }
            return;
        }
        else if (lexer_context.token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                src ++;
                lexer_context.token = Inc;
            } else {
                lexer_context.token = Add;
            }
            return;
        }
        else if (lexer_context.token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                src ++;
                lexer_context.token = Dec;
            } else {
                lexer_context.token = Sub;
            }
            return;
        }
        else if (lexer_context.token == '!') {
            // parse '!='
            if (*src == '=') {
                src++;
                lexer_context.token = Ne;
            }
            return;
        }
        else if (lexer_context.token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                lexer_context.token = Le;
            } else if (*src == '<') {
                src ++;
                lexer_context.token = Shl;
            } else {
                lexer_context.token = Lt;
            }
            return;
        }
        else if (lexer_context.token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                lexer_context.token = Ge;
            } else if (*src == '>') {
                src ++;
                lexer_context.token = Shr;
            } else {
                lexer_context.token = Gt;
            }
            return;
        }
        else if (lexer_context.token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                lexer_context.token = Lor;
            } else {
                lexer_context.token = Or;
            }
            return;
        }
        else if (lexer_context.token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                lexer_context.token = Lan;
            } else {
                lexer_context.token = And;
            }
            return;
        }
        else if (lexer_context.token == '^') {
            lexer_context.token = Xor;
            return;
        }
        else if (lexer_context.token == '%') {
            lexer_context.token = Mod;
            return;
        }
        else if (lexer_context.token == '*') {
            lexer_context.token = Mul;
            return;
        }
        else if (lexer_context.token == '[') {
            lexer_context.token = Brak;
            return;
        }
        else if (lexer_context.token == '?') {
            lexer_context.token = Cond;
            return;
        }
        else if (lexer_context.token == '~' || lexer_context.token == ';' || lexer_context.token == '{' || lexer_context.token == '}' || lexer_context.token == '(' || lexer_context.token == ')' || lexer_context.token == ']' || lexer_context.token == ',' || lexer_context.token == ':') {
            // directly return the character as lexer_context.token;
            return;
        }
    }
    return;
}

void statement() {
    DEBUG_PRINT("statement\n");
    // there are 6 kinds of statements here:
    // 1. if (...) <statement> [else <statement>]
    // 2. while (...) <statement>
    // 3. { <statement> }
    // 4. return xxx;
    // 5. <empty statement>;
    // 6. expression; (expression end with semicolon)

    int *a, *b; // bess for branch control

    if (lexer_context.token == If) {
        // if (...) <statement> [else <statement>]
        //
        //   if (...)           <cond>
        //                      JZ a
        //     <statement>      <statement>
        //   else:              JMP b
        // a:                 a:
        //     <statement>      <statement>
        // b:                 b:
        //
        //
        match(If);
        match('(');
        DEBUG_PRINT("IF expression\n");
        expression(Assign);  // parse condition
        match(')');

        // emit code for if
        *++text = JZ;
        b = ++text;

        DEBUG_PRINT("IF statement\n");
        statement();         // parse statement
        if (lexer_context.token == Else) { // parse else
            match(Else);

            // emit code for JMP B
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;

            DEBUG_PRINT("if else statementn");
            statement();
        }

        *b = (int)(text + 1);
    }
    else if (lexer_context.token == While) {
        DEBUG_PRINT("whiles\n");
        //
        // a:                     a:
        //    while (<cond>)        <cond>
        //                          JZ b
        //     <statement>          <statement>
        //                          JMP a
        // b:                     b:
        match(While);

        a = text + 1;

        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text;

        statement();

        *++text = JMP;
        *++text = (int)a;
        *b = (int)(text + 1);
    }
    else if (lexer_context.token == '{') {
        // { <statement> ... }
        match('{');

        while (lexer_context.token != '}') {
            statement();
        }

        match('}');
    }
    else if (lexer_context.token == Return) {
        // return [expression];
        match(Return);

        if (lexer_context.token != ';') {
            expression(Assign);
        }

        match(';');

        // emit code for return
        *++text = LEV;
    }
    else if (lexer_context.token == ';') {
        // empty statement
        match(';');
    }
    else {
        // a = b; or function_call();
        expression(Assign);
        match(';');
    }
}

void function_parameter() {

    DEBUG_PRINT("function_parameter\n");
    int type;
    int params;
    params = 0;
    while (lexer_context.token != ')') {
        // int name, ...
        type = INT;
        if (lexer_context.token == Int) {
            match(Int);
        } else if (lexer_context.token == Char) {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (lexer_context.token == Mul) {
            match(Mul);
            type = type + PTR;
        }

        // parameter name
        if (lexer_context.token != Id) {
            printf("%d: bad parameter declaration\n", line);
            exit(-1);
        }
        if (current_id->class == Loc) {
            printf("%d: duplicate parameter declaration\n", line);
            exit(-1);
        }

        match(Id);
        // store the local variable
        DEBUG_PRINT("store the local variable\n");
        current_id->Bclass = current_id->class; current_id->class  = Loc;
        current_id->Btype  = current_id->type;  current_id->type   = type;
        current_id->Bvalue = current_id->value; current_id->value  = params++;   // index of current parameter

        if (lexer_context.token == ',') {
            match(',');
        }
    }
    index_of_bp = params+1;
}

void function_body() {
    DEBUG_PRINT("function_body\n");
    // type func_name (...) {...}
    //                   -->|   |<--

    // ... {
    // 1. local declarations
    // 2. statements
    // }

    int pos_local; // position of local variables on the stack.
    int type;
    pos_local = index_of_bp;

    while (lexer_context.token == Int || lexer_context.token == Char) {
        // local variable declaration, just like global ones.
        basetype = (lexer_context.token == Int) ? INT : CHAR;

        match(lexer_context.token);
        while (lexer_context.token != ';') {
            type = basetype;
            while (lexer_context.token == Mul) {
                match(Mul);
                type = type + PTR;
            }

            if (lexer_context.token != Id) {
                // invalid declaration
                printf("%d: bad local declaration\n", line);
                exit(-1);
            }
            if (current_id->class == Loc) {
                // identifier exists
                printf("%d: duplicate local declaration\n", line);
                exit(-1);
            }
            match(Id);


            DEBUG_PRINT("store the local variable\n");
            // store the local variable
            current_id->Bclass = current_id->class; current_id->class  = Loc;
            current_id->Btype  = current_id->type;  current_id->type   = type;
            current_id->Bvalue = current_id->value; current_id->value  = ++pos_local;   // index of current parameter

            if (lexer_context.token == ',') {
                match(',');
            }
        }
        match(';');
    }

    DEBUG_PRINT("save the stack size for local variables %x\n", text);
    // save the stack size for local variables
    *++text = ENT;
    *++text = pos_local - index_of_bp;

    // statements
    //DEBUG_PRINT("statements\n");
    while (lexer_context.token != '}') {
        statement();
    }

    // emit code for leaving the sub function
    *++text = LEV;
}

void function_declaration() {
    // type func_name (...) {...}
    //               | this part
    DEBUG_PRINT("function_declaration...\n");

    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();
    //match('}');

    // unwind local variable declarations for all local variables.
    current_id = symbols;
    while (current_id->token) {
        if (current_id->class == Loc) {
            current_id->class = current_id->Bclass;
            current_id->type  = current_id->Btype;
            current_id->value = current_id->Bvalue;
        }
        current_id = current_id + 1;
    }
}

void enum_declaration() {
    // parse enum [id] { a = 1, b = 3, ...}
    int i;
    i = 0;
    while (lexer_context.token != '}') {
        if (lexer_context.token != Id) {
            printf("%d: bad enum identifier %d\n", line, lexer_context.token);
            exit(-1);
        }
        next();
        if (lexer_context.token == Assign) {
            // like {a=10}
            next();
            if (lexer_context.token != Num) {
                printf("%d: bad enum initializer\n", line);
                exit(-1);
            }
            i = lexer_context.token_val;
            next();
        }

        current_id->class = Num;
        current_id->type = INT;
        current_id->value = i++;

        if (lexer_context.token == ',') {
            next();
        }
    }
}

void global_declaration() {
    // global_declaration ::= enum_decl | variable_decl | function_decl
    //
    // enum_decl ::= 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num'} '}'
    //
    // variable_decl ::= type {'*'} id { ',' {'*'} id } ';'
    //
    // function_decl ::= type {'*'} id '(' parameter_decl ')' '{' body_decl '}'


    DEBUG_PRINT("global_declaration\n");

    int type; // tmp, actual type for variable
    int i; // tmp

    basetype = INT;

    // parse enum, this should be treated alone.
    if (lexer_context.token == Enum) {
        // enum [id] { a = 10, b = 20, ... }
        match(Enum);
        if (lexer_context.token != '{') {
            match(Id); // skip the [id] part
        }
        if (lexer_context.token == '{') {
            // parse the assign part
            match('{');
            enum_declaration();
            match('}');
        }

        DEBUG_PRINT("enum parsed\n");

        match(';');
        return;
    }

    // parse type information
    if (lexer_context.token == Int) {
        match(Int);
        DEBUG_PRINT("int matched\n");
    }
    else if (lexer_context.token == Char) {
        match(Char);
        basetype = CHAR;
        DEBUG_PRINT("char matched\n");
    }

    // parse the comma seperated variable declaration.
    while (lexer_context.token != ';' && lexer_context.token != '}') {
        type = basetype;
        // parse pointer type, note that there may exist `int ****x;`
        while (lexer_context.token == Mul) {
            match(Mul);
            type = type + PTR;
            DEBUG_PRINT("ptr type\n");
        }

        if (lexer_context.token != Id) {
            // invalid declaration
            printf("%d: bad global declaration\n", line);
            exit(-1);
        }
        if (current_id->class) {
            // identifier exists
            printf("%d: duplicate global declaration %s\n", line, current_id->name);
            exit(-1);
        }
        match(Id);
        current_id->type = type;
        DEBUG_PRINT("current type %d\n", type);

        if (lexer_context.token == '(') {
            current_id->class = Fun;
            current_id->value = (void*)(text + 1); // the memory address of function
            DEBUG_PRINT("function_declaration\n");
            function_declaration();
        } else {
            // variable declaration
            DEBUG_PRINT("variable declaration\n");
            current_id->class = Glo; // global variable
            current_id->value = (void*)data; // assign memory address
            data = data + sizeof(int);
        }

        if (lexer_context.token == ',') {
            match(',');
        }
    }
    next();
}


void lex_init()
{
    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

     // add keywords to symbol table
    int i = Char;
    while (i <= While) {
        next();
        current_id->token = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) { 
        next();
        current_id->class = Sys;
        current_id->type = INT;
        current_id->value = i++;
    }

    next(); current_id->token = Char; // handle void type
    next(); idmain = current_id; // keep track of main
    printf("main %s\n", idmain->name);

    DEBUG_PRINT("Added important default lexer_context.tokens and system lexer_context.tokens\n");
}

void* program(char* _text, char* _data, char* _str)
{
    // get next lexer_context.token
    text = _text;
    data = _data;
    src = _str;

    next();
    while (lexer_context.token > 0) {
        global_declaration();
    }
    return idmain->value;
}


