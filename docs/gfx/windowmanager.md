# GFX Library - Window Manager API Documentation

## Macros

### WM_VALIDATE
Macro to validate that all operations in a window manager are not NULL. 

- Usage:
WM_VALIDATE(wm)
If any operation is NULL, it returns -ERROR_OPS_CORRUPTED.

### WM_VALIDATE_FLAGS
Macro to validate the flags of a window manager.

- Usage:
WM_VALIDATE_FLAGS(wm)
It returns -ERROR_INVALID_ARGUMENTS if any flags other than WM_FULLSCREEN or WM_RESIZABLE are set.

## Enums

### windowmanager_states
Enumeration defines the states of the window manager. 

- WM_UNUSED: The window manager is not used.
- WM_ACTIVE: The window manager is active.
- WM_INITIALIZED: The window manager has been initialized.

### windowmanager_flags
Enumeration defines the flags for the window manager. 

- WM_FULLSCREEN: The window is in fullscreen mode.
- WM_RESIZABLE: The window is resizable.

## Structs

### windowmanager_ops
Struct for window manager operations.

- int (*add)(struct windowmanager *wm, struct window *window): Adds a new window.
- int (*remove)(struct windowmanager *wm, struct window *window): Removes a window.
- int (*draw)(struct windowmanager *wm, struct window *window): Draws all windows.
- int (*push_front)(struct windowmanager *wm, struct window *window): Pushes a window to the front.
- int (*mouse_event)(struct windowmanager *wm, int x, int y, char flags): Handles mouse events.

### windowmanager
Structure defines a window manager. It includes properties for operation management, composition buffer size and its buffer, windows and their count, spinlock, state and flags of the window manager, mouse state and its properties.

- See the code for all properties [windowmanager.h](https://github.com/joexbayer/NETOS/blob/main/include/windowmanager.h).

## Functions

### init_windowmanager
This function is used to initialize the window manager with the given flags.

- int init_windowmanager(struct windowmanager* wm, int flags)

#### Usage
```c
struct windowmanager* wm = kalloc(sizeof(struct windowmanager));
init_windowmanager(wm, WM_FULLSCREEN | WM_RESIZABLE);
```

Main: [windowmanager.c](https://github.com/joexbayer/NETOS/blob/main/kernel/windowmanager.c).
