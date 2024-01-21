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

#define MAX_WIDGETS 10

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
        window->drawRect(x, y, width, height, focused ? COLOR_WHITE : COLOR_VGA_LIGHT_GRAY);
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
    }

    void draw(Window* window) {

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

    void Mouse() {}

    bool focusable() {
        return true;
    }

private:
    char* text;
    char data[100];
    int size = 0;
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
                widget->focusable() &&
                x >= widget->x && x <= widget->x + widget->width &&
                y >= widget->y && y <= widget->y + widget->height;

            if (isWithinBounds) {
                /* Unfocus the previously focused widget, if any */
                if (focusedWidget != -1) {
                    widgets[focusedWidget]->focused = false;
                }

                /* Focus the current widget */
                focusedWidget = i;
                widget->Mouse();
                widget->focused = true;
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