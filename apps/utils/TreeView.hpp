/**
 * @file TreeView.hpp
 * @author Joe Bayer (joexbayer)
 * @brief A tree view widget
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __TREEVIEW_HPP__
#define __TREEVIEW_HPP__

#include <utils/StringHelper.hpp>
#include <utils/Graphics.hpp>
#include <utils/cppUtils.hpp>
#include <lib/syscall.h>

#include <fs/fat16.h>
#include <fs/fs.h>

static const unsigned char __folder_icon[16*16] = {
    0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
    ,0x92,0xfa,0x92,0x92,0x92,0x92,0x92,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
    ,0xfa,0x92,0xdb,0xfc,0xdb,0xfc,0xdb,0x92,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
    ,0x92,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0xfa
    ,0x92,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x90,0x00
    ,0x92,0xff,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0x90,0x00
    ,0x92,0xff,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0x90,0x00
    ,0x92,0xff,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0x90,0x00
    ,0x92,0xff,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0x90,0x00
    ,0x92,0xff,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0x90,0x00
    ,0x92,0xff,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0x90,0x00
    ,0x92,0xff,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0x90,0x00
    ,0x92,0xff,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0xdb,0xfc,0x90,0x00
    ,0x92,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x00
    ,0xfa,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfa
    ,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
};

static const unsigned char __file_icon[16*16] = {
    0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
    ,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92
    ,0x92,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0xdb,0x92,0xfa
    ,0x92,0xdb,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x92,0xfa
    ,0x92,0xdb,0x02,0x02,0x02,0x02,0x02,0x02,0xff,0x00,0xff,0x00,0xff,0x00,0x92,0xfa
    ,0x92,0xdb,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0xdb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x92,0xfa
    ,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0x92,0xfa
    ,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
    ,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa,0xfa
};

/**
 * @brief A tree view widget
 * @details This class implements a tree view widget that can be used to
 * display a file system tree. It is used by the Finder application
 * to display the file system tree.
 * 
 * @warning Assumes that the file system is FAT16 and a window is present.
 */
class TreeView {
public:
    TreeView(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {
        /* Initialize any other required attributes */
        path = new String("/");
    }

    ~TreeView() {
        /* Free any allocated memory */
        delete path;
    }

    void drawTree(Window* window) {
        /* Draw the tree view background */
        window->drawContouredRect(x, y, width, height);
        window->drawContouredBox(8, 12, width - 16, height-20, COLOR_WHITE);

        window->drawFormatText(2, 2, COLOR_BLACK, "Tree View");

        int entries = drawTreeRecursive(1, path, 0);
    }

    void update(const char* newPath) {
        /* Update the tree view with the new path */
        path->concat(newPath);
        /* Load files or directories as required */
    }

    /* ... Other methods ... */

private:
    int x, y, width, height;
    String* path;

    void drawIcon(int x, int y, const unsigned char* icon) {
        for (int i = 0; i < 16*16; i++) {
            if (icon[i] == 0xff) continue;
            gfx_draw_pixel(x + (i % 16), y + (i / 16), icon[i]);
        }
    }

    /* Logic of caching and interaction */
    struct TreeNode {
        String* name;
        int x, y, depth, entries;
        int isDirectory;
    } nodes[100];
    int nodeCount = 0;

    int insertNode(const char* name, int x, int y, int isDirectory) {
        nodes[nodeCount].name = new String(name);
        nodes[nodeCount].x = x;
        nodes[nodeCount].y = y;
        nodes[nodeCount].isDirectory = isDirectory;
        nodeCount++;
    }

    int drawTreeRecursive(int depth, String* path, int entries) {
        int fd = open(path->getData(), FS_FILE_FLAG_READ);
        if (fd <= -1) return -1;

        int origEntries = entries;
        struct fat16_directory_entry entry;
        int lastY = entries * 12;

        while (1) {
            int ret = read(fd, &entry, sizeof(struct fat16_directory_entry));
            if (ret <= 0) {
                break;
            }

            /* parse name */
            char name[13];
            memset(name, 0, 13);
            memcpy(name, entry.filename, 8);
            //memcpy(name + 8, entry.extension, 3);

            if (
                entry.attributes & FAT16_FLAG_VOLUME_LABEL ||
                entry.filename[0] == 0xe5 ||
                entry.filename[0] == 0x00 ||
                entry.filename[0] == '.'
            ) continue;

            entries++;
            int textY = entries * 14;

            /* Draw a line from the parent to this directory */
            if (depth > 1 && !(entry.attributes & FAT16_FLAG_SUBDIRECTORY)) {
                gfx_draw_line( depth * 12 - 9, lastY+8,   depth * 12 - 9,  textY+8, COLOR_BLACK);
                gfx_draw_line( depth * 12 - 9, textY+4 ,   depth * 12-2,  textY+4, COLOR_BLACK);
            }

            if (entry.attributes & FAT16_FLAG_SUBDIRECTORY) {
                drawIcon(depth * 12, textY-4, __folder_icon);
            } else {
                drawIcon(depth * 12, textY-4, __file_icon);
            }

            gfx_draw_format_text((depth * 12)+18, textY, COLOR_BLACK, "%s", name);
            if (entry.attributes & FAT16_FLAG_SUBDIRECTORY) {
                String* path2 = new String(path->getData());
                path2->concat(name);

                //insertNode(path2->getData(), depth * 12, textY, 1);
                int ret = drawTreeRecursive(depth + 1, path2, entries);
                if (ret > 0) entries += ret;

                delete path2;
            }

            lastY = textY; 
        }

        fclose(fd);
        return entries - origEntries;
    }
};


#endif /* !__ */