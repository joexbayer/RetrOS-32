#ifndef __INTERPRETER_LEX_H
#define __INTERPRETER_LEX_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// types of variable/function
enum { 
    CHAR,
    INT,
    PTR
};

void* program(int* _text, char* _data, char* _str);
char* lex_get_error();
int lex_get_error_line();
void lex_init();

#ifdef __cplusplus
}
#endif


#endif /* !__INTERPRETER_LEX_H */
