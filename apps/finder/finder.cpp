#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>
#include "../utils/StringHelper.hpp"
#include <fs/ext.h>
#include <fs/directory.h>

#define WIDTH 400
#define HEIGHT 300

/* todo add to file */

class Finder : public Window
{
public:
    Finder() : Window(WIDTH, HEIGHT, "Finder", 0){
        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);
        setHeader(cwd.getData());

        for(int i = 0; i < 5; i++){
            icon[i] = (const unsigned char*)malloc(32*32);
        }
        int fd = open("afolder.ico", FS_FLAG_READ);
        read(fd, (void*)icon[0], 32*32);
        fclose(fd);

        fd = open("afile.ico", FS_FLAG_READ);
        read(fd, (void*)icon[1], 32*32);
        fclose(fd);

        drawIcon(8, 8, icon[0]);
        drawIcon(40, 8, icon[1]);   
    }

    void drawIcon(int x, int y, const unsigned char* icon){
        for (int i = 0; i < 32*32; i++){
            if(icon[i] == 0xff) continue;
            drawPixel(x + i%32, y + i/32, icon[i]);
        }
    }

    ~Finder();

    int changeDirectory(const char* path){
        cwd.concat(path);
        setHeader(cwd.getData());
    }

    int showFiles(){
        int ret;
        struct directory_entry entry;
        int i = 0;

        if(m_fd != 0) fclose(m_fd);

        m_fd = open(cwd.getData(), FS_FLAG_READ);
        if(m_fd <= 0){
            m_fd = 0;
            return -1;
        }

        while (1){
            ret = read(m_fd, &entry, sizeof(struct directory_entry));
            if(ret <= 0) break;

            drawText(0, (i++)*8, entry.name, COLOR_BLACK);
        }

        fclose(m_fd);
        m_fd = 0;

        return 0;
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
                if(m_fd != 0 ) fclose(m_fd);
                exit();
                break;
            default:
                break;
            }
        }
    }

private:
    int m_fd = 0;

    String cwd = "/";
    const unsigned char* icon[5];
};


int main(void)
{
    Finder finder;
    finder.Run();

    return 0;
}