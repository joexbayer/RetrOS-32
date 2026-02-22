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
        memset(&root, 0, sizeof(root));
        root.name = path;
        root.isDirectory = 1;
        root.isExpanded = 1;
        root.childrenCount = 0;
        root.childrenCapacity = 16;
        root.children = new TreeNode[root.childrenCapacity];
        if (root.children != nullptr) {
            memset(root.children, 0, root.childrenCapacity * sizeof(TreeNode));
            loadTree(&root, 0, path, 0);
        }
    }

    ~TreeView() {
        clearChildren(&root);
        if (root.children != nullptr) {
            delete[] root.children;
            root.children = nullptr;
        }
        root.childrenCapacity = 0;
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

    void refresh() {
        clearChildren(&root);
        if (root.children == nullptr) {
            root.childrenCapacity = 16;
            root.children = new TreeNode[root.childrenCapacity];
            if (root.children == nullptr) {
                root.childrenCapacity = 0;
                return;
            }
        }
        memset(root.children, 0, root.childrenCapacity * sizeof(TreeNode));
        root.childrenCount = 0;
        loadTree(&root, 0, path, 0);
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
        refresh();
    }

private:
    /* Logic of caching and interaction */
    struct TreeNode {
        String* name;
        int x, y, depth, entries;
        int isDirectory;
        int isExpanded;
        int childrenCount;
        int childrenCapacity;
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

    void clearNodeRecursive(struct TreeNode* node)
    {
        if (node == nullptr) {
            return;
        }

        clearChildren(node);

        if (node->children != nullptr) {
            delete[] node->children;
            node->children = nullptr;
        }
        node->childrenCapacity = 0;
        node->childrenCount = 0;

        if (node->name != nullptr) {
            delete node->name;
            node->name = nullptr;
        }
    }

    void clearChildren(struct TreeNode* node)
    {
        if (node == nullptr || node->children == nullptr) {
            if (node != nullptr) {
                node->childrenCount = 0;
            }
            return;
        }

        for (int i = 0; i < node->childrenCount; i++) {
            clearNodeRecursive(&node->children[i]);
        }

        memset(node->children, 0, node->childrenCapacity * sizeof(TreeNode));
        node->childrenCount = 0;
    }

    bool ensureChildCapacity(struct TreeNode* parent)
    {
        if (parent == nullptr) {
            return false;
        }

        if (parent->children == nullptr) {
            parent->childrenCapacity = 16;
            parent->children = new TreeNode[parent->childrenCapacity];
            if (parent->children == nullptr) {
                parent->childrenCapacity = 0;
                return false;
            }
            memset(parent->children, 0, parent->childrenCapacity * sizeof(TreeNode));
        }

        if (parent->childrenCount < parent->childrenCapacity) {
            return true;
        }

        int new_capacity = parent->childrenCapacity * 2;
        if (new_capacity < 16) {
            new_capacity = 16;
        }

        TreeNode* resized = new TreeNode[new_capacity];
        if (resized == nullptr) {
            return false;
        }

        memset(resized, 0, new_capacity * sizeof(TreeNode));
        memcpy(resized, parent->children, parent->childrenCount * sizeof(TreeNode));
        delete[] parent->children;

        parent->children = resized;
        parent->childrenCapacity = new_capacity;
        return true;
    }

    int pathlen(char* path) {
        int len = 0;
        while (path[len] != 0 && path[len] != ' ' && len < 8) len++;
        return len;
    }

    struct TreeNode* insertNode(struct TreeNode* parent, const char* name, int x, int y, int isDirectory) {
        if (!ensureChildCapacity(parent)) {
            return nullptr;
        }

        struct TreeNode* node = &parent->children[parent->childrenCount];
        memset(node, 0, sizeof(TreeNode));

        node->name = new String(name);
        if (node->name == nullptr) {
            return nullptr;
        }

        node->x = x;
        node->y = y;
        node->isDirectory = isDirectory;
        node->isExpanded = 0;
        node->childrenCount = 0;
        node->childrenCapacity = 0;
        node->children = nullptr;

        if (isDirectory) {
            node->childrenCapacity = 16;
            node->children = new TreeNode[node->childrenCapacity];
            if (node->children == nullptr) {
                delete node->name;
                node->name = nullptr;
                node->childrenCapacity = 0;
                return nullptr;
            }
            memset(node->children, 0, node->childrenCapacity * sizeof(TreeNode));
        }

        parent->childrenCount++;
        return node;
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
            if (newRoot == nullptr) {
                continue;
            }

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
