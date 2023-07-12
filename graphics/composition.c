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
#include <kthreads.h>
#include <math.h>
#include <net/net.h>
#include <arch/interrupts.h>

#include <windowmanager.h>

#include <fs/ext.h>

#include <diskdev.h>
#include <net/netdev.h>

static struct window_server {
    uint8_t sleep_time;
    uint8_t* composition_buffer;
    struct window* order;
    mutex_t lock;

    struct windowmanager wm;

} wind = {
    .sleep_time = 2
};

/* Mouse globals */
static char gfx_mouse_state = 0;
static struct mouse m;

int gfx_check_changes(struct window* w)
{
    while(w != NULL){
        if(w->changed)
            return 1;
        w = w->next;
    }
    
    return 0;
}

/**
 * @brief Removes a window from the "wind.order" list.
 * Important keep the wind.order list intact even if removing the first element.
 * @param w 
 */
void gfx_composition_remove_window(struct window* w)
{
    while (wind.wm.state != WM_INITIALIZED);
    
    wind.wm.ops->remove(&wind.wm, w);
}

/**
 * @brief Adds new window in the "wind.order" list.
 * 
 * @param w 
 */
void gfx_composition_add_window(struct window* w)
{
    while (wind.wm.state != WM_INITIALIZED);

    wind.wm.ops->add(&wind.wm, w);
}

static int __is_fullscreen = 0;
static void* inner_window_save;
void gfx_set_fullscreen(struct window* w)
{
    if(w != wind.wm.windows){
        dbgprintf("Cannot fullscreen window that isnt in focus\n");
        return;
    }

    /* store and backup original window information */
    w->inner_width = vbe_info->width;
    w->inner_height = vbe_info->height;

    inner_window_save = w->inner;
    w->inner = wind.composition_buffer;
    w->pitch = vbe_info->width;
    w->y = 0;
    w->x = 0;

    dbgprintf("%s is now in fullscreen\n", w->name);

    __is_fullscreen = 1;
}

void gfx_unset_fullscreen(struct window* w)
{
    if(w != wind.wm.windows){
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

int gfx_decode_background_image(char* background, int width, int height){

    char* temp = (char*) kalloc(5000);
    inode_t inode = ext_open("bg.bin", 0);
    if(inode == 0){
        dbgprintf("[WSERVER] Could not open background file.\n");
        return -1;
    }

    int ret = ext_read(inode, temp, 5000);
    if(ret == 0){
        dbgprintf("[WSERVER] Could not read background file.\n");
        return -1;
    }
    ext_close(inode);

    int out;
    run_length_decode(temp, ret, wind.composition_buffer, &out);
    kfree(temp);

    int originalWidth = 320;   // Original image width
    int originalHeight = 240;  // Original image height
    int targetWidth = vbe_info->width;     // Target screen width
    int targetHeight = vbe_info->height;    // Target screen height

    /* upscale image */
    float scaleX = (float)targetWidth / originalWidth;
    float scaleY = (float)targetHeight / originalHeight;

    for (int i = 0; i < originalWidth; i++){
        for (int j = 0; j < originalHeight; j++){
            // Calculate the position on the screen based on the scaling factors
            int screenX = (int)(i * scaleX);
            int screenY = (int)(j * scaleY);

            // Retrieve the pixel value from the original image
            unsigned char pixelValue = wind.composition_buffer[j * originalWidth + i];

            // Set the pixel value on the screen at the calculated position
            for (int x = 0; x < scaleX; x++){
                for (int y = 0; y < scaleY; y++){
                    vesa_put_pixel(background, screenX + x, screenY + y, pixelValue);
                }
            }
        }
    }


    return ERROR_OK;
}

static char* background;
#define TIME_PREFIX(unit) unit < 10 ? "0" : ""
void gfx_compositor_main()
{
    int buffer_size = vbe_info->width*vbe_info->height*(vbe_info->bpp/8)+1;

    dbgprintf("[WSERVER] %d bytes allocated for composition buffer.\n", buffer_size);
    wind.composition_buffer = (uint8_t*) palloc(buffer_size);
    background = (uint8_t*) kalloc(buffer_size);

    PANIC_ON_ERR(init_windowmanager(&wind.wm, 0));
    wind.wm.composition_buffer = wind.composition_buffer;

    gfx_decode_background_image(background, vbe_info->width, vbe_info->height);

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
        int mouse_changed = mouse_get_event(&m);
        
        int window_changed = gfx_check_changes(wind.wm.windows);

        struct time time;
        get_current_time(&time);
        
        unsigned char key = kb_get_char();
        if(key != 0){
            if(key == F10){
                if(!__is_fullscreen) {
                    gfx_set_fullscreen(wind.wm.windows);
                } else {
                    gfx_unset_fullscreen(wind.wm.windows);
                }
                struct gfx_event e = {
                    .data = wind.wm.windows->inner_width,
                    .data2 = wind.wm.windows->inner_height,
                    .event = GFX_EVENT_RESOLUTION
                };
                gfx_push_event(wind.wm.windows, &e);
            } if (key == F5) {
                start("shell");
            } else {
                struct gfx_event e = {
                    .data = key,
                    .event = GFX_EVENT_KEYBOARD
                };
                gfx_push_event(wind.wm.windows, &e);
            }
        }

        
        if(window_changed && !__is_fullscreen){
            
            //memset(wind.composition_buffer, theme->os.background/*41*/, buffer_size);
            memcpy(wind.composition_buffer, background, buffer_size);

            // vesa_fillrect(wind.composition_buffer, 0, 0, vbe_info->width, 16, theme->window.background);
            // /* black line under rect */
            // vesa_fillrect(wind.composition_buffer, 0, 16, vbe_info->width, 1, 0);

            // vesa_printf(wind.composition_buffer, 4, 4, 0, "%s", "HOME");
            // /* open text */
            // vesa_printf(wind.composition_buffer, 60, 4, 0, "%s", "Open");

            // vesa_printf(wind.composition_buffer, 120, 4, 0, "%s", "File");

                
            PANIC_ON_ERR(wind.wm.ops->draw(&wind.wm, wind.wm.windows));


            /* performance */
            int usage;
            int ret;
            vesa_printf(wind.composition_buffer, 4, 16, 0, "%s", "PID   Usage   Type      State   Name");
            for (int i = 1; i < MAX_NUM_OF_PCBS; i++){
                struct pcb_info info;
                ret = pcb_get_info(i, &info);
                if(ret < 0) continue;
                usage = (int)(info.usage*100);
                vesa_printf(wind.composition_buffer, 4, 16+((i+1)*8), 0, "%d  %d/%d (%d) %s", info.pid, (pcb_get_by_pid(i)->preempts), pcb_total_usage(), timer_get_tick(), info.name);
            }
        }
    

        kernel_yield();

        //if(mouse_changed || window_changed)
        ENTER_CRITICAL();
        memcpy((uint8_t*)vbe_info->framebuffer, wind.composition_buffer, buffer_size-1);
        LEAVE_CRITICAL();
        /* Copy buffer over to framebuffer. */
        
        if(mouse_changed){
            wind.wm.ops->mouse_event(&wind.wm, m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);
    }
}

void gfx_init()
{
    mutex_init(&wind.lock);
}
