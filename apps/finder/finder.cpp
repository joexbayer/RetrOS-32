#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>
#include "../utils/StringHelper.hpp"
#include <fs/ext.h>
#include <fs/directory.h>

#define WIDTH 400
#define HEIGHT 300


class Finder : public Window
{
public:
    Finder() : Window(WIDTH, HEIGHT, "Finder", 0){
        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);
        setHeader(cwd.getData());
    }

    ~Finder();

    int changeDirectory(const char* path){
        cwd.concat(path);
        setHeader(cwd.getData());
    }

    int showFiles(){
        if(m_fd != -1) fclose(m_fd);
        m_fd = open(cwd.getData(), FS_FLAG_READ);
        
        if(m_fd == -1){
            return -1;
        }

        struct directory_entry entry;
        int ret;
        int i = 0;
        while (1){
            ret = read(m_fd, &entry, sizeof(struct directory_entry));
            if(ret <= 0) break;

            drawText(0, (i++)*8, entry.name, COLOR_BLACK);
        }

        fclose(m_fd);
    }

    void Run(){
        /* event loop */
        showFiles();

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
    int m_fd;

    String cwd = "/";
};


int main(void)
{
    Finder finder;
    finder.Run();

    while (1)
    {
        /* code */
    }
    

    return 0;
}