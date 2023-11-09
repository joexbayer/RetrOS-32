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
#include <kutils.h>
#include <math.h>
#include <net/net.h>
#include <arch/interrupts.h>

#include <gfx/windowserver.h>

#include <fs/fs.h>

#include <windowmanager.h>

#include <fs/ext.h>

/* prototypes */
void __kthread_entry gfx_compositor_main();

static struct window_servers {
    uint8_t sleep_time;

    /* frame buffers */
    ubyte_t* composition_buffer;
    ubyte_t* background;

    int buffer_size;

    struct windowmanager wm;

    byte_t flags;
} wind = {
    .sleep_time = 2,
    .flags = WINDOW_SERVER_UNINITIALIZED,
    .background = NULL
};

struct windowserver* ws;

static inline int gfx_check_changes(struct window* w)
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
     while (ws == NULL || ws->_wm->state != WM_INITIALIZED);
     ws->ops->remove(ws, w);
}

/**
 * @brief Adds new window in the "wind.order" list.
 * 
 * @param w 
 */
void gfx_composition_add_window(struct window* w)
{
    while (ws == NULL || ws->_wm->state != WM_INITIALIZED);

     ws->ops->add(ws, w);
}

static int __is_fullscreen = 0;
static void* inner_window_save;
static void gfx_set_fullscreen(struct window* w)
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

static void gfx_unset_fullscreen(struct window* w)
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

int gfx_set_background_color(color_t color)
{
    if(wind.flags & WINDOW_SERVER_UNINITIALIZED){
        dbgprintf("[WSERVER] Cannot set background color before initialization.\n");
        return -1;
    }

    if(wind.background == NULL){
        dbgprintf("[WSERVER] Background buffer is NULL.\n");
        return -2;
    }

    memset(wind.background, color, wind.buffer_size);
    return ERROR_OK;
}

int gfx_decode_background_image(const char* file)
{
    if(wind.flags & WINDOW_SERVER_UNINITIALIZED){
        dbgprintf("[WSERVER] Cannot decode background image before initialization.\n");
        return -1;
    }

    byte_t* temp = malloc(10000);
    inode_t inode = fs_open(file, FS_FILE_FLAG_READ);
    if(inode < 0){
        gfx_set_background_color(3);
        dbgprintf("[WSERVER] Could not open background file: %d.\n", inode);
        free(temp);
        return -1;
    }

    int ret = fs_read(inode, temp, 10000);
    if(ret <= 0){
        gfx_set_background_color(3);
        dbgprintf("[WSERVER] Could not read background file: %d.\n", inode);
        free(temp);
        return -1;
    }
    fs_close(inode);

    byte_t* temp_window = (byte_t*) malloc(320*240);
    if(temp_window == NULL){
        gfx_set_background_color(3);
        dbgprintf("[WSERVER] Could not allocate memory for background image.\n");
        free(temp);
        return -1;
    }

    int out;
    dbgprintf("[WSERVER] Decoding %d bytes.\n", ret);
    run_length_decode(temp, ret, temp_window, &out);

    dbgprintf("[WSERVER] Decoded %d bytes.\n", out);

    int originalWidth = 320;   // Original image width
    int originalHeight = 240;  // Original image height
    int targetWidth = vbe_info->width;     // Target screen width
    int targetHeight = vbe_info->height;    // Target screen height

    /* upscale image */
    float scaleX = (float)targetWidth / (float)originalWidth;
    float scaleY = (float)targetHeight / (float)originalHeight;

    dbgprintf("[WSERVER] Scaling image from %dx%d to %dx%d.\n", originalWidth, originalHeight, targetWidth, targetHeight);
    
    for (int i = 0; i < originalWidth; i++){
        for (int j = 0; j < originalHeight; j++){
            // Calculate the position on the screen based on the scaling factors
            int screenX = (int)(i * scaleX);
            int screenY = (int)(j * scaleY);

            // Retrieve the pixel value from the original image
            unsigned char pixelValue = temp_window[j * originalWidth + i];

            // Set the pixel value on the screen at the calculated position
            for (int x = 0; x < scaleX; x++){
                for (int y = 0; y < scaleY; y++){
                    vesa_put_pixel(wind.background, screenX + x, screenY + y, pixelValue);
                }
            }
        }
    }
    free(temp);
    free(temp_window);

    return ERROR_OK;
}

void __kthread_entry gfx_compositor_main()
{
    ws = ws_new();
    if(ws == NULL){
        dbgprintf("[WSERVER] Could not allocate memory for window server.\n");
        return;
    }

    int taskbar_pid = start("taskbar");
    ws->taskbar = pcb_get_by_pid(taskbar_pid);
    ws->ops->set_background(ws, 3);


    while(1){
        ws->ops->draw(ws);
    }


    if(wind.flags & WINDOW_SERVER_UNINITIALIZED){

        wind.buffer_size = vbe_info->width*vbe_info->height*(vbe_info->bpp/8)+1;

        wind.composition_buffer = (uint8_t*) palloc(wind.buffer_size);
        if(wind.composition_buffer == NULL){
            warningf("Could not allocate memory for composition buffer.\n");
            return;
        }

        wind.background = (uint8_t*) kalloc(wind.buffer_size+1);
        if(wind.background == NULL){
            warningf("Could not allocate memory for background buffer.\n");
            return;
        }

        PANIC_ON_ERR(init_windowmanager(&wind.wm, 0));
        /* TODO: fix this */
        wind.wm.composition_buffer = wind.composition_buffer;
        

        wind.flags &= ~WINDOW_SERVER_UNINITIALIZED;
        gfx_decode_background_image("/circles.img");

        //memset(background, 3, wind.buffer_size);
        dbgprintf("[WSERVER] %d bytes allocated for composition buffer.\n", wind.buffer_size);
    }

    dbgprintf("[WSERVER] Window Server initialized.\n");
    struct mouse m;

    int workspace = 0;
    struct pcb* taskbar = pcb_get_by_pid(taskbar_pid);

    /* Main composition loop */
    while(1){
        struct gfx_theme* theme = kernel_gfx_current_theme();
        
        /**
         * Problem with interrupts from mouse?
         * Manages to corrupt some register or variables.
         * 
         * Should also NOT redraw entire screen, only things that change.
         */
        int mouse_changed = mouse_get_event(&m);
        int window_changed = gfx_check_changes(wind.wm.windows);

        struct time time;
        get_current_time(&time);
        
        unsigned char key = kb_get_char();
        if(key != 0){
            if(key == F4){

                /* remove task bar window from list */
                wind.wm.ops->remove(&wind.wm, taskbar->gfx_window);

                workspace = (workspace+1)%WM_MAX_WORKSPACES;
                wind.wm.ops->workspace(&wind.wm, workspace);
                dbgprintf("Switched to workspace %d.\n", workspace);
                window_changed = 1;

                wind.wm.ops->add(&wind.wm, taskbar->gfx_window);

            } else if(key == F10){

                __is_fullscreen ? gfx_unset_fullscreen(wind.wm.windows) : gfx_set_fullscreen(wind.wm.windows);

                struct gfx_event e = {
                    .data = wind.wm.windows->inner_width,
                    .data2 = wind.wm.windows->inner_height,
                    .event = GFX_EVENT_RESOLUTION
                };
                gfx_push_event(wind.wm.windows, &e);
            } else {
                struct gfx_event e = {
                    .data = key,
                    .event = GFX_EVENT_KEYBOARD
                };
                gfx_push_event(wind.wm.windows, &e);
            }
        }

        /* This code runs only if a window has changed */
        if(window_changed && !__is_fullscreen){
            memcpy(wind.composition_buffer, wind.background, wind.buffer_size);
            int len = strlen("RetrOS-32 Development Build");
            vesa_printf(wind.composition_buffer, (vbe_info->width/2) - (len/2)*8, vbe_info->height-8, COLOR_BLACK, "%s", "RetrOS-32 Development Build");

            if(1){
                /* performance */
                int usage;
                int ret;
                vesa_printf(wind.composition_buffer, 4, 300, COLOR_WHITE, "%s", "PID   Usage   Type      State   Name");
                for (int i = 1; i < MAX_NUM_OF_PCBS; i++){
                    struct pcb_info info;
                    ret = pcb_get_info(i, &info);
                    if(ret < 0) continue;
                    usage = (int)(info.usage*100);
                    vesa_printf(wind.composition_buffer, 4, 300+((i+1)*8), 0, "%d  %d/%d (%d) %s", info.pid, (pcb_get_by_pid(i)->preempts), pcb_total_usage(), timer_get_tick(), info.name);
                }

                /* workspaces window */
                vesa_printf(wind.composition_buffer, 4, 384, COLOR_WHITE, "Workspace: %d/%d (F3 to switch)", workspace, WM_MAX_WORKSPACES-1);
                struct mem_info minfo;
	            get_mem_info(&minfo);

                struct unit kernel = calculate_size_unit(minfo.kernel.used);
	            struct unit kernel_total = calculate_size_unit(minfo.kernel.total);

	            struct unit permanent = calculate_size_unit(minfo.permanent.used);
	            struct unit permanent_total = calculate_size_unit(minfo.permanent.total);

                vesa_printf(wind.composition_buffer, 4, 400+8, COLOR_WHITE, "Memory:");
                vesa_printf(wind.composition_buffer, 4, 400+24, COLOR_WHITE, "Kernel: %d/%s %d/%s", kernel.size, kernel.unit,  kernel_total.size, kernel_total.unit);
                vesa_printf(wind.composition_buffer, 4, 400+32, COLOR_WHITE, "Permanent: %d/%s %d/%s", permanent.size, permanent.unit, permanent_total.size, permanent_total.unit);

            }
                
            wind.wm.ops->draw(&wind.wm, wind.wm.windows);
        }

        kernel_yield();

        ENTER_CRITICAL();
        
        /* Copy buffer over to framebuffer. */
        memcpy((uint8_t*)vbe_info->framebuffer, wind.composition_buffer, wind.buffer_size-1);

        LEAVE_CRITICAL();
        
        if(mouse_changed){
            wind.wm.ops->mouse_event(&wind.wm, m.x, m.y, m.flags);
        }
        vesa_put_icon16((uint8_t*)vbe_info->framebuffer, m.x, m.y);
    }
}