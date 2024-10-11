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
#include <utils/Function.hpp>

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
        root = {path, 0, 0, 0, 0, 1, 1, 0, nullptr};
        root.children = new TreeNode[16];

        loadTree(&root, 0, path, 0);
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

        int entries = drawTreeRecursive(&root, 1, 1);
    }

    const char* click(int x, int y) {
        /* Check if a node was clicked */
        for (int i = 0; i < root.childrenCount; i++) {
            int y2 = (1+i) * 16;

            if (y >= y2 && y < y2 + 16) {
                if (root.children[i].isDirectory) {
                    root.children[i].isExpanded = !root.children[i].isExpanded;
                    gfx_draw_rectangle(9, 13, width - 15, height-19, COLOR_WHITE);
                    drawTreeRecursive(&root, 1, 1);
                    return nullptr;
                } else {
                    return root.children[i].name->getData();
                }
            }
            
            if(root.children[i].isDirectory) {
                if (root.children[i].isExpanded) {
                    const char* ret = __click(&root.children[i], x, y, 1+i);
                    if (ret != nullptr) return ret;
                    y2 += (root.children[i].childrenCount) * 16;
                }
            }

        }

        return nullptr;
    }

    void update(const char* newPath) {
        /* Update the tree view with the new path */
        path->concat(newPath);
    }

private:
    /* Logic of caching and interaction */
    struct TreeNode {
        String* name;
        int x, y, depth, entries;
        int isDirectory;
        int isExpanded;
        int childrenCount;
        struct TreeNode* children;
    } root;
    int nodeCount = 0;

    int x, y, width, height;
    String* path;

    const char* __click(struct TreeNode* root, int x, int y, int entries){
        for (int i = 0; i < root->childrenCount; i++) {
            int y2 = (1+i+entries) * 16;

            if (y >= y2 && y < y2 + 16) {
                if (root->children[i].isDirectory) {
                    root->children[i].isExpanded = !root->children[i].isExpanded;

                    return nullptr;
                } else {
                    return root->children[i].name->getData();
                }
            }
        }

        return nullptr;
    }

    void drawIcon(int x, int y, const unsigned char* icon) {
        for (int i = 0; i < 16*16; i++) {
            if (icon[i] == 0xff) continue;
            gfx_draw_pixel(x + (i % 16), y + (i / 16), icon[i]);
        }
    }

    int pathlen(char* path) {
        int len = 0;
        while (path[len] != 0 && path[len] != ' ' && len < 8) len++;
        return len;
    }

    struct TreeNode* insertNode(struct TreeNode* parent, const char* name, int x, int y, int isDirectory) {
        parent->children[parent->childrenCount].name = new String(name);
        parent->children[parent->childrenCount].x = x;
        parent->children[parent->childrenCount].y = y;
        parent->children[parent->childrenCount].isDirectory = isDirectory;
        if(isDirectory) {
            parent->children[parent->childrenCount].childrenCount = 0;
            parent->children[parent->childrenCount].children = new TreeNode[16];

        }
        return &parent->children[ parent->childrenCount++];
    }


    int loadTree(struct TreeNode* root, int depth, String* path, int entries){
        int fd = open(path->getData(), FS_FILE_FLAG_READ);
        if (fd <= -1) return -1;

        int origEntries = entries;
        struct fat16_directory_entry entry;

        while (1) {
            int ret = read(fd, &entry, sizeof(struct fat16_directory_entry));
            if (ret <= 0) {
                break;
            }

            /* parse name */
            char name[14] = {0};
            int len = pathlen((char*)entry.filename);
            memset(name, 0, 14);
            memcpy(name, entry.filename, len);
            if(!(entry.attributes & FAT16_FLAG_SUBDIRECTORY)) {
                name[len++] = '.';
                memcpy(name + len, entry.extension, 3);

            } else {
                name[len++] = '/';
            }

            if (
                entry.attributes & FAT16_FLAG_VOLUME_LABEL ||
                entry.filename[0] == 0xe5 ||
                entry.filename[0] == 0x00 ||
                entry.filename[0] == '.'
            ) continue;

            entries++;

            String fullPath = path->getData();
            fullPath.concat(name);
            struct TreeNode* newRoot = insertNode(root, fullPath.getData(), (depth * 12), 0, entry.attributes & FAT16_FLAG_SUBDIRECTORY);

            if (entry.attributes & FAT16_FLAG_SUBDIRECTORY) {
                String* path2 = new String(path->getData());
                path2->concat(name);

                //insertNode(path2->getData(), depth * 12, textY, 1);
                int ret = loadTree(newRoot, depth + 1, path2, entries);
                if (ret > 0) entries += ret;

                delete path2;
            }
        }

        fclose(fd);
        return entries - origEntries;
    }


    int drawTreeRecursive(struct TreeNode* root, int depth, int entries) {
        for (int i = 0; i < root->childrenCount; i++) {

            if (root->children[i].isDirectory) {
                drawIcon(depth * 16, (entries + i) * 16, __folder_icon);
            } else {
                drawIcon(depth * 16, (entries + i) * 16, __file_icon);
            }

            const char* name = root->children[i].name->getData();
            /* Only use name from last / */
            for (int j = 0; j < strlen(name)-1; j++) {
                if (name[j] == '/') {
                    name = name + j + 1;
                }
            }

            /* draw line from parent to current */
            if (depth > 1) {
                int current_x = depth * 16;
                int current_y = (entries + i) * 16;
                int parent_y = (entries - 1) * 16 + 16;
                gfx_draw_line((depth - 1) * 16 + 8, parent_y, (depth - 1) * 16 + 8, current_y, COLOR_BLACK);
                gfx_draw_line((depth - 1) * 16 + 8, current_y + 8, current_x, current_y + 8, COLOR_BLACK);
            }

            gfx_draw_format_text(depth * 16 + 18, (entries + i) * 16 + 5, COLOR_BLACK, name);
            if(root->children[i].isExpanded && root->children[i].isDirectory) {
                entries += drawTreeRecursive(&root->children[i], depth + 1, entries + i + 1);
            }
        }

        return root->childrenCount;
    }
};


#endif /* !__ */