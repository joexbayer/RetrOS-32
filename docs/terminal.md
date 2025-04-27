# Terminal API Documentation

- See the code for all properties and methods [terminal.h](https://github.com/joexbayer/RetrOS-32/blob/main/include/terminal.h).

### struct terminal
Represents a terminal object with its properties and actions.

#### Usage
```c
struct terminal* term = terminal_create(TERMINAL_TEXT_MODE);
term->ops->attach(term);
```

This attaches the terminal to the current process.
After that, calling twritef will write to the terminal.

Most importantly the terminal is inherited by child processes.
Meaning child processes calling twritef will write to the same terminal.

## Enums

### terminal_flag_t
Flags representing certain properties a terminal can have.

- `TERMINAL_FLAG_NONE`: No flags.
- `TERMINAL_TEXT_MODE`: The terminal is in text mode.
- `TERMINAL_GRAPHICS_MODE`: The terminal is in graphics mode.

## Structs

### terminal_ops
Struct for terminal operations.
These can be overwritten by a kernel thread, but doing so may reduce the usability of the terminal.

- `int (*writef)(struct terminal* term, char* fmt, ...);`: Writes formatted text to the terminal.

#### Other useful operations
- `void (*write)(struct terminal*, char* text)`: Writes text to the terminal.
- `void (*clear)(struct terminal*)`: Clears the terminal.
- `void (*destroy)(struct terminal*)`: Destroys the terminal.
s
## Example
Example of how the terminal ops can be overwritten by a kernel thread.
```c
static int __net_terminal_writef(struct terminal* term, char* fmt, ...);

...

struct terminal* term = terminal_create(TERMINAL_GRAPHICS_MODE);
if(term == NULL){
    dbgprintf("Unable to create terminal\n");
    return;
}

struct terminal_ops ops = {
    .writef = __net_terminal_writef
};
term->ops->set(term, &ops);
```

In this example, all calls to twritef will be redirected to the __net_terminal_writef function instead.
Meaning we can then forward the text to a network socket. Thereby creating a network terminal.