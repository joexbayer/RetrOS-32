#include <windowmanager.h>
#include <screen.h>
#include <serial.h>
#include <pcb.h>

#define USABLE_WIDTH (SCREEN_WIDTH-1)
#define USABLE_HEIGHT (SCREEN_HEIGHT-1)

struct window_binary_tree {
	struct window* right;
	struct window* root;
	struct window* left;
};

static struct window windows[MAX_NUM_OF_PCBS];
static struct window_binary_tree root;

void split_screen() {
	
}

void draw_window(struct window* w)
{
    if(w->visable == 0)
        return;

    //dbgprintf("[WM] Window: Anchor %d, width: %d, height: %d\n", w->x, w->width, w->height);

    for (uint8_t i = 0; i < w->width; i++)
    {
        scrput(w->x+i, w->y, 205, w->color);
        scrput(w->x+i, w->y+w->height, 196, w->color);
    }

    for (uint8_t i = 0; i < w->height; i++)
    {
        scrput(w->x, w->y+i, 179, w->color);
        scrput(w->x+w->width, w->y+i, 179, w->color);
    }

    scrput(w->x, w->y, 213, w->color);
    scrput(w->x+w->width, w->y, 184, w->color);

    scrput(w->x, w->y+w->height, 192, w->color);
    scrput(w->x+w->width, w->y+w->height, 217, w->color);
    scrprintf(w->x+2, w->y, w->name);
}

int attach_window(struct window* w)
{
    current_running->window = w;
    return 0;
	/* This function should create a window for the process requesting it.
	 * Importantly it should tile them by dividing the current main window in two.
	 * Storing the windows in a binary tree.
	 */
    current_running->window = &windows[current_running->pid];
    /* Setup window to based on current windows */
	if(root.root == NULL)
	{
		root.root = current_running->window;
		root.root->height = USABLE_HEIGHT;
		root.root->width = USABLE_WIDTH;
		root.root->visable = 1;
		root.root->color = VGA_COLOR_LIGHT_GREY;
		memcpy(root.root->name, "WINDOW", strlen("WINDOW"));
		root.root->x = 1;
		root.root->y = 1;
		root.root->state.color = VGA_COLOR_LIGHT_GREY;
		root.root->state.column = 0;
		root.root->state.row = USABLE_HEIGHT-1;
		return 1;
	}

    return 1;
}

int get_window_height()
{
    if(current_running->window != NULL)
        return current_running->window->height;

    return SCREEN_HEIGHT;
}

int get_window_width()
{
    if(current_running->window != NULL)
        return current_running->window->width;

    return SCREEN_WIDTH;
}

uint8_t is_window_visable()
{
    if(current_running->window != NULL)
        return current_running->window->visable;
    
    return 0;
        
}

struct terminal_state* get_terminal_state()
{
    return &current_running->window->state;
}

void init_wm(){
	root.root = NULL;
	root.left = NULL;
	root.right = NULL;
}
