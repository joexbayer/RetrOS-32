#ifndef COLORPICKER_HPP
#define COLORPICKER_HPP

#include <utils/Graphics.hpp>
#include <utils/StdLib.hpp>
#include <utils/Widgets.hpp>
#include <utils/Thread.hpp>
#include <utils/Function.hpp>
#include <libc.h>
#include <colors.h>
#include <lib/printf.h>



volatile int selected = 0;


class ColorPicker : public Window {
public:
    
    ColorPicker() : Window(160, 160, "ColorPicker", 0) {
        /* Draw colors */
        for (int i = 0; i < 256; ++i) {
            int x = (i % 16) * 10;
            int y = (i / 16) * 10;
            drawRect(x, y, 10, 10, i);
        }

        /* Draw color preview */
        drawRect(0, 150, 160, 10, 0);
        gfx_draw_format_text(0, 150, COLOR_WHITE, "(0x%x)", color);

        /* Draw text under preview */
        gfx_draw_format_text(0, 160, COLOR_WHITE, "Press enter to select color");
    }

    int submit() {
        return 0;
    }

    int run() {
        while (true) {
            struct gfx_event event;
            int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
			if(ret == -1) continue;

             switch (event.event){
                case GFX_EVENT_RESOLUTION:
                    /* update screensize */
                    break;
                case GFX_EVENT_EXIT:
                    exit();
                    /* exit */
                    return -1;
                case GFX_EVENT_KEYBOARD:
                    if(event.data == '\n') {
                        return color;
                    }
                    /* keyboard event in e.data */
                    break;
                case GFX_EVENT_MOUSE:
                    color = ((event.data2 / 10) * 16) + (event.data / 10);
                    drawRect(0, 150, 160, 10, color);
                    gfx_draw_format_text(0, 150, COLOR_WHITE, "(0x%x)", color);
                    break;
                }
        }

        return 0;
    }
    
private:
    int width;
    int height;

    int color;
};

void __color_picker_entry(void* args) {
    int color = 0;
    volatile int* sig = (signal_t*)args;

    ColorPicker* colorPicker = new ColorPicker();
    color = colorPicker->run(); 

    selected = color;

    printf("Setting 0x%x to %d\n", sig, selected);
}

static int __check_ptr(volatile signal_t* ptr) {
    return selected == 0;
}

int color_picker_entry() {
    volatile int sig = 0;
    volatile int* sig_ptr = &sig;
    

    Thread* colorPickerThread = new Thread(__color_picker_entry, 0);
    colorPickerThread->start((void*)&sig);

    printf("Waiting for 0x%x\n", sig_ptr);
    while (__check_ptr(sig_ptr)) {
        //printf("Waiting for 0x%x -  %d\n", sig_ptr, selected);
        yield();
    }

    return selected;
}

#endif // COLORPICKER_HPP