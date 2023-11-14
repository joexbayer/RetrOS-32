#include <utils/Graphics.hpp>
#include <util.h>
#include <colors.h>

extern float sin_60[60];
extern float cos_60[60];

int angle = 0;


struct Point3D {
    float x, y, z;
};

struct Point2D {
    int x, y;
};

class CubeDemo : public Window {
public:
    CubeDemo(int width, int height) : Window(200, 200, "Cube", 1) {
        this->angle = 0;
        this->width = width;
        this->height = height;
    }

    void drawCube() {
        gfx_draw_rectangle(0, 0, width, height, COLOR_VGA_LIGHTEST_GRAY);
        for (int i = 0; i < 12; i++) {
            Point2D p1 = project(cube[edges[i][0]]);
            Point2D p2 = project(cube[edges[i][1]]);
            gfx_draw_line(p1.x, p1.y, p2.x, p2.y, COLOR_VGA_DARKEST_GRAY);
        }
    }

    void rotateCube() {
        /* Currently we only have 60 precalculated sin / cos values. */
        angle = (angle + 1) % 60;
        float sinValue = sin_60[angle];
        float cosValue = cos_60[angle];
        
        for (int i = 0; i < 8; i++) {
            // rotation around the y-axis
            float newX = cube[i].x * cosValue - cube[i].z * sinValue;
            float newZ = cube[i].x * sinValue + cube[i].z * cosValue;
            
            // rotation around the z-axis
            float newY = newX * cosValue - cube[i].y * sinValue;
            newX = newX * sinValue + cube[i].y * cosValue;

            cube[i].x = newX;
            cube[i].y = newY;
            cube[i].z = newZ;
        }
    }

    void setSize(int width, int height) {
        this->width = width;
        this->height = height;
    }

private:
    int angle;
    Point3D cube[8] = {
        {-1, -1, -1},
        {1, -1, -1},
        {1, 1, -1},
        {-1, 1, -1},
        {-1, -1, 1},
        {1, -1, 1},
        {1, 1, 1},
        {-1, 1, 1},
    };

    int edges[12][2] = {
        /* Back face */
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        /* Front face */
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
         /* Connecting edges */
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };

    int width, height;

    /* Project 3D cube to 2D */
    Point2D project(Point3D point) {
        return {(int)(point.x * (width/4) + height/2), (int)(point.y * (width/4) + height/2)};
    }

};



int main()
{
    CubeDemo demo(200, 200);
    struct gfx_event e;
    demo.setHeader("3D");

    while (1){
        demo.rotateCube();
        demo.drawCube();
        sleep(20);

        gfx_get_event(&e, GFX_EVENT_NONBLOCKING);
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