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
        int row = 0;
        int max_rows = visibleRows();
        drawTreeRecursive(&root, 1, &row, -1, max_rows);
    }

    void resize(int newWidth, int newHeight) {
        width = newWidth;
        height = newHeight;
    }

    const char* click(int x, int y) {
        (void)x;

        int max_rows = visibleRows();
        if (max_rows <= 0) {
            return nullptr;
        }

        if (y < 16 || y >= (height - 8)) {
            return nullptr;
        }

        int target_row = (y / 16) - 1;
        if (target_row < 0 || target_row >= max_rows) {
            return nullptr;
        }

        int row = 0;
        struct TreeNode* hit = findNodeByRow(&root, &row, target_row);
        if (hit == nullptr) {
            return nullptr;
        }

        if (hit->isDirectory) {
            hit->isExpanded = !hit->isExpanded;
            return nullptr;
        }

        return hit->name->getData();
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
    int x, y, width, height;
    String* path;

    struct TreeNode* findNodeByRow(struct TreeNode* node, int* row, int target_row)
    {
        for (int i = 0; i < node->childrenCount; i++) {
            if (*row == target_row) {
                return &node->children[i];
            }

            (*row)++;
            if (node->children[i].isDirectory && node->children[i].isExpanded) {
                struct TreeNode* found = findNodeByRow(&node->children[i], row, target_row);
                if (found != nullptr) {
                    return found;
                }
            }
        }
        return nullptr;
    }

    int visibleRows() const
    {
        int usable_height = height - 24;
        if (usable_height < 16) {
            return 0;
        }

        int rows = usable_height / 16;
        if (rows < 1) {
            rows = 1;
        }

        return rows;
    }

    void drawClippedText(int x, int y, const char* text)
    {
        if (text == nullptr) {
            return;
        }

        int right_limit = width - 12;
        int pixels = right_limit - x;
        if (pixels < 8) {
            return;
        }

        int max_chars = pixels / 8;
        if (max_chars <= 0) {
            return;
        }

        int len = strlen(text);
        if (len <= max_chars) {
            gfx_draw_format_text(x, y, COLOR_BLACK, "%s", text);
            return;
        }

        char clipped[64];
        int keep = max_chars;
        if (keep > 63) {
            keep = 63;
        }

        if (keep >= 4) {
            keep -= 3;
            memcpy(clipped, text, keep);
            clipped[keep + 0] = '.';
            clipped[keep + 1] = '.';
            clipped[keep + 2] = '.';
            clipped[keep + 3] = 0;
        } else {
            for (int i = 0; i < keep; i++) {
                clipped[i] = '.';
            }
            clipped[keep] = 0;
        }

        gfx_draw_format_text(x, y, COLOR_BLACK, "%s", clipped);
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


    void drawTreeRecursive(struct TreeNode* node, int depth, int* row, int parent_row, int max_rows)
    {
        for (int i = 0; i < node->childrenCount; i++) {
            if (*row >= max_rows) {
                return;
            }

            struct TreeNode* child = &node->children[i];
            int current_row = *row;
            int current_y = (current_row + 1) * 16;
            int current_x = depth * 16;

            if (current_y + 15 >= height - 8) {
                return;
            }

            if (child->isDirectory && current_x + 16 < width - 8) {
                drawIcon(current_x, current_y, __folder_icon);
            } else if (current_x + 16 < width - 8) {
                drawIcon(current_x, current_y, __file_icon);
            }

            const char* name = child->name->getData();
            /* Only use name from last / */
            for (int j = 0; j < strlen(name)-1; j++) {
                if (name[j] == '/') {
                    name = name + j + 1;
                }
            }

            /* draw line from parent to current */
            if (depth > 1 && parent_row >= 0) {
                int parent_y = (parent_row + 1) * 16 + 8;
                int branch_x = (depth - 1) * 16 + 8;
                if (branch_x >= width - 8) {
                    branch_x = width - 9;
                }
                int line_to_x = current_x;
                if (line_to_x >= width - 8) {
                    line_to_x = width - 9;
                }

                /* Draw vertical line */
                gfx_draw_line(
                    branch_x, // parent x + 8
                    parent_y,
                    branch_x,
                    current_y + 8,
                    COLOR_BLACK
                );

                /* Draw horizontal line */
                gfx_draw_line(
                    branch_x,
                    current_y + 8,
                    line_to_x,
                    current_y + 8,
                    COLOR_BLACK
                );
            }

            drawClippedText(depth * 16 + 18, current_y + 5, name);

            (*row)++;
            if (child->isExpanded && child->isDirectory) {
                drawTreeRecursive(child, depth + 1, row, current_row, max_rows);
            }
        }
    }
};


#endif /* !__ */
