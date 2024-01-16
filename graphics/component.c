#include <gfx/component.h>
#include <vbe.h>

int gfx_point_in_rectangle(int x1, int y1, int x2, int y2, int x, int y)
{
    return (x >= x1 && x <= x2) && (y >= y1 && y <= y2);
}

void gfx_draw_component(uint8_t* buffer, struct gfx_component* c)
{
    switch (c->type)
    {
    case GFX_RECTANGLE:
        vesa_fillrect(buffer, c->x, c->y, c->x + c->width, c->y + c->height, c->color);
        break;
    
    default:
        break;
    }
}

int gfx_input_event(struct gfx_input_manager* input, struct gfx_event* event)
{
    dbgprintf("input event\n");
    for(int i = 0; i < input->input_count; i++){
        struct gfx_input* in = &input->inputs[i];
        if(in->placeholder[0] == 0) continue;

        dbgprintf("Checking input %d\n", i);

        if(event->event == GFX_EVENT_MOUSE){
            if(gfx_point_in_rectangle(in->x, in->y, in->x + in->width, in->y + in->height, event->data, event->data2)){
                in->clicked = 1;
            }else{
                in->clicked = 0;
            }
        }else if(event->event == GFX_EVENT_KEYBOARD){
            if(in->clicked){
                if(event->data == 0x08){
                    if(in->buffer_size > 0){
                        in->buffer_size--;
                        in->buffer[in->buffer_size] = 0;
                    }
                }else{
                    if(in->buffer_size < 256){
                        in->buffer[in->buffer_size] = event->data;
                        in->buffer_size++;
                    }
                }
            }
        }
    }

    return 0;
}

int gfx_input_draw(struct window* w, struct gfx_input_manager* input)
{
    for(int i = 0; i < input->input_count; i++){
        struct gfx_input* in = &input->inputs[i];
        if(in->placeholder[0] == 0) continue;

        w->draw->box(w, in->x, in->y, in->width, in->height, 29);
        if(in->clicked){
            w->draw->box(w, in->x, in->y, in->width, in->height, 30);
        }

        if(in->buffer_size > 0){
            w->draw->text(w, in->x+2, in->y+2,  in->buffer, 0x17);
        } else {
            w->draw->text(w, in->x+2, in->y+2,  in->placeholder, 0x17);
        }
    }

    return 0;
}

int gfx_input_manager_add(struct gfx_input_manager* man, struct gfx_input input)
{
    /* add new input to manager */
    
    if(man->input_count < 8){
        man->inputs[man->input_count] = input;
        man->input_count++;
        return 0; 
    }

    dbgprintf("Could not add input\n");

    return -1;
}