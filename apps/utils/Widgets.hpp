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
#include <utils/StdLib.hpp>

#define MAX_WIDGETS 32

typedef void (*Callback)();

/* Extended base class for all types of widgets */
class Widget {
public:
    virtual void draw(Window* window) = 0;
    virtual void Keyboard(char c) = 0;
    virtual void Mouse() = 0;
    virtual bool focusable() = 0; /* Indicates if the widget can receive focus */
    virtual ~Widget() {}
    
    /* Widget properties */
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool focused = false;

protected:
};

/* A simple button widget */
class Button : public Widget {
public:
    Button(int x, int y, int width, int height, char* text, Callback callback) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->text = text;
        this->callback = callback;
    }

    /* Draws the button */
    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, focused ? COLOR_VGA_LIGHT_GRAY : 30);
        window->drawText(x + 4, y + 4, text, COLOR_BLACK);
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
    Callback callback;
};

/* A simple input widget */
class Input : public Widget {
public:
    Input(int x, int y, int width, int height, char* text) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
        this->text = text;
        memset(data, 0, 100);
    }

    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, focused ? 28 : 30);

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
        printf("Input: %c\n", c);
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
    Label(int x, int y, int width, int height, char* text) {
        this->x = x;
        this->y = y;
        this->width = width;
        this->height = height;
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
    List(int x, int y, int width, int height, char** items, int itemCount) {
        this->x = x;
        this->y = y;
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
    Checkbox(int x, int y, bool value) {
        this->x = x;
        this->y = y;
        this->width = 12;
        this->height = 12;
        this->value = value;
    }

    void draw(Window* window) {
        window->drawContouredBox(x, y, width, height, 30);
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
};

class WidgetManager {
public:
    /**
     * @brief Adds a widget to the widget manager
     * 
     * @param widget The widget to add
     * @return int The index of the widget in the widget manager
     */
    int addWidget(Widget* widget) {
        if (widgetCount >= MAX_WIDGETS) {
            return -1;
        }

        widgets[widgetCount] = widget;
        widgetCount++;

        return widgetCount - 1;
    }

    void draw(Window* window) {
        for (int i = 0; i < widgetCount; i++) {
            widgets[i]->draw(window);
        }
    }

    void Keyboard(char c) {
        if(focusedWidget != -1){
            widgets[focusedWidget]->Keyboard(c);
        }
    }

    /**
     * @brief Handles mouse events
     * 
     * @param x coordinate
     * @param y coordinate
     */
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


    Widget* getWidget(int index) {
        return widgets[index];
    }

    int getWidgetCount() {
        return widgetCount;
    }

private:
    Widget* widgets[MAX_WIDGETS];
    int widgetCount = 0;
    int focusedWidget = -1;
};


#endif /* __WIDGETS_HPP__ */