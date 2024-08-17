/**
 * @file finder.cpp
 * @author Joe Bayer (joexbayer)
 * @brief A file manager app
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <libc.h>
#include <utils/Graphics.hpp>
#include <gfx/events.h>
#include <colors.h>
#include <utils/StringHelper.hpp>
#include <utils/StdLib.hpp>
#include <utils/Thread.hpp>
#include <fs/ext.h>
#include <fs/directory.h>
#include <utils/TreeView.hpp>

#include <fs/fat16.h>

#include <lib/printf.h>

#define WIDTH 350
#define HEIGHT 225
#define ICON_SIZE 32

#define TREE_VIEW_WIDTH 110  /* Width for the tree view panel */
#define HEADER_HEIGHT 10     /* Height for the header */

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
    String* name;
private:
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
        drawRect(0, 0, WIDTH, HEIGHT, 30);

        treeView = new TreeView(0, 0, TREE_VIEW_WIDTH, HEIGHT);

        for(int i = 0; i < 5; i++){
            icon[i] = (unsigned char*)malloc(ICON_SIZE*ICON_SIZE);
        }

        loadIcon("/ico/folder.ico", 0);
        loadIcon("/ico/text.ico", 1);
        loadIcon("/ico/cfile.ico", 2);
        loadIcon("/ico/bin.ico", 3);

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
        //struct directory_entry entry;
        struct fat16_directory_entry entry;

        if(m_fd != 0) fclose(m_fd);

        m_cache->clear();

        m_fd = open(path->getData(), FS_FLAG_READ);
        if(m_fd <= -1) return -1;

        while (1) {
            int ret = read(m_fd, &entry, sizeof(struct fat16_directory_entry));
            if (ret <= 0) {
                break;
            }

                        /* parse name */
            char name[13];
            memset(name, 0, 13);
            memcpy(name, entry.filename, 8);
            memcpy(name + 8, entry.extension, 3);

            printf("entry: %s\n", name);

            if (
                entry.filename[0] == 0xe5 || entry.filename[0] == 0x00
            )  continue;

            File* file = new File(name, entry.attributes, 0, 0, 0, 0);
            m_cache->addFile(file);
        }

        fclose(m_fd);
        m_fd = 0;

        return 0;
    }


    int showFiles(int x, int y){

        if (x == 0 && y == 0) {
            /* Draw the main background */
            drawRect(TREE_VIEW_WIDTH, HEADER_HEIGHT, WIDTH - TREE_VIEW_WIDTH, HEIGHT - HEADER_HEIGHT, COLOR_WHITE);
            
            /* Draw the header */
            drawContouredRect(TREE_VIEW_WIDTH, 0, WIDTH - TREE_VIEW_WIDTH, HEADER_HEIGHT);

            /* Header text and other UI elements, adjusted for the new layout */
            gfx_draw_format_text(TREE_VIEW_WIDTH + 2, 2, COLOR_BLACK, "< >");
            gfx_draw_format_text(TREE_VIEW_WIDTH + (WIDTH - TREE_VIEW_WIDTH)/2 - (path->getLength()*8)/2, 2, COLOR_BLACK, "%s", path->getData());
            gfx_draw_format_text(WIDTH - strlen("XXX items")*8, 2, COLOR_BLACK, "%d items", m_cache->getSize());
        }
    
        File* file;
        int iter = 0;
        int j = 0, i = 0;
        int size = m_cache->getSize();
        while (iter < size){
            file = m_cache->getByIndex(iter);
            if(file == 0) continue;

            /* draw icon based on type */
            /* Adjust positioning for icons, considering the TREE_VIEW_WIDTH and HEADER_HEIGHT */
            int xOffset = TREE_VIEW_WIDTH + 4 + j * (WIDTH - TREE_VIEW_WIDTH) / 2;
            int yOffset = (HEADER_HEIGHT + 4 + i * ICON_SIZE);

            if (file->flags & FAT16_FLAG_SUBDIRECTORY) {
                drawIcon(xOffset, yOffset, icon[0]);
            } else if (file->name->includes("TXT")) {
                drawIcon(xOffset, yOffset, icon[1]);
            } else if (file->name->includes("C  ")) {
                drawIcon(xOffset, yOffset, icon[2]);
            } else {
                drawIcon(xOffset, yOffset, icon[3]);
            }

            /* Position text under the icon */
            int textXOffset = xOffset+ICON_SIZE+4;
            int textYOffset = yOffset + ICON_SIZE/2; /* Position the text just below the icon */
            drawText(textXOffset, textYOffset, file->getName(), COLOR_BLACK);

            /* Check if x,y is inside box */
            if (x > xOffset && x < xOffset + ICON_SIZE && y > yOffset && y < yOffset + ICON_SIZE) {
                if (file->flags & FAT16_FLAG_SUBDIRECTORY) {
                    /* change directory */

                    /* ugly hack */
                    if(String::strcmp(file->getName(), "..") == 0){
                        int index = path->getLength()-1;
                        while(index > 0 && path->getData()[index] != '/') index--;

                        String path2 = path->substring(0, index);
                        delete path;
                        path = new String(path2);
                        
                        setHeader(path->getData());
                        loadFiles();
                        showFiles(0, 0);
                        printf("change directory to %s\n", path->getData());
                        return 0;
                    }

                    if(file->getName()[0] == '.'){
                        if (i * ICON_SIZE > HEIGHT - ICON_SIZE-100) {
                            i = 0;
                            ++j;
                        } else {
                            ++i;
                        }
                        iter++;
                        continue;
                    }
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

            if (i * ICON_SIZE > HEIGHT - ICON_SIZE-100) {
                i = 0;
                ++j;
            } else {
                ++i;
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

        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);

        showFiles(0, 0);

        treeView->drawTree(this);

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
    TreeView* treeView;
};

int main(void)
{
    Finder finder;
    finder.Run();

    return 0;
}