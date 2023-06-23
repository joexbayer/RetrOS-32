/**
 * @file composition.c
 * @author Joe Bayer (joexbayer)
 * @brief Window Server composition and window manage api.
 * @version 0.1
 * @date 2023-01-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <vbe.h>
#include <gfx/window.h>
#include <gfx/gfxlib.h>
#include <gfx/events.h>
#include <gfx/theme.h>
#include <keyboard.h>
#include <scheduler.h>
#include <util.h>
#include <memory.h>
#include <serial.h>
#include <rtc.h>
#include <timer.h>
#include <colors.h>
#include <mouse.h>
#include <gfx/component.h>
#include <sync.h>
#include <assert.h>
#include <math.h>
#include <net/net.h>
#include <arch/interrupts.h>

#include <diskdev.h>
#include <net/netdev.h>

static struct window_server {
    uint8_t sleep_time;
    uint8_t* composition_buffer;
    struct gfx_window* order;
    mutex_t order_lock;
} wind = {
    .sleep_time = 2
};

/* Mouse globals */
static char gfx_mouse_state = 0;
static struct mouse m;

int gfx_check_changes(struct gfx_window* w)
{
    while(w != NULL){
        if(w->changed)
            return 1;
        w = w->next;
    }
    return 0;
}

void gfx_recursive_draw(struct gfx_window* w)
{
    if(w->next != NULL)
        gfx_recursive_draw(w->next);
    
    gfx_draw_window(wind.composition_buffer, w);
}

/**
 * @brief Push a window to the front of "wind.order" list.
 * Changing its z-axis position in the framebuffer.
 * 
 * Assuming w is NOT already the first element.
 * @param w 
 */
static void gfx_window_push_front(struct gfx_window* w)
{
    acquire(&wind.order_lock);

    assert(w != wind.order);

    /* remove w from wind.order list */
    for (struct gfx_window* i = wind.order; i != NULL; i = i->next){
        if(i->next == w){
            i->next = w->next;
            break;
        }
    }
    
    /* Replace wind.order with w, pushing old wind.order back. */
    wind.order->in_focus = 0;
    struct gfx_window* save = wind.order;
    wind.order = w;
    w->next = save;
    wind.order->in_focus = 1;

    w->changed = 1;

    release(&wind.order_lock);
}

/**
 * @brief Removes a window from the "wind.order" list.
 * Important keep the wind.order list intact even if removing the first element.
 * @param w 
 */
void gfx_composition_remove_window(struct gfx_window* w)
{
    acquire(&wind.order_lock);

    if(wind.order == w)
    {
        wind.order = w->next;
        if(wind.order != NULL){
            wind.order->changed = 1;
            wind.order->in_focus = 1;
        }
        goto gfx_composition_remove_window_exit;
    }

    struct gfx_window* iter = wind.order;
    while(iter != NULL && iter->next != w)
        iter = iter->next;
    if(iter == NULL){
        goto gfx_composition_remove_window_exit;
    }

    iter->next = w->next;
    
gfx_composition_remove_window_exit:

    dbgprintf("[GFX] Removing window\n");
    release(&wind.order_lock);
}

/**
 * @brief Adds new window in the "wind.order" list.
 * 
 * @param w 
 */
void gfx_composition_add_window(struct gfx_window* w)
{
    acquire(&wind.order_lock);

    if(wind.order == NULL){
        wind.order = w;
        wind.order->in_focus = 1;
        release(&wind.order_lock);
        return;
    }
    
    struct gfx_window* iter = wind.order;
    wind.order->in_focus = 0;
    wind.order = w;

    wind.order->in_focus = 1;
    wind.order->next = iter;

    release(&wind.order_lock);
}

/**
 * @brief raw mouse event handler for window API.
 * 
 * @param x 
 * @param y 
 * @param flags 
 */
void gfx_mouse_event(int x, int y, char flags)
{
    for (struct gfx_window* i = wind.order; i != NULL; i = i->next)
        if(gfx_point_in_rectangle(i->x, i->y, i->x+i->width, i->y+i->height, x, y)){
            /* on click when left mouse down */
            if(flags & 1 && gfx_mouse_state == 0){
                gfx_mouse_state = 1;
                i->mousedown(i, x, y);

                /* If clicked window is not in front, push it. */
                if(i != wind.order){
                    gfx_window_push_front(i);
                }

                /* TODO: push mouse gfx event to window */

            } else if(!(flags & 1) && gfx_mouse_state == 1) {
                /* If mouse state is "down" send click event */
                gfx_mouse_state = 0;
                i->click(i, x, y);
                i->mouseup(i, x, y);

                uint16_t new_x = CLAMP( (x - (i->x+8)), 0,  i->inner_width);
                uint16_t new_y = CLAMP( (y - (i->y+8)), 0,  i->inner_height);

                struct gfx_event e = {
                    .data = new_x,
                    .data2 = new_y,
                    .event = GFX_EVENT_MOUSE
                };
                gfx_push_event(wind.order, &e);
            }

            i->hover(i, x, y);
            return;
        }
    
    /* No window was clicked. */
}
static int __is_fullscreen = 0;
static void* inner_window_save;
void gfx_set_fullscreen(struct gfx_window* w)
{
    if(w != wind.order){
        dbgprintf("Cannot fullscreen window that isnt in focus\n");
        return;
    }

    /* store and backup original window information */
    w->inner_width = vbe_info->width-16;
    w->inner_height = vbe_info->height-16;

    inner_window_save = w->inner;
    w->inner = wind.composition_buffer+(vbe_info->width*8)+8;
    w->pitch = vbe_info->width;
    w->y = 8;

    dbgprintf("%s is now in fullscreen\n", w->name);

    __is_fullscreen = 1;
}

void gfx_unset_fullscreen(struct gfx_window* w)
{
    if(w != wind.order){
        dbgprintf("Cannot fullscreen window that isnt in focus\n");
        return;
    }

    /* store and backup original window information */
    w->inner_width = w->width-16;
    w->inner_height = w->height-16;

    w->inner = inner_window_save;
    w->pitch = w->inner_width;
    w->y = 10;
    w->x = 10;

    dbgprintf("%s is now not in fullscreen\n", w->name);

    __is_fullscreen = 0;
}

/* Helper function for displaying progress */
static int __calculate_relative_difference(int a, int b) {
    int diff = ABS(a - b);
    int max = MAX(a, b);

    if (max == 0)
        return 1;

    int ratio = (diff * 8) / max;
    ratio = (ratio >= 1) ? ratio : 1;

    return 8 - ratio;
}

/**
 * @brief Main window server kthread entry function
 * Allocates a second framebuffer that will be memcpy'd to the VGA framebuffer.
 * 
 * Handles mouse events and pushes them to correct window.
 * Only redraws screen if a window has changed.
 * 
 * In current state its still very slow.
 */
#define TIME_PREFIX(unit) unit < 10 ? "0" : ""

#define TIMELINE_SIZE 5
struct gfx_timeline {
    char timeline[TIMELINE_SIZE];
    int tick;
    color_t bg;
    color_t fg;
    int start;
};

void gfx_timeline_draw(struct gfx_timeline* tl)
{
    vesa_fillrect(wind.composition_buffer, tl->start, 0, 8*TIMELINE_SIZE, 8,  (kernel_gfx_current_theme())->os.background);
    for (int i = 0; i < TIMELINE_SIZE; i++){
        if(tl->timeline[i] != 0)
            vesa_put_block(wind.composition_buffer, tl->timeline[i], tl->start+(i*8), 0,  tl->fg);
    }
}

void gfx_timeline_update(struct gfx_timeline* tl, char value)
{
    if(value == 0) return;

    if(tl->timeline[TIMELINE_SIZE-1] != value){
        tl->timeline[TIMELINE_SIZE-1] = value;
        memcpy(&tl->timeline[0], &tl->timeline[1], TIMELINE_SIZE-1);
        return;
    }
    tl->tick++;

    if(tl->tick == 50){
        memcpy(&tl->timeline[0], &tl->timeline[1], TIMELINE_SIZE-1);
        tl->tick = 0;
    }
}

void create_circle_pattern(uint8_t* buffer, int width, int height, int centerX, int centerY, int radius, int color) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dx = x - centerX;
            int dy = y - centerY;
            int distanceSquared = dx * dx + dy * dy;
            if (distanceSquared <= radius * radius) {
                vesa_fillrect(buffer, x, y, 1, 1, color);
            }
        }
    }
}


void create_checkerboard(uint8_t* buffer, int width, int height, int cellSize, int color1, int color2) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int cellX = x / cellSize;
            int cellY = y / cellSize;
            int color = ((cellX + cellY) % 2 == 0) ? color1 : color2;
            vesa_fillrect(buffer, x, y, 1, 1, color);
        }
    }
}

void create_old_computer_background(uint8_t* buffer, int width, int height, int gridSize, int lineInterval, int lineColor) {
    memset(buffer, COLOR_VGA_BG, width*height);

    // Draw horizontal lines
    for (int y = lineInterval; y < height; y += lineInterval) {
        for (int x = 0; x < width; x++) {
            buffer[y * width + x] = lineColor;
        }
    }

    // Draw vertical lines
    for (int x = lineInterval; x < width; x += lineInterval) {
        for (int y = 0; y < height; y++) {
            buffer[y * width + x] = lineColor;
        }
    }
}


static char* background;

void gfx_compositor_main()
{
    int buffer_size = vbe_info->width*vbe_info->height*(vbe_info->bpp/8)+1;

    dbgprintf("[WSERVER] %d bytes allocated for composition buffer.\n", buffer_size);
    wind.composition_buffer = (uint8_t*) palloc(buffer_size);
    background = (uint8_t*) kalloc(buffer_size);
    //create_checkerboard(background, vbe_info->width, vbe_info->height, 50, COLOR_VGA_GREEN, COLOR_VGA_MISC);
    //create_circle_pattern(background, vbe_info->width, vbe_info->height, vbe_info->width/2, vbe_info->height/2, 100, COLOR_VGA_MISC);

    create_old_computer_background(background, vbe_info->width, vbe_info->height, 160, 16, 191);

    //gfx_set_fullscreen(wind.order);

    struct gfx_timeline memory_timeline = {
        .fg = COLOR_VGA_GREEN,
        .bg = COLOR_VGA_BG,
        .start = 100
    };

    int net_send_last_diff = 0;
    int net_send_last = 0;
    int net_send = 0;
    struct gfx_timeline net_send_timeline = {
        .fg = COLOR_VGA_MISC,
        .bg = COLOR_VGA_BG,
        .start = 160
    };

    int net_recv_last_diff = 0;
    int net_recv_last = 0;
    int net_recv = 0;
    struct gfx_timeline net_recv_timeline = {
        .fg = COLOR_VGA_RED,
        .bg = COLOR_VGA_BG,
        .start = 220
    };

    /* Main composition loop */
    while(1){
        struct gfx_theme* theme = kernel_gfx_current_theme();
        
        /**
         * Problem with interrupts from mouse?
         * Manages to corrupt some register or variables.
         * 
         * Should also NOT redraw entire screen, only things that change.
         */
        //ENTER_CRITICAL();
        int test = rdtsc();
        int mouse_changed = mouse_event_get(&m);
        int window_changed = gfx_check_changes(wind.order);
        struct time time;
        get_current_time(&time);
        
        
        unsigned char key = kb_get_char();
        if(key != 0){

            if(key == F10){
                if(!__is_fullscreen) {
                    gfx_set_fullscreen(wind.order);
                } else {
                    gfx_unset_fullscreen(wind.order);
                }
                struct gfx_event e = {
                    .data = wind.order->inner_width,
                    .data2 = wind.order->inner_height,
                    .event = GFX_EVENT_RESOLUTION
                };
                gfx_push_event(wind.order, &e);
            } else {
                struct gfx_event e = {
                    .data = key,
                    .event = GFX_EVENT_KEYBOARD
                };
                gfx_push_event(wind.order, &e);
            }
        }

        if(window_changed && !__is_fullscreen){
            
            //memset(wind.composition_buffer, theme->os.background/*41*/, buffer_size);
            memcpy(wind.composition_buffer, background, buffer_size);


            for (int i = 0; i < (vbe_info->width/8) - 2; i++){
                vesa_put_box(wind.composition_buffer, 80, 8+(i*8), 0, theme->os.foreground);
                vesa_put_box(wind.composition_buffer, 0, 8+(i*8), vbe_info->height-8, theme->os.foreground);
            }

            for (int i = 0; i < (vbe_info->height/8)-2; i++){
                vesa_put_box(wind.composition_buffer, 2, 0, 8+(i*8), theme->os.foreground);
                vesa_put_box(wind.composition_buffer, 2, vbe_info->width-8, 8+(i*8), theme->os.foreground);
            }

            vesa_put_box(wind.composition_buffer, 82, 0, 0, theme->os.foreground);
            vesa_put_box(wind.composition_buffer, 85, vbe_info->width-8, 0, theme->os.foreground);

            /* bottom left and right corners*/
            vesa_put_box(wind.composition_buffer, 20, 0, vbe_info->height-8, theme->os.foreground);
            vesa_put_box(wind.composition_buffer, 24, vbe_info->width-8, vbe_info->height-8, theme->os.foreground);

            vesa_fillrect(wind.composition_buffer, 8, 0, strlen("NETOS")*8, 8, theme->os.foreground);
            vesa_write_str(wind.composition_buffer, 8, 0, "NETOS",  theme->os.text);
        }

        vesa_fillrect(wind.composition_buffer, vbe_info->width-strlen("00:00:00 00/00/00")*8 - 16, 0, strlen("00:00:00 00/00/00")*8, 8, theme->os.foreground);
        vesa_printf(wind.composition_buffer, vbe_info->width-strlen("00:00:00 00/00/00")*8 - 16, 0 ,  theme->os.text, "%s%d:%s%d:%s%d %s%d/%s%d/%d", TIME_PREFIX(time.hour), time.hour, TIME_PREFIX(time.minute), time.minute, TIME_PREFIX(time.second), time.second, TIME_PREFIX(time.day), time.day, TIME_PREFIX(time.month), time.month, time.year);

        /* Memory timeline */  
        
        struct mem_info minfo;
        get_mem_info(&minfo);
        char current_memory_usage = __calculate_relative_difference(minfo.kernel.used, minfo.kernel.total);
        gfx_timeline_update(&memory_timeline, current_memory_usage);

        struct net_info ninfo;
        net_get_info(&ninfo);

        int new_diff = ninfo.recvd - net_recv_last;
        int new_net_recv = __calculate_relative_difference(new_diff, net_recv_last_diff);
        int interpolated_recv = EASE(net_recv, new_net_recv, 0.3f);
        net_recv = new_net_recv;

        net_recv_last_diff = new_diff;
        net_recv_last = ninfo.recvd;
        gfx_timeline_update(&net_recv_timeline, interpolated_recv);

        new_diff = ninfo.sent - net_send_last;
        int new_net_send = __calculate_relative_difference(new_diff, net_send_last_diff);
        
        int interpolated_send = EASE(net_send, new_net_send, 0.3f);
        net_send = new_net_send;
        
        //dbgprintf("new send: %d\n", interpolated_send);
        net_send_last_diff = new_diff;
        net_send_last = ninfo.sent;

        gfx_timeline_update(&net_send_timeline, interpolated_send);

        gfx_timeline_draw(&memory_timeline);
        gfx_timeline_draw(&net_recv_timeline);
        gfx_timeline_draw(&net_send_timeline);

        if(window_changed && !__is_fullscreen){

            /* Interrupts */
            for (int i = 0; i < 12; i++){
                vesa_fillrect(wind.composition_buffer, 8, vbe_info->height-16-(12*8) + 8+(i*8), strlen("Int 4: 1000000")*8, 8, theme->os.background);
                vesa_printf(wind.composition_buffer, 8, vbe_info->height-16-(12*8) + 8+(i*8), theme->os.foreground, "Int %d: %d", i, interrupt_get_count(i));

                vesa_fillrect(wind.composition_buffer, 8 +strlen("Int 4: 1000000")*8 ,vbe_info->height-16-(12*8) + 8+(i*8), strlen("Int 4: 1000000")*8, 8, theme->os.background);
                vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8, vbe_info->height-16-(12*8) +8+(i*8), theme->os.foreground, "Int %d: %d", i+12, interrupt_get_count(i+12));

                vesa_fillrect(wind.composition_buffer, 8 +strlen("Int 4: 1000000")*8*2 ,vbe_info->height-16-(12*8) + 8+(i*8), strlen("Int 4: 1000000")*8, 8, theme->os.background);
                vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*2, vbe_info->height-16-(12*8) +8+(i*8), theme->os.foreground, "Int %d: %d", i+24, interrupt_get_count(i+24));

                vesa_fillrect(wind.composition_buffer, 8 +strlen("Int 4: 1000000")*8*3 ,vbe_info->height-16-(12*8) + 8+(i*8), strlen("Int 4: 1000000")*8, 8, theme->os.background);
                vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*3, vbe_info->height-16-(12*8) +8+(i*8), theme->os.foreground, "Int %d: %d", i+36, interrupt_get_count(i+36));
            }

            /* Memory */
            vesa_fillrect(wind.composition_buffer, 8, vbe_info->height-16-(15*8) + 8, strlen("kmem: 10000000/10000000")*8, 8, theme->os.background);
            vesa_printf(wind.composition_buffer, 8, vbe_info->height-16-(15*8) + 8, theme->os.foreground, "kmem: %d/%d", minfo.kernel.used, minfo.kernel.total);
            vesa_fillrect(wind.composition_buffer, 8, vbe_info->height-16-(14*8) + 8, strlen("pmem: 10000000/10000000")*8, 8, theme->os.background);
            vesa_printf(wind.composition_buffer, 8,vbe_info->height-16-(14*8) + 8, theme->os.foreground, "pmem: %d/%d", minfo.permanent.used, minfo.permanent.total);

            /* Processes */
            int ret;
            int line = 1;
            vesa_fillrect(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*4, vbe_info->height-16-(11*8), strlen("kmem: 100000/100000")*8, 8*8, theme->os.background);
            vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*4, vbe_info->height-16-(11*8), theme->os.foreground,"   PID  STACK");
            vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*4, vbe_info->height-16-(11*8), theme->os.foreground,"   __________");
            for (int i = 0; i < MAX_NUM_OF_PCBS; i++){
                struct pcb_info info;
                struct pcb* pcb;
                ret = pcb_get_info(i, &info);
                if(ret < 0) continue;
                pcb = pcb_get_by_pid(i);
                vesa_printf(wind.composition_buffer, 8+strlen("Int 4: 1000000")*8*4 + 8, vbe_info->height-16-(11*8) + ((line++)*8), theme->os.foreground,"   %d   0x%s%x  %d/%d", info.pid, info.is_process ? "" : "00", info.stack, pcb->preempts, timer_get_tick());
            }

            /* NIC */
            vesa_fillrect(wind.composition_buffer, 8, vbe_info->height-16-(17*8) + 8, strlen(current_netdev.name)*8, 8, theme->os.background);
            vesa_printf(wind.composition_buffer, 8, vbe_info->height-16-(17*8) + 8, theme->os.foreground, "NetDev: %s (rx %d - tx %d)", is_netdev_attached() ? current_netdev.name : "No NIC found.", ninfo.recvd, ninfo.sent);

            /* Diskdev */
            vesa_fillrect(wind.composition_buffer, 8, vbe_info->height-16-(18*8) + 8, strlen(disk_name())*8, 8, theme->os.background);
            vesa_printf(wind.composition_buffer, 8, vbe_info->height-16-(18*8) + 8, theme->os.foreground, "Disk: %s", disk_attached() ? disk_name() : "No disk found.");

            //LEAVE_CRITICAL();
            if(__is_fullscreen){
            } else {
                gfx_recursive_draw(wind.order);
            }
         }

        kernel_yield();

        //if(mouse_changed || window_changed)
        ENTER_CRITICAL();
        memcpy((uint8_t*)vbe_info->framebuffer, wind.composition_buffer, buffer_size-1);
        LEAVE_CRITICAL();
        /* Copy buffer over to framebuffer. */

        if(mouse_changed){
            gfx_mouse_event(m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);
    }
}

void gfx_init()
{
    mutex_init(&wind.order_lock);
}
