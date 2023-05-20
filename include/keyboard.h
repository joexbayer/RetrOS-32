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
#define F4 247
#define F5 246
#define F6 245
#define F7 244
#define F8 243
#define F9 242
#define F10 241

void init_keyboard();
unsigned char kb_get_char();

#endif