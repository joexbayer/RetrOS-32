#include <utils/Graphics.hpp>
#include <libc.h>
#include <colors.h>

class Template : public Window {
public:
    Template(int width, int height) : Window(200, 200, "Template", 1) {
    }
private:
    int width;
    int height;
};

int main()
{
    Template t(200, 200);

    struct gfx_event e;
    while (1){
        gfx_get_event(&e, GFX_EVENT_BLOCKING); /* alt: GFX_EVENT_NONBLOCKING */
        switch (e.event)
        {
        case GFX_EVENT_RESOLUTION:
            /* update screensize */
            break;
        case GFX_EVENT_EXIT:
            /* exit */
            return 0;
        case GFX_EVENT_KEYBOARD:
            /* keyboard event in e.data */
            break;
        case GFX_EVENT_MOUSE:
            /* mouse event in e.data and e.data2 */
            break;
        }

    }
    return 0;
}