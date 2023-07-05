# GFX Library - Window API Documentation

### struct window
Represents a window object with its properties and actions.
Mainly needed to draw to the window using its draw operations.

- See the code for all properties and methods [window.h](https://github.com/joexbayer/NETOS/blob/main/include/gfx/window.h).

## Functions

### gfx_new_window
Creates a new window.

- `struct window* gfx_new_window(int width, int height, window_flag_t flags)`

#### Usage
```c
struct window* win = gfx_new_window(250, 200, GFX_IS_RESIZABLE);
win->draw->rect(win, 0, 0, 250, 200, COLOR_VGA_GREEN);
```
Example: [kclock](https://github.com/joexbayer/NETOS/blob/main/kernel/kthreads/kclock.c).

## Enums

### window_state_t
Represents the motion status of a window.

- `GFX_WINDOW_MOVING`: The window is in a moving state.
- `GFX_WINDOW_STATIC`: The window is in a static state.

### window_flag_t
Flags representing certain properties a window can have.

- `GFX_IS_RESIZABLE`: The window is resizable.
- `GFX_IS_IMMUATABLE`: The window cannot be modified.
- `GFX_IS_MOVABLE`: The window can be moved.

## Structs

### window_ops
Struct for window operations.
These can be overwritten by a kernel thread, but doing so may reduce the usability of the window.
Hover and click are used to move the window. For receiving window interactions use the GFX events (link to gfx events)

- `void (*click)(struct window*, int x, int y)`: Triggered when the window is clicked.
- `void (*hover)(struct window*, int x, int y)`: Triggered when mouse hovers over the window.
- `void (*mousedown)(struct window*, int x, int y)`: Triggered when mouse button is pressed down on the window.
- `void (*mouseup)(struct window*, int x, int y)`: Triggered when mouse button is released over the window.
- `void (*resize)(struct window*, int width, int height)`: Resizes the window.
- `void (*move)(struct window*, int x, int y)`: Moved window to given x, y coordinates

### window_draw_ops
Struct for window drawing operations.

- `void (*draw)(struct window*)`: Draws the window.
- `void (*rect)(struct window*, int x, int y, int width, int height, color_t color)`: Draws a rectangle.
- `void (*textf)(struct window*, int x, int y, color_t color, char* fmt, ...)`: Formats and outputs text.
- `void (*text)(struct window*, int x, int y, char* text, color_t color)`: Outputs text.
- `void (*line)(struct window*, int x1, int y1, int x2, int y2, color_t color)`: Draws a line.
- `void (*circle)(struct window*, int x, int y, int radius, color_t color)`: Draws a circle.