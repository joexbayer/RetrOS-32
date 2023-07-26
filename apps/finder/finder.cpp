#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>
#include "../utils/StringHelper.hpp"
#include <fs/ext.h>
#include <fs/directory.h>
#include <lib/printf.h>

#define WIDTH 400
#define HEIGHT 300
#define ICON_SIZE 32

class File {
public:
    File(const char* name, int flags, int x, int y, int w, int h) {
        this->name = new String(name);
        this->flags = flags;
        this->x = x;
        this->y = y;
        this->w = w;
        this->h = h;
    }

    ~File() {
        name->~String();
        delete name;
    }

    const char* getName() const {
        return name->getData();
    }

    int flags;
private:
    String* name;
    int x,y,w,h;
};

class FileCache {
public:
    FileCache() {
        m_size = 0;
    }

    ~FileCache() {
        clear();
    }

    int getSize() const {
        return m_size;
    }

    File* getByIndex(int index) const {
        return m_files[index];
    }

    File* getFile(const char* path) {
        for(int i = 0; i < m_size; i++){
            if(String::strcmp(m_files[i]->getName(), path) == 0){
                return m_files[i];
            }
        }
        return 0;
    }

    void addFile(File* file) {
        m_files[m_size++] = file;
    }

    void clear(){
        for(int i = 0; i < m_size; i++){
            File* file = (File*) m_files[i];
            file->~File();
            delete file;
        }
        m_size = 0;
    }

private:
    File* m_files[100];
    int m_size = 0;
};

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

        path = new String("/");
        m_cache = new FileCache();

        setHeader(path->getData());
    }

    ~Finder(){
        for(int i = 0; i < 5; i++){
            free(icon[i]);
        }
        delete path;
        delete m_cache;
    }

    void loadIcon(char* path, int index){
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

    int changeDirectory(const char* strpath){

    }

    int loadFiles(){
        struct directory_entry entry;


        if(m_fd != 0) fclose(m_fd);

        m_cache->clear();

        m_fd = open(path->getData(), FS_FLAG_READ);
        if(m_fd <= -1) return -1;

        while (1) {
            int ret = read(m_fd, &entry, sizeof(struct directory_entry));
            if (ret <= 0) {
                break;
            }

            File* file = new File(entry.name, entry.flags, 0, 0, 0, 0);
            m_cache->addFile(file);
        }

        fclose(m_fd);
        m_fd = 0;

        return 0;
    }

    int showFiles(int x, int y){
        if(x == 0 && y == 0 )
            drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);
        
        File* file;
        int iter = 0;
        int j = 0, i = 0;
        int size = m_cache->getSize();
        while (iter < size){
            file = m_cache->getByIndex(i);
            if(file == 0) continue;

            /* draw icon based on type */
            int xOffset = 4 + j * WIDTH / 2;
            int yOffset = 4 + i * ICON_SIZE;

            if (file->flags & FS_DIR_FLAG_DIRECTORY) {
                drawIcon(xOffset, yOffset, icon[0]);
            } else {
                drawIcon(xOffset, yOffset, icon[1]);
            }

            drawText(40 + j * WIDTH / 2, 16 + (i++) * ICON_SIZE, file->getName(), COLOR_BLACK);

            /* check if x,y is inside box */
            if (x > xOffset && x < xOffset + ICON_SIZE && y > yOffset && y < yOffset + ICON_SIZE) {
                if (file->flags & FS_DIR_FLAG_DIRECTORY) {
                    /* change directory */

                    path->concat(file->getName());
                    setHeader(path->getData());
                    loadFiles();
                    showFiles(0, 0);
                    printf("change directory to %s\n", path->getData());
                    return 0;
                } else {
                    /* open file */
                }
            }

            if (i * ICON_SIZE > HEIGHT - ICON_SIZE) {
                i = 0;
                ++j;
            }
            iter++;
        }

        fclose(m_fd);
        m_fd = 0;

        return 0;
    }

    void Run(){
        /* event loop */
        loadFiles();
        showFiles(0, 0);

        struct gfx_event event;
        while (1){
            int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
            if(ret == -1) continue;

            switch (event.event){
            case GFX_EVENT_MOUSE:{
                    /* check if a mouse click is inside of a file */
                    showFiles(event.data, event.data2);
                }
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
    FileCache* m_cache;
    String* path;
};


int main(void)
{
    Finder finder;
    finder.Run();

    return 0;
}