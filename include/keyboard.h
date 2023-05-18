#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define ARROW_UP 254
#define ARROW_DOWN 253
#define ARROW_LEFT 252
#define ARROW_RIGHT 251
#define F1 250
#define F2 249
#define F3 248

void init_keyboard();
unsigned char kb_get_char();

#endif