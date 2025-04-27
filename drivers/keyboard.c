/**
 * @file keyboard.c
 * @author Joe Bayer (joexbayer)
 * @brief Really simple PS/2 Keyboard driver with US keyboard layout.
 * @see http://www.osdever.net/bkerndev/Docs/keyboard.htm
 * @version 0.1
 * @date 2022-06-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <arch/interrupts.h>
#include <arch/io.h>
#include <keyboard.h>
#include <kutils.h>
#include <libc.h>
#include <pcb.h>
#include <scheduler.h>
#include <serial.h>
#include <sync.h>

#include <vbe.h>

#define KB_IRQ 33 /* Default is 1, 33 after mapped. */
#define KB_BUFFER_SIZE 255

static mutex_t kb_lock;
static unsigned char kb_buffer[KB_BUFFER_SIZE];
static volatile int kb_buffer_head = 0;
static volatile int kb_buffer_tail = 0;

static unsigned char kbdus[128] = {
    0,          27,          '1', '2', '3',  '4', '5', '6', '7',  '8', /* 9 */
    '9',        '0',         '-', '=', '\b', /* Backspace */
    '\t',                                    /* Tab */
    'q',        'w',         'e', 'r',       /* 19 */
    't',        'y',         'u', 'i', 'o',  'p', '[', ']', '\n', /* Enter key
                                                                   */
    0, /* 29   - Control */
    'a',        's',         'd', 'f', 'g',  'h', 'j', 'k', 'l',  ';', /* 39 */
    '\'',       '`',         0,                        /* Left shift */
    '\\',       'z',         'x', 'c', 'v',  'b', 'n', /* 49 */
    'm',        ',',         '.', '/', 0,              /* Right shift */
    '*',        0,                                     /* Alt */
    ' ',                                               /* Space bar */
    0,                                                 /* Caps lock */
    F1,                                                /* 59 - F1 key ... > */
    F2,         F3,          F4,  F5,  F6,   F7,  F8,  F9,  F10, /* < ... F10 */
    0,                               /* 69 - Num lock*/
    0,                               /* Scroll Lock */
    0,                               /* Home key */
    ARROW_UP,                        /* Up Arrow */
    0,                               /* Page Up */
    '-',        ARROW_LEFT,          /* Left Arrow */
    0,          ARROW_RIGHT,         /* Right Arrow */
    '+',        0,                   /* 79 - End key*/
    ARROW_DOWN,                      /* Down Arrow */
    0,                               /* Page Down */
    0,                               /* Insert Key */
    0,                               /* Delete Key */
    0,          0,           0,   0, /* F11 Key */
    0,                               /* F12 Key */
    0,                               /* All other keys are undefined */
};
static int __keyboard_presses = 0;
static uint8_t __shift_pressed = 0;
static uint8_t __alt_pressed = 0;
static uint8_t __ctrl_pressed = 0;
static uint8_t __super_pressed = 0;

unsigned char kb_get_char(int spin) {
  acquire(&kb_lock);

  if (spin) {

    while (kb_buffer_tail == kb_buffer_head) {
      release(&kb_lock);

      kernel_yield();

      acquire(&kb_lock);
    }

  } else {
    if (kb_buffer_tail == kb_buffer_head) {
      release(&kb_lock);
      return 0;
    }
  }

  unsigned char c = kb_buffer[kb_buffer_tail];
  kb_buffer_tail = (kb_buffer_tail + 1) % KB_BUFFER_SIZE;

  release(&kb_lock);
  return c;
}

void kb_add_char(unsigned char c) {
  kb_buffer[kb_buffer_head] = c;
  kb_buffer_head = (kb_buffer_head + 1) % KB_BUFFER_SIZE;
}

static void __int_handler kb_callback() {
  uint8_t scancode =
      inportb(0x60); /* Receive scancode, also ACK's interrupt? */

  switch (scancode) {
  case 0x2a: /* shift down */
    __shift_pressed = 1;
    return;
  case 0xaa: /* shift up */
    __shift_pressed = 0;
    return;
  case 224: /* shift down */
    __alt_pressed = 1;
    return;
  case 184: /* shift up */
    __alt_pressed = 0;
    return;
  case 29: /* ctrl down */
    __ctrl_pressed = 1;
    return;
  case 157: /* ctrl up */
    __ctrl_pressed = 0;
    return;
  case 91: /* windows key */
    __super_pressed = 1;
    return;
  case 219: /* windows key up */
    __super_pressed = 0;
    return;
  default:
    break;
  }

  if (scancode & 0x80)
    return;

  /* CTRL: down 29 up 157 */

  unsigned char c = kbdus[scancode];
  if (c == '7' && __shift_pressed) {
    kb_add_char('/');
  } else if (c == '8' && __shift_pressed) {
    kb_add_char('(');
  } else if (c == '9' && __shift_pressed) {
    kb_add_char(')');
  } else if (c == ',' && __shift_pressed) {
    kb_add_char(';');
  } else if (c == '0' && __shift_pressed) {
    kb_add_char('=');
  } else if (c == '7' && __shift_pressed) {
    kb_add_char('/');
  } else if (c == '8' && __alt_pressed) {
    kb_add_char('[');
  } else if (c == '9' && __alt_pressed) {
    kb_add_char(']');
  } else if (c == '7' && __alt_pressed) {
    kb_add_char('{');
  } else if (c == '0' && __alt_pressed) {
    kb_add_char('}');
  } else if (c == '2' && __shift_pressed) {
    kb_add_char('"');
  } else if (c == 92 && __shift_pressed) {
    kb_add_char('*');
  } else if (__ctrl_pressed) {
    kb_add_char(128 + c);
  } else {
    kb_add_char(__shift_pressed ? c + ('A' - 'a') : c);
  }
  __keyboard_presses++;
}

static void keyboard_buffer_clear() {
  while (inportb(0x64) & 1) {
    inportb(0x60); /* Read and discard */
  }
}

void init_keyboard() {
  mutex_init(&kb_lock);

  outportb(0x64, 0xAD); /* Disable first PS/2 port */
  keyboard_buffer_clear(); 
  outportb(0x64, 0xAE); /* Enable port */

  while (inportb(0x64) & 2); 
  outportb(0x60, 0xFF);

  int timeout = 100000;
  while ((inportb(0x64) & 1) == 0 && --timeout);

  if (timeout) {
    uint8_t ack = inportb(0x60);
    dbgprintf("[PS/2] Keyboard reset response: 0x%x\n", ack);
  } else {
    uint8_t status = inportb(0x64);
    if (status & 1) {
      uint8_t leftover = inportb(0x60);
      dbgprintf("[PS/2] Late data after timeout: 0x%x\n", leftover);
    } else {
      dbgprintf("[PS/2] No keyboard reset response (status: 0x%x).\n", status);
    }
  }

  keyboard_buffer_clear();

  interrupt_install_handler(KB_IRQ, &kb_callback);
  dbgprintf("[PS/2] Keyboard handler installed.\n");
}