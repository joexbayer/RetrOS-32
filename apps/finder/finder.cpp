#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>
#include <../utils/StringHelper.hpp>

#define WIDTH 800
#define HEIGHT 600

class Finder : public Window
{
public:
    Finder() : Window(WIDTH, HEIGHT, "Finder", 0){
        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);
        setHeader(cwd.getData());
    }

    ~Finder();

    int changeDirectory(){

    }

    int showFiles(){

    }

    void Run(){
        /* event loop */
        struct gfx_event event;
        while (1){
            int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
            if(ret == -1) continue;

            switch (event.event){
            case GFX_EVENT_MOUSE:
                break;
            case GFX_EVENT_EXIT:
                exit();
                break;
            default:
                break;
            }
        }
    }

private:

    String cwd = "/";
};


int main(void)
{

    return 0;
}