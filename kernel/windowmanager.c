#include "util.h"
#include <windowmanager.h>
#include <screen.h>
#include <serial.h>
#include <pcb.h>

#define USABLE_WIDTH (SCREEN_WIDTH-1)
#define USABLE_HEIGHT (SCREEN_HEIGHT-1)

struct window_binary_tree {
	struct window_binary_tree* right;
	struct window_binary_tree* root;
	struct window_binary_tree* left;
    struct window* value;
};

static struct window windows[MAX_NUM_OF_PCBS];
static struct window_binary_tree root;

void split_screen() 
{
	
}

void clear_window(struct window* w)
{
    for (uint8_t i = 0; i < w->width; i++)
    {
        scrput(w->x+i, w->y, ' ', w->color);
        scrput(w->x+i, w->y+w->height, ' ', w->color);
    }
   
    for (uint8_t i = 0; i < w->height; i++)
    {
        scrput(w->x, w->y+i, ' ', w->color);
        scrput(w->x+w->width, w->y+i, ' ', w->color);
    }
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

struct window_binary_tree* new_node()
{
    struct window_binary_tree* node = (struct window_binary_tree*) alloc(sizeof(struct window_binary_tree));
    node->root = NULL;
    node->left = NULL;
    node->right = NULL;
    node->value = NULL;
    return node;

}

int init_window(struct window* w, int x, int y, int width, int height)
{
    w->x = x;
    w->y = y;
    w->color = VGA_COLOR_LIGHT_GREY;
    memcpy(w->name, "WINDOW", strlen("WINDOW"));
    w->visable = 1;
    w->width = width;
    w->height = height;
    w->state.color = VGA_COLOR_LIGHT_GREY;
    w->state.column = 0;
    w->state.row = USABLE_HEIGHT-1;

    return 1;
}   

int window_split_horizontal(struct window_binary_tree* wbt)
{

    clear_window(wbt->value);

    /* General idea: take the root of wbt and split its window left and right */
    int l_width = wbt->value->width / 2;
    struct window* left = wbt->left->value;
    struct window* right = wbt->right->value;

    /* This assumes that the left window is already "defined" and right not.*/

    init_window(left, left->x, left->y, l_width, left->height);
    init_window(right, l_width+2, left->y, l_width, left->height);
    right->color = VGA_COLOR_GREEN;
    return 0;
}

int attach_window(struct window* w)
{
    //current_running->window = w;
    //return 0;
	/* This function should create a window for the process requesting it.
	 * Importantly it should tile them by dividing the current main window in two.
	 * Storing the windows in a binary tree.
	 */
    current_running->window = &windows[current_running->pid];
    /* Setup root window if nothing else is defined. */
	if(root.root == NULL && root.left == NULL && root.root == NULL && root.value == NULL)
	{
		root.value = current_running->window;
		root.value->height = USABLE_HEIGHT-2;
		root.value->width = USABLE_WIDTH-2;
		root.value->visable = 1;
		root.value->color = VGA_COLOR_LIGHT_GREY;
		memcpy(root.value->name, "WINDOW", strlen("WINDOW"));
		root.value->x = 1;
		root.value->y = 1;
		root.value->state.color = VGA_COLOR_LIGHT_GREY;
		root.value->state.column = 0;
		root.value->state.row = USABLE_HEIGHT-1;
		return 1;
	}
   
    /* Will assume left AND right is NULL, and root never is.*/
    struct window_binary_tree* current = &root;
    while(current->left != NULL && current->right != NULL)
       current = current->left;

    current->left = new_node();
    current->left->root = current;
    current->left->value = current->value;

    current->right = new_node();
    current->right->root = current;
    current->right->value = current_running->window;

    window_split_horizontal(current);

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
