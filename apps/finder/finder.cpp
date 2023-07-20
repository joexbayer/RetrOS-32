#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>
#include "../utils/StringHelper.hpp"
#include <fs/ext.h>
#include <fs/directory.h>

#define WIDTH 400
#define HEIGHT 300
#define ICON_SIZE 32


/* todo add to file */
class Finder : public Window
{
public:
    Finder() : Window(WIDTH, HEIGHT, "Finder", 0){
        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);

        for(int i = 0; i < 5; i++){
            icon[i] = (unsigned char*)malloc(ICON_SIZE*ICON_SIZE);
        }

        loadIcon("folder.ico", 0);
        loadIcon("text.ico", 1);

        m_path = new String("/");
    }

    void loadIcon(const char* path, int index){
        int fd = open(path, FS_FLAG_READ);
        read(fd, (void*)icon[index], ICON_SIZE*ICON_SIZE);
        fclose(fd);
    }

    void drawIcon(int x, int y, const unsigned char* icon){
        for (int i = 0; i < ICON_SIZE*ICON_SIZE; i++){
            if(icon[i] == 0xff) continue;
            drawPixel(x + i%ICON_SIZE, y + i/ICON_SIZE, icon[i]);
        }
    }

    ~Finder();

    int changeDirectory(const char* path){
        
    }

    int showFiles(){
        int ret;
        struct directory_entry entry;

        if(m_fd != 0) fclose(m_fd);

        gfx_draw_format_text(4, 4, COLOR_BLACK, "Path: %s", m_path->getData());

        m_fd = open(m_path->getData(), FS_FLAG_READ);
        if(m_fd <= -1) return -1;

        int j = 0, i = 0;
        while (1) {
            int ret = read(m_fd, &entry, sizeof(struct directory_entry));
            if (ret <= 0) {
                break;
            }

            /* draw icon based on type */
            int xOffset = 4 + j * WIDTH / 2;
            int yOffset = 4 + i * ICON_SIZE;

            if (entry.flags & FS_DIR_FLAG_DIRECTORY) {
                drawIcon(xOffset, yOffset, icon[0]);
            } else {
                drawIcon(xOffset, yOffset, icon[1]);
            }

            drawText(40 + j * WIDTH / 2, 16 + (i++) * ICON_SIZE, entry.name, COLOR_BLACK);

            if (i * ICON_SIZE > HEIGHT - ICON_SIZE) {
                i = 0;
                ++j;
            }
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
    unsigned char* icon[5];

    String* m_path;
};


int main(void)
{
    Finder finder;
    finder.Run();

    return 0;
}