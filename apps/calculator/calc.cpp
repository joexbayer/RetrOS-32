#include <util.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>

#define BUTTON_WIDTH 30
#define BUTTON_HEIGHT 30
#define WIDTH 120
#define HEIGHT 170

static const char operations[] = "+-*/";

class Calculator : public Window {
private:
    int currentValue = 0;
    int previousValue = 0;
    char operation = 0;

public:
    Calculator() : Window(WIDTH, HEIGHT, "Calculator", 0) {
        drawUI();
    }

    void drawUI() {
        // Draw keypad
        drawRect(0, 0, WIDTH, HEIGHT, COLOR_WHITE);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                drawContouredRect(j * BUTTON_WIDTH, (2 - i) * BUTTON_HEIGHT + 50, BUTTON_WIDTH, BUTTON_HEIGHT);
                char num = '1' + j + i * 3;
                drawChar(j * BUTTON_WIDTH + 10, (2 - i) * BUTTON_HEIGHT + 60, num, 255);
            }
        }

        // Draw 0 button
        drawContouredRect(30*3, 50, BUTTON_WIDTH, BUTTON_HEIGHT);
        drawChar(30*3 + 10, 60, '0', 255);

        drawContouredRect(30*3, 80, BUTTON_WIDTH, BUTTON_HEIGHT*2);
        drawChar(30*3 + 10, 100, '=', 255);

        // Draw operation buttons
        for (int i = 0; i < 4; i++) {
            drawContouredRect(i * BUTTON_WIDTH, 140, BUTTON_WIDTH, BUTTON_HEIGHT);
            drawChar(10+ i * BUTTON_WIDTH, 150, operations[i], COLOR_BLACK);
        }

        // Draw input display
        updateDisplay();
    }

    void handleKeyPress(char key) {
        switch (key) {
            case '0' ... '9':
                currentValue = currentValue * 10 + (key - '0');
                updateDisplay();
                break;
            case '+':
            case '-':
            case '*':
            case '/':
                previousValue = currentValue;
                currentValue = 0;
                operation = key;
                updateDisplay();
                break;
            case '=':
            case '\n':  // Enter key
                executeOperation();
                operation = '=';
                previousValue = 0;
                updateDisplay();
                currentValue = 0;
                break;
            case '\b':  // Backspace key
                // Optional: Remove the last digit from currentValue
                currentValue /= 10;
                updateDisplay();
                break;
            default:
                // Handle other keys or ignore them
                break;
        }
    }

    void handleEvent() {
        

        while (1)
        {
            struct gfx_event event;
            while (1){
                int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
                if(ret == -1) continue;

                switch (event.event){
                case GFX_EVENT_MOUSE:{
                        /* check if a mouse click is inside of a file */
                        handleClick(event.data, event.data2);
                    }
                    break;
                case GFX_EVENT_EXIT:
                    exit();
                    break;
                case GFX_EVENT_KEYBOARD:
                    handleKeyPress(event.data);
                    break;
                default:
                    break;
                }
            }
        }
    }

    void handleClick(int x, int y) {
        // Handle number click for numbers 1 through 9
        if (y > 50 && y <= 50 + 3 * BUTTON_HEIGHT) {
            int col = x / BUTTON_WIDTH;
            int row = 2 - (y - 50) / BUTTON_HEIGHT;
            
            if (col >= 0 && col < 3 && row >= 0 && row < 3) {
                int num = 1 + col + row * 3;
                currentValue = currentValue * 10 + num;
                updateDisplay();
            }
        }

        // Handle 0 click
        int zeroButtonLeft = 30*3;
        int zeroButtonRight = 30*3 + BUTTON_WIDTH;
        int zeroButtonTop = 50;
        int zeroButtonBottom = 50 + BUTTON_HEIGHT;

        if (y > zeroButtonTop && y <= zeroButtonBottom && x > zeroButtonLeft && x <= zeroButtonRight) {
            currentValue = currentValue * 10;  // appending '0' is essentially multiplying by 10
            updateDisplay();
        }

        // Handle equals click
        int equalsButtonLeft = 30*3;
        int equalsButtonRight = 30*3 + BUTTON_WIDTH;
        int equalsButtonTop = 80;
        int equalsButtonBottom = 80 + BUTTON_HEIGHT*2;

        if (y > equalsButtonTop && y <= equalsButtonBottom && x > equalsButtonLeft && x <= equalsButtonRight) {
            executeOperation();
            operation = '=';
            previousValue = 0;
            updateDisplay();
            currentValue = 0;
        }
        
        // Handle operation click
        if (y > 140 && y < 140 + BUTTON_HEIGHT) {
            int idx = x / BUTTON_WIDTH; // updated the calculation based on x-coordinate
            if (idx >= 0 && idx < 4) {  // check if idx is within valid bounds
                previousValue = currentValue;
                currentValue = 0;
                operation = operations[idx];
                updateDisplay();
            }
        }

    }

    void updateDisplay() {
        // Clear previous display
        drawContouredRect(10, 10, WIDTH-20, 30);
        if(previousValue != 0)
            gfx_draw_format_text(20, 16, COLOR_BLACK, "   %p", previousValue);
        gfx_draw_format_text(20, 24, COLOR_BLACK, "%c  %p", operation == 0 ? ' ' : operation, currentValue);
    }

    void executeOperation() {
        switch (operation) {
            case '+':
                currentValue += previousValue;
                break;
            case '-':
                currentValue = previousValue - currentValue;
                break;
            case '*':
                currentValue *= previousValue;
                break;
            case '/':
                if (currentValue != 0)
                    currentValue = previousValue / currentValue;
                else
                    currentValue = 0; // Avoid divide by zero
                break;
        }
    }
};

extern "C" int main() {
    Calculator calc;
    calc.handleEvent();

    return 0;
}
