#include <utils/Graphics.hpp>
#include <utils/StdLib.hpp>
#include <utils/Widgets.hpp>
#include <utils/Thread.hpp>
#include <utils/Function.hpp>
#include <utils/ColorPicker.hpp>

#include <libc.h>
#include <colors.h>
#include <lib/printf.h>

#include "Users.hpp"

class Users : public Window {
public:
    Users(int width, int height) : Window(200, 200, "Users", 1) {
        this->width = width;
        this->height = height;

        /* Create widgets */
        widgets = new WidgetManager();
        LayoutHandle main = widgets->addLayout(new Layout(
            10, 10,
            180, 180,
            VERTICAL,
            0
        ));

        widgets->addWidget(main, LEFT, new Button(100, 14, "Color", Function([this]() {
            int color = color_picker_entry();
            
            char buffer[100] = {0};
            sprintf(buffer, "bg 0x%x", color);
            printf("Executing: %s\n", buffer);
            system(buffer);
        })));
        widgets->addWidget(main, RIGHT, new Button(100, 14, "Create", Function([this]() {

            Thread* editor = new Thread(editorEntry, 0);
            editor->start(0);
        
        })));

        Input* input = new Input(100, 12, "Input");
        widgets->addWidget(main, CENTER, input);
    
        widgets->addWidget(main, LEFT, new Checkbox(true));
        widgets->addWidget(main, LEFT, new Label("Checkbox"));
        widgets->addWidget(main, LEFT, new Checkbox(false));
    }


    int eventHandler(struct gfx_event* event) {
        switch (event->event){
        case GFX_EVENT_RESOLUTION:
            /* update screensize */
            break;
        case GFX_EVENT_EXIT:
            delete widgets;
            exit();
            /* exit */
            return -1;
        case GFX_EVENT_KEYBOARD:
            widgets->Keyboard(event->data);
            /* keyboard event in e.data */
            break;
        case GFX_EVENT_MOUSE:
            widgets->Mouse(event->data, event->data2);
            break;
        }
        return 0;
    }

    void draw() {
        /* Clear screen */
        drawRect(0, 0, width, height, 30);
        /* Draw widgets */
        widgets->draw(this);
    }

private:
    int width;
    int height;
    WidgetManager* widgets;
};

class MyClass {
public:
    void memberFunction() {
        Function f = [this]() { this->anotherFunction(); };
        f(); // Invoke the lambda
    }


    void anotherFunction() {
        printf("Hello from another function!\n");
    }
};


int main()
{
    MyClass c;
    c.memberFunction();

    Users t(200, 200);
    t.draw();

    struct gfx_event e;
    while (1){
        gfx_get_event(&e, GFX_EVENT_BLOCKING); /* alt: GFX_EVENT_NONBLOCKING */
        if(t.eventHandler(&e) == -1) break;
        t.draw();
    }
    return 0;
}