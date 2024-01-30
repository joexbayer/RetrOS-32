/**
 * @file Widgets.hpp
 * @author Joe Bayer (joexbayer)
 * @brief Widgest and components library 
 * @version 0.1
 * @date 2024-01-20
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __WIDGETS_HPP__
#define __WIDGETS_HPP__

#include <utils/StringHelper.hpp>
#include <utils/Graphics.hpp>
#include <utils/Function.hpp>
#include <utils/StdLib.hpp>


#define MAX_WIDGETS 32
#define MAX_LAYOUTS 32

typedef void (*Callback)();

/* Extended base class for all types of widgets */
class Widget {
public:
    virtual void draw(Window* window) = 0;
    virtual void Keyboard(char c) = 0;
    virtual void Mouse() = 0;
    virtual bool focusable() = 0; /* Indicates if the widget can receive focus */
    virtual ~Widget() {}

    void disable() {
        disabled = true;
    }

    void enable() {
        disabled = false;
    }

    Widget* setTag(char* tag) {
        strcpy(this->tag, tag);
        return this;
    }

    char* getTag() {
        return tag;
    }
    
    /* Widget properties */
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool focused = false;
    bool disabled = false;

protected:
    char tag[16] = {0};
};

/**
 * @brief A simple button widget
 * 
 */
class Button : public Widget {
public:
    Button(int width, int height, char* text, Function callback) : callback(callback) {
        this->width = width;
        this->height = height;
        this->text = text;
    }

    /* Draws the button */
    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, focused ? COLOR_VGA_LIGHT_GRAY : 30);
        /* draw text in center */
        int textWidth = strlen(text) * 8;
        int textHeight = 8;
        window->drawText(x + width / 2 - textWidth / 2, y + height / 2 - textHeight / 2, text, COLOR_BLACK);
    }

    /* Ignores keyboard events */
    void Keyboard(char c) {}

    void Mouse() {
        callback();
    }

    bool focusable() {
        return false;
    }

private:
    char* text;
    Function callback;
};

/* A simple spacing widget */
class Spacing : public Widget {
public:
    Spacing(int width, int height) {
        this->width = width;
        this->height = height;
    }

    void draw(Window* window) {}

    void Keyboard(char c) {}

    void Mouse() {}

    bool focusable() {
        return false;
    }
};


/* A simple input widget */
class Input : public Widget {
public:
    Input(int width, int height, char* text, char* tag = "") {
        this->width = width;
        this->height = height;
        this->text = text;
        strcpy(this->tag, tag);
        memset(data, 0, 100);
    }

    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, focused ? 31 : 30);

        if(size == 0){
            window->drawText(x + 4, y + 4, text, COLOR_VGA_LIGHT_GRAY);
        } else {
            window->drawText(x + 4, y + 4, data, COLOR_BLACK);
        }
    }

    char* getData() {
        return data;
    }

    /* Handles keyboard events */
    void Keyboard(char c) {
        if (c == '\b') {
            if (size > 0) {
                size--;
                data[size] = 0;
            }
        } else {
            data[size] = c;
            size++;
            data[size] = '\0';
        }
    }

    void Mouse() {
        printf("Input clicked!\n");
    }

    bool focusable() {
        return true;
    }

private:
    char* text;
    char data[100];
    int size = 0;
};

/* A simple label widget */
class Label : public Widget {
public:
    Label(char* text) {
        this->width = strlen(text) * 8;
        this->height = 8;
        this->text = text;
    }

    void draw(Window* window) {
        window->drawText(x, y, text, COLOR_BLACK);
    }

    void Keyboard(char c) {}

    void Mouse() {}

    bool focusable() {
        return false;
    }

private:
    char* text;
};

/* A simple list widget */
class List : public Widget {
public:
    List(int width, int height, char** items, int itemCount) {
        this->width = width;
        this->height = height;
        this->items = items;
        this->itemCount = itemCount;
    }

    void draw(Window* window) {
        //window->drawContouredBox(x, y, width, height, focused ? 28 : 30);

        for (int i = 0; i < itemCount; i++) {
            window->drawCircle(x + 2, y + 6 + i * 12, 2, COLOR_VGA_LIGHT_GRAY, 1);
            window->drawText(x + 4, y + 4 + i * 12, items[i], COLOR_BLACK);
        }
    }

    void Keyboard(char c) {}

    void Mouse() {}

    bool focusable() {
        return false;
    }

private:
    char** items;
    int itemCount;
};

/* Simple Checkbox widget */
class Checkbox : public Widget {
public:
    Checkbox(bool value, char* text = "") {
        this->width = 12;
        this->height = 12;
        this->value = value;
        this->text = text;
    }

    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, 30);
        window->drawText(x + 16, y + 4, text, COLOR_BLACK);
        if (value) {
            window->drawCircle(x + width - 6, y + 6, 2, COLOR_VGA_LIGHT_GRAY, 1);
        }
    }

    void Keyboard(char c) {}

    void Mouse() {
        value = !value;
    }

    bool focusable() {
        return true;
    }

private:
    bool value;
    char* text;
};

typedef enum {
    HORIZONTAL,
    VERTICAL
} LayoutType;

typedef enum {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    CENTER
} LayoutPosition;

typedef int LayoutHandle;

enum {
    LAYOUT_FLAG_NONE = 0,
    LAYOUT_FLAG_BORDER = 1
};

class Layout {
public:
    Layout(int width, int height, LayoutType type, int flags) {
        this->x = 0;
        this->y = 0;
        this->width = width;
        this->height = height;
        this->type = type;
        this->offset = 0;
        this->flags = flags;
        memset(widgets, 0, MAX_WIDGETS * sizeof(Widget*));
    }
    
    Layout(int x, int y, int width, int height, LayoutType type, int flags) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->type = type;
        this->offset = 0;
        this->flags = flags;
        memset(widgets, 0, MAX_WIDGETS * sizeof(Widget*));
    }

    ~Layout() {
        for (int i = 0; i < widgetCount; i++) {
            delete widgets[i];
        }
    }

    Widget* getByTag(char* tag) {
        for (int i = 0; i < widgetCount; i++) {
            if (strlen(widgets[i]->getTag()) > 1 && strcmp(widgets[i]->getTag(), tag) == 0) {
                return widgets[i];
            }
        }
        return nullptr;
    }

    void draw(Window* window) {
        if (flags & LAYOUT_FLAG_BORDER) 
            window->drawContouredBox(x, y, width, height, 30);
        
        for (int i = 0; i < widgetCount; i++) {
            widgets[i]->draw(window);
        }
    }

    void Keyboard(char c) {
        if (focusedWidget != -1) {
            widgets[focusedWidget]->Keyboard(c);
        }
    }

    int addWidget(Widget* widget, LayoutPosition position) {
        widgets[widgetCount] = widget;
        widgetCount++;

        switch(type) {
            case HORIZONTAL:
                widget->x = x + offset + 2;
                widget->y = y + 2;
                offset += widget->width + 4;
                break;
            case VERTICAL:
                /* switch based on position, put widgets under each other */
                switch (position) {
                    case LEFT:
                        widget->x = x + 2;
                        widget->y = y + offset;
                        break;
                    case RIGHT:
                        widget->x = x + width - widget->width -2;
                        widget->y = y + offset;
                        break;
                    case CENTER:
                        /* puts widget in the middle but still stacked under last */
                        widget->x = x + width / 2 - widget->width / 2;
                        widget->y = y + offset;
                        break;
                }
                offset += widget->height + 4;
                break;
        }

        printf("Added widget, new offset %d\n", offset);

        return widgetCount;
    }

     void Mouse(int x, int y) {
        for (int i = 0; i < widgetCount; i++) {
            Widget* widget = widgets[i];

            /* Check if the widget is focusable and the mouse is within its bounds */
            bool isWithinBounds = 
                x >= widget->x && x <= widget->x + widget->width &&
                y >= widget->y && y <= widget->y + widget->height;

            if (isWithinBounds) {
                if (widget->focusable()) {

                    /* Unfocus the previously focused widget, if any */
                    if(focusedWidget != -1)
                        widgets[focusedWidget]->focused = false;
                    
                    /* Focus the current widget */
                    focusedWidget = i;
                    widget->focused = true;
                    printf("Focused widget: %d\n", focusedWidget);
                }

                widget->Mouse();
                return;
            }
        }
    }

    int x;
    int y;
    int width;
    int height;

private:
    LayoutType type;
    Widget* widgets[MAX_WIDGETS];
    int widgetCount = 0;
    int focusedWidget = -1;
    int flags = 0;

    int offset = 0;
};

class WidgetManager {
public:

    ~WidgetManager() {
        for (int i = 0; i < layoutCount; i++) {
            delete layouts[i];
        }
    }

    int addLayout(Layout* layout) {
        layouts[layoutCount] = layout;
        layoutCount++;

        yOffset += layout->height + 2;

        return layoutCount-1;
    }

    /**
     * @brief Adds a widget to the widget manager
     * 
     * @param widget The widget to add
     * @return int The index of the widget in the widget manager
     */
    int addWidget(LayoutHandle lh, LayoutPosition lp,  Widget* widget) {
        if (lh >= layoutCount) {
            return -1;
        }

        return layouts[lh]->addWidget(widget, lp);;
    }

    void draw(Window* window) {
        for (int i = 0; i < layoutCount; i++) {
            layouts[i]->draw(window);
        }
    }

    Widget* getByTag(char* tag) {
        printf("Searching for widget with tag %s\n", tag);
        for (int i = 0; i < layoutCount; i++) {
            Widget* widget = layouts[i]->getByTag(tag);
            if (widget != nullptr) {
                return widget;
            }
        }

        return nullptr;
    }

    void Keyboard(char c) {
        if (focusedLayout != -1) {
            layouts[focusedLayout]->Keyboard(c);
        }
    }

    /**
     * @brief Handles mouse events
     * 
     * @param x coordinate
     * @param y coordinate
     */
    void Mouse(int x, int y) {
        for (int i = 0; i < layoutCount; i++) {
            Layout* layout = layouts[i];

            /* Check if the widget is focusable and the mouse is within its bounds */
            bool isWithinBounds = 
                x >= layout->x && x <= layout->x + layout->width &&
                y >= layout->y && y <= layout->y + layout->height;

            focusedLayout = i;

            if (isWithinBounds) {
                layout->Mouse(x, y);
                return;
            }
        }
    }
    
    int xOffset = 0;
    int yOffset = 0;

private:
    Layout* layouts[MAX_LAYOUTS];
    int layoutCount = 0;
    int focusedLayout = -1;
};


#endif /* __WIDGETS_HPP__ */