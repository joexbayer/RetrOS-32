#include <lib/graphics.h>
#include <util.h>
#include <colors.h>
#include <math.h>

float factorial(int n) {
    if (n == 0) return 1;
    float result = 1;
    for (int i = 1; i <= n; i++) {
        result *= i;
    }
    return result;
}

float approx_sin(float x) {
    return x 
           - (x*x*x)/factorial(3) 
           + (x*x*x*x*x)/factorial(5) 
           - (x*x*x*x*x*x*x)/factorial(7);
}

const int GRAPH_WIDTH = 400;
const int GRAPH_HEIGHT = 200;
const int GRAPH_OFFSET_X = 50; /* Increased offset for axis labels */
const int GRAPH_OFFSET_Y = 50; /* Increased offset for axis labels */
const int ORIGIN_X = GRAPH_OFFSET_X + GRAPH_WIDTH / 2;
const int ORIGIN_Y = GRAPH_OFFSET_Y + GRAPH_HEIGHT / 2;
const int GRID_SPACING = 20; /* Spacing for grid lines */

class GraphDemo : public Window {
public:
    GraphDemo(int width, int height) : Window(width, height, "Graph", 1) {
        this->width = width;
        this->height = height;
    }

    void drawGraph() {
        drawRect(0, 0, width, height, 28);
        
        /* Draw the rectangle for the graph */
        drawContouredRect(GRAPH_OFFSET_X, GRAPH_OFFSET_Y, GRAPH_WIDTH, GRAPH_HEIGHT);

        /* Draw grid lines */
        for (int i = GRAPH_OFFSET_X + GRID_SPACING; i < GRAPH_OFFSET_X + GRAPH_HEIGHT; i += GRID_SPACING) {
            drawLine(i, GRAPH_OFFSET_Y, i, GRAPH_OFFSET_Y + GRAPH_WIDTH, COLOR_VGA_MEDIUM_GRAY);
            //drawFormatText(i, ORIGIN_Y + 5, COLOR_VGA_DARKEST_GRAY, "%d", i - ORIGIN_X);
        }
        for (int i = GRAPH_OFFSET_Y + GRID_SPACING; i < GRAPH_OFFSET_Y + GRAPH_WIDTH; i += GRID_SPACING) {
           drawLine(GRAPH_OFFSET_X, i, GRAPH_OFFSET_X + GRAPH_HEIGHT, i, COLOR_VGA_MEDIUM_GRAY);
           //drawFormatText(ORIGIN_X - 25, i, COLOR_VGA_DARKEST_GRAY, "%d", ORIGIN_Y - i);
        }

        /* Draw X and Y axis inside the rectangle */
        drawLine(GRAPH_OFFSET_X, ORIGIN_Y, GRAPH_OFFSET_X + GRAPH_HEIGHT, ORIGIN_Y, COLOR_VGA_DARKEST_GRAY);
        drawLine(ORIGIN_X, GRAPH_OFFSET_Y, ORIGIN_X, GRAPH_OFFSET_Y + GRAPH_WIDTH, COLOR_VGA_DARKEST_GRAY);

        /* Axis labels */
        drawText(GRAPH_OFFSET_X + GRAPH_HEIGHT + 5, ORIGIN_Y, "X", COLOR_VGA_DARKEST_GRAY);
        drawText(ORIGIN_X - 10, GRAPH_OFFSET_Y - 20, "Y", COLOR_VGA_DARKEST_GRAY);

        /* Draw sine wave inside the rectangle */
        for (int x = 0; x < GRAPH_WIDTH; x++) {
            float radianValue = (float)x * 2 * PI / GRAPH_WIDTH - PI;
            int y = ORIGIN_Y - (int)(100 * approx_sin(radianValue));
            drawPixel(GRAPH_OFFSET_X + x, y, COLOR_VGA_LIGHT_BLUE);
        }

        /* Display the formula under the graph rectangle */
        drawText(GRAPH_OFFSET_X, GRAPH_OFFSET_Y + GRAPH_HEIGHT + 20, "y = sin(x)", COLOR_VGA_DARKEST_GRAY);
    }

    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
    }

private:
    int width, height;
};

int main()
{
    GraphDemo demo(GRAPH_WIDTH + 2 * GRAPH_OFFSET_X, GRAPH_HEIGHT + 2 * GRAPH_OFFSET_Y + 50);
    struct gfx_event e;
    demo.setHeader("Graphs");

    while (1){
        demo.drawGraph();
        sleep(20);

        gfx_get_event(&e, GFX_EVENT_BLOCKING);
        switch (e.event)
        {
        case GFX_EVENT_RESOLUTION:
            demo.setSize(e.data, e.data2);
            break;
        case GFX_EVENT_EXIT:
            return 0;
        }
    }

    return 0;
}
