/**
 * @file wolf3d.cpp
 * @author Joe Bayer (joexbayer)
 * @brief Wolfstein (like) 3D game
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <utils/Graphics.hpp>
#include <util.h>
#include <colors.h>
#include <math.h>

float sin_values[360] = {0.000000, 0.017452, 0.034899, 0.052336, 0.069756, 0.087156, 0.104528, 0.121869, 0.139173, 0.156434, 0.173648, 0.190809, 0.207912, 0.224951, 0.241922, 0.258819, 0.275637, 0.292372, 0.309017, 0.325568, 0.342020, 0.358368, 0.374607, 0.390731, 0.406737, 0.422618, 0.438371, 0.453990, 0.469472, 0.484810, 0.500000, 0.515038, 0.529919, 0.544639, 0.559193, 0.573576, 0.587785, 0.601815, 0.615661, 0.629320, 0.642788, 0.656059, 0.669131, 0.681998, 0.694658, 0.707107, 0.719340, 0.731354, 0.743145, 0.754710, 0.766044, 0.777146, 0.788011, 0.798636, 0.809017, 0.819152, 0.829038, 0.838671, 0.848048, 0.857167, 0.866025, 0.874620, 0.882948, 0.891007, 0.898794, 0.906308, 0.913545, 0.920505, 0.927184, 0.933580, 0.939693, 0.945519, 0.951057, 0.956305, 0.961262, 0.965926, 0.970296, 0.974370, 0.978148, 0.981627, 0.984808, 0.987688, 0.990268, 0.992546, 0.994522, 0.996195, 0.997564, 0.998630, 0.999391, 0.999848, 1.000000, 0.999848, 0.999391, 0.998630, 0.997564, 0.996195, 0.994522, 0.992546, 0.990268, 0.987688, 0.984808, 0.981627, 0.978148, 0.974370, 0.970296, 0.965926, 0.961262, 0.956305, 0.951057, 0.945519, 0.939693, 0.933580, 0.927184, 0.920505, 0.913545, 0.906308, 0.898794, 0.891007, 0.882948, 0.874620, 0.866025, 0.857167, 0.848048, 0.838671, 0.829038, 0.819152, 0.809017, 0.798636, 0.788011, 0.777146, 0.766044, 0.754710, 0.743145, 0.731354, 0.719340, 0.707107, 0.694658, 0.681998, 0.669131, 0.656059, 0.642788, 0.629320, 0.615661, 0.601815, 0.587785, 0.573576, 0.559193, 0.544639, 0.529919, 0.515038, 0.500000, 0.484810, 0.469472, 0.453990, 0.438371, 0.422618, 0.406737, 0.390731, 0.374607, 0.358368, 0.342020, 0.325568, 0.309017, 0.292372, 0.275637, 0.258819, 0.241922, 0.224951, 0.207912, 0.190809, 0.173648, 0.156434, 0.139173, 0.121869, 0.104528, 0.087156, 0.069756, 0.052336, 0.034899, 0.017452, 0.000000, -0.017452, -0.034899, -0.052336, -0.069756, -0.087156, -0.104528, -0.121869, -0.139173, -0.156434, -0.173648, -0.190809, -0.207912, -0.224951, -0.241922, -0.258819, -0.275637, -0.292372, -0.309017, -0.325568, -0.342020, -0.358368, -0.374607, -0.390731, -0.406737, -0.422618, -0.438371, -0.453990, -0.469472, -0.484810, -0.500000, -0.515038, -0.529919, -0.544639, -0.559193, -0.573576, -0.587785, -0.601815, -0.615661, -0.629320, -0.642788, -0.656059, -0.669131, -0.681998, -0.694658, -0.707107, -0.719340, -0.731354, -0.743145, -0.754710, -0.766044, -0.777146, -0.788011, -0.798636, -0.809017, -0.819152, -0.829038, -0.838671, -0.848048, -0.857167, -0.866025, -0.874620, -0.882948, -0.891007, -0.898794, -0.906308, -0.913545, -0.920505, -0.927184, -0.933580, -0.939693, -0.945519, -0.951057, -0.956305, -0.961262, -0.965926, -0.970296, -0.974370, -0.978148, -0.981627, -0.984808, -0.987688, -0.990268, -0.992546, -0.994522, -0.996195, -0.997564, -0.998630, -0.999391, -0.999848, -1.000000, -0.999848, -0.999391, -0.998630, -0.997564, -0.996195, -0.994522, -0.992546, -0.990268, -0.987688, -0.984808, -0.981627, -0.978148, -0.974370, -0.970296, -0.965926, -0.961262, -0.956305, -0.951057, -0.945519, -0.939693, -0.933580, -0.927184, -0.920505, -0.913545, -0.906308, -0.898794, -0.891007, -0.882948, -0.874620, -0.866025, -0.857167, -0.848048, -0.838671, -0.829038, -0.819152, -0.809017, -0.798636, -0.788011, -0.777146, -0.766044, -0.754710, -0.743145, -0.731354, -0.719340, -0.707107, -0.694658, -0.681998, -0.669131, -0.656059, -0.642788, -0.629320, -0.615661, -0.601815, -0.587785, -0.573576, -0.559193, -0.544639, -0.529919, -0.515038, -0.500000, -0.484810, -0.469472, -0.453990, -0.438371, -0.422618, -0.406737, -0.390731, -0.374607, -0.358368, -0.342020, -0.325568, -0.309017, -0.292372, -0.275637, -0.258819, -0.241922, -0.224951, -0.207912, -0.190809, -0.173648, -0.156434, -0.139173, -0.121869, -0.104528, -0.087156, -0.069756, -0.052336, -0.034899, -0.017452};

float cos_values[360] = {1.000000, 0.999848, 0.999391, 0.998630, 0.997564, 0.996195, 0.994522, 0.992546, 0.990268, 0.987688, 0.984808, 0.981627, 0.978148, 0.974370, 0.970296, 0.965926, 0.961262, 0.956305, 0.951057, 0.945519, 0.939693, 0.933580, 0.927184, 0.920505, 0.913545, 0.906308, 0.898794, 0.891007, 0.882948, 0.874620, 0.866025, 0.857167, 0.848048, 0.838671, 0.829038, 0.819152, 0.809017, 0.798636, 0.788011, 0.777146, 0.766044, 0.754710, 0.743145, 0.731354, 0.719340, 0.707107, 0.694658, 0.681998, 0.669131, 0.656059, 0.642788, 0.629320, 0.615661, 0.601815, 0.587785, 0.573576, 0.559193, 0.544639, 0.529919, 0.515038, 0.500000, 0.484810, 0.469472, 0.453990, 0.438371, 0.422618, 0.406737, 0.390731, 0.374607, 0.358368, 0.342020, 0.325568, 0.309017, 0.292372, 0.275637, 0.258819, 0.241922, 0.224951, 0.207912, 0.190809, 0.173648, 0.156434, 0.139173, 0.121869, 0.104528, 0.087156, 0.069756, 0.052336, 0.034899, 0.017452, 0.000000, -0.017452, -0.034899, -0.052336, -0.069756, -0.087156, -0.104528, -0.121869, -0.139173, -0.156434, -0.173648, -0.190809, -0.207912, -0.224951, -0.241922, -0.258819, -0.275637, -0.292372, -0.309017, -0.325568, -0.342020, -0.358368, -0.374607, -0.390731, -0.406737, -0.422618, -0.438371, -0.453990, -0.469472, -0.484810, -0.500000, -0.515038, -0.529919, -0.544639, -0.559193, -0.573576, -0.587785, -0.601815, -0.615661, -0.629320, -0.642788, -0.656059, -0.669131, -0.681998, -0.694658, -0.707107, -0.719340, -0.731354, -0.743145, -0.754710, -0.766044, -0.777146, -0.788011, -0.798636, -0.809017, -0.819152, -0.829038, -0.838671, -0.848048, -0.857167, -0.866025, -0.874620, -0.882948, -0.891007, -0.898794, -0.906308, -0.913545, -0.920505, -0.927184, -0.933580, -0.939693, -0.945519, -0.951057, -0.956305, -0.961262, -0.965926, -0.970296, -0.974370, -0.978148, -0.981627, -0.984808, -0.987688, -0.990268, -0.992546, -0.994522, -0.996195, -0.997564, -0.998630, -0.999391, -0.999848, -1.000000, -0.999848, -0.999391, -0.998630, -0.997564, -0.996195, -0.994522, -0.992546, -0.990268, -0.987688, -0.984808, -0.981627, -0.978148, -0.974370, -0.970296, -0.965926, -0.961262, -0.956305, -0.951057, -0.945519, -0.939693, -0.933580, -0.927184, -0.920505, -0.913545, -0.906308, -0.898794, -0.891007, -0.882948, -0.874620, -0.866025, -0.857167, -0.848048, -0.838671, -0.829038, -0.819152, -0.809017, -0.798636, -0.788011, -0.777146, -0.766044, -0.754710, -0.743145, -0.731354, -0.719340, -0.707107, -0.694658, -0.681998, -0.669131, -0.656059, -0.642788, -0.629320, -0.615661, -0.601815, -0.587785, -0.573576, -0.559193, -0.544639, -0.529919, -0.515038, -0.500000, -0.484810, -0.469472, -0.453990, -0.438371, -0.422618, -0.406737, -0.390731, -0.374607, -0.358368, -0.342020, -0.325568, -0.309017, -0.292372, -0.275637, -0.258819, -0.241922, -0.224951, -0.207912, -0.190809, -0.173648, -0.156434, -0.139173, -0.121869, -0.104528, -0.087156, -0.069756, -0.052336, -0.034899, -0.017452, -0.000000, 0.017452, 0.034899, 0.052336, 0.069756, 0.087156, 0.104528, 0.121869, 0.139173, 0.156434, 0.173648, 0.190809, 0.207912, 0.224951, 0.241922, 0.258819, 0.275637, 0.292372, 0.309017, 0.325568, 0.342020, 0.358368, 0.374607, 0.390731, 0.406737, 0.422618, 0.438371, 0.453990, 0.469472, 0.484810, 0.500000, 0.515038, 0.529919, 0.544639, 0.559193, 0.573576, 0.587785, 0.601815, 0.615661, 0.629320, 0.642788, 0.656059, 0.669131, 0.681998, 0.694658, 0.707107, 0.719340, 0.731354, 0.743145, 0.754710, 0.766044, 0.777146, 0.788011, 0.798636, 0.809017, 0.819152, 0.829038, 0.838671, 0.848048, 0.857167, 0.866025, 0.874620, 0.882948, 0.891007, 0.898794, 0.906308, 0.913545, 0.920505, 0.927184, 0.933580, 0.939693, 0.945519, 0.951057, 0.956305, 0.961262, 0.965926, 0.970296, 0.974370, 0.978148, 0.981627, 0.984808, 0.987688, 0.990268, 0.992546, 0.994522, 0.996195, 0.997564, 0.998630, 0.999391, 0.999848};

class Wolfstein : public Window {
public:
    Wolfstein(int width, int height) : Window(width, height, "Wolfstein", 1) {
        this->m_width = width;
        this->m_height = height;
    }

    void render(){
        /* white sky */
        gfx_draw_rectangle(0, 0, m_width, m_height/2, COLOR_WHITE);
        gfx_draw_rectangle(0, m_height/2, m_width, m_height/2, COLOR_VGA_MEDIUM_DARK_GRAY);
        
        drawRays();
        drawMinimap();
    }

    /* input */
    void input(char key){
        switch (key){
        case 'a':{
                m_player.angle -= 6;
                m_player.angle = FixAng(m_player.angle);
                m_player.dir.x = cos(m_player.angle);
                m_player.dir.y = -sin(m_player.angle);

                render();
            }
            break;
        case 'd':{
                m_player.angle += 6;
                m_player.angle = FixAng(m_player.angle);

                m_player.dir.x = cos(m_player.angle);
                m_player.dir.y = -sin(m_player.angle);
                render();
            }
            break;
        case 'w':{
                m_player.pos.x += m_player.dir.x * 0.2;
                m_player.pos.y += m_player.dir.y * 0.2;
                render();
            }
            break;
        case 's':{
                m_player.pos.x -= m_player.dir.x * 0.2;
                m_player.pos.y -= m_player.dir.y * 0.2;
                render();
            }
            break;
        case 'q':{
                showDebug = !showDebug;
                render();
            }
        default:
            break;
        }
    }

    void setSize(int width, int height) {
        this->m_width = width;
        this->m_height = height;
    }

private:
    struct Point2D {
        float x, y;
    };

    struct Player {
        struct Point2D pos;
        struct Point2D dir;
        int angle;
    } m_player = { {4, 4}, {1, 0}, 0 };

    int m_width;
    int m_height;

    char showDebug = 0;

    float cos(int angle){
        return cos_values[CLAMP(angle, 0, 359)];
    }

    float sin(int angle){
        return sin_values[CLAMP(angle, 0, 359)];
    }

    #define RED 0xE0
    #define GREEN 0x1C
    #define BLUE 0x03
    unsigned char m_map[8*8] = {
        1,1,1,1,1,1,1,1,
        1,0,RED,0,GREEN,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,BLUE,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,	
    };

    void drawMinimap() {
        if(!showDebug){
            return;
        }

        int rectSize = 8;
        for (int i = 0; i < 8; i++) {
            int rectY = i * rectSize;
            for (int j = 0; j < 8; j++) {
                int rectX = j * rectSize;
                int color = m_map[i * 8 + j] != 0 ? COLOR_BLACK : COLOR_WHITE;
                gfx_draw_rectangle_rgb(rectX, rectY, rectSize, rectSize, color);
            }
        }

        /* Draw player */
        int playerX = m_player.pos.x * rectSize;
        int playerY = m_player.pos.y * rectSize;
        gfx_draw_circle(playerX, playerY, 2, COLOR_VGA_GREEN, 1);

        /* draw player rays */
        for(int ray = -30; ray < 30; ray++){
            int rayAngle = FixAng(m_player.angle + ray);

            /* Ray direction */
            float dirX = cos(FixAng(rayAngle));
            float dirY = -sin(FixAng(rayAngle));

            /* draw ray */
            gfx_draw_line(playerX, playerY, playerX + dirX * 8, playerY + dirY * 8, COLOR_BLUE);
        }
    }

    void drawRays() {
        
        int rays = 60;
        int i = 0;
        int rectSize = (m_width + rays - 1) / rays;

        for (int rayIndex = -30; rayIndex < 30; rayIndex++) {
            /* Calculate ray's angle */
            int rayAngle = FixAng(m_player.angle + rayIndex); /* Adjust ray angle based on player angle */

            /* Perform raycasting */
            int hit;
            float hitDistance = castRay(rayAngle, &hit);
            int lineHeight = calculateWallHeight(hitDistance, rayAngle);

            /* Scale and shift lineHeight */
            float scaledHeight = static_cast<float>(lineHeight) / (m_height-50);
            float distanceFactor = scaledHeight * scaledHeight;

            /* Dim the color based on distance while keeping a minimum blue intensity */
            unsigned char dimmedColor = hit == 0 || hit == 1 ? 0 : dim_color_with_min_blue(hit, distanceFactor);

            /* Select color based on lineHeight */
            int color = COLOR_VGA_MEDIUM_GRAY;

            /* Calculate the dimensions for the rectangle */
            int rectX = i * rectSize; /* X position */
            int rectWidth = MIN(rectSize, m_width - rectX); /* Adjust width for the last ray */
            int lineOffset = (m_height - lineHeight) / 2; /* Y position */
            int rectHeight = lineHeight; /* Height of the rectangle */
            i++;

            /* Draw the vertical rectangle for this ray */
            gfx_draw_rectangle_rgb(rectX, lineOffset, rectWidth, rectHeight, dimmedColor);
        }
    }

    unsigned char dim_color_with_min_blue(uint8_t baseColor, float distanceFactor) {
        /* Extract individual color components */
        int redComponent = (baseColor >> 5) & 0x07; /* Get the top 3 bits */
        int greenComponent = (baseColor >> 2) & 0x07; /* Get the next 3 bits */
        int blueComponent = baseColor & 0x03; /* Get the last 2 bits */

        /* Apply the distance factor to each color component */
        redComponent = (int)(redComponent * distanceFactor);
        greenComponent = (int)(greenComponent * distanceFactor);
        blueComponent = (int)(blueComponent * distanceFactor);

        /* Clamp components within their respective bit limits */
        redComponent = MIN(redComponent, 0x07);
        greenComponent = MIN(greenComponent, 0x07);
        blueComponent = MIN(blueComponent, 0x03);

        /* Recombine the color components */
        return (uint8_t)((redComponent << 5) | (greenComponent << 2) | blueComponent);
    }


    float castRay(int angle, int *hit) {
        /* Ray step size */
        float stepSize = 0.01; /* Small step size for ray marching */

        /* Start position (player's position) */
        float rayX = m_player.pos.x;
        float rayY = m_player.pos.y;

        /* Ray direction */
        float dirX = cos(FixAng(angle));
        float dirY = -sin(FixAng(angle));

        /* Ray marching */
        float distance;
        float distanceStep = (m_height * 0.0015);
        for (distance = 0; distance < m_height-25; distance += distanceStep) {
            int mapX = ((int)rayX);
            int mapY = ((int)rayY);

            /* Check if ray is out of bounds, one dimension */
            if (mapX < 0 || mapX >= 8 || mapY < 0 || mapY >= 8) {
                *hit = 0;
                break;
            }

            /* Check if ray hits a wall */
            if (m_map[mapY * 8 + mapX] != 0) { /* Assuming 1 represents a wall */
                *hit = m_map[mapY * 8 + mapX];
                break;
            }

            /* Move the ray forward */
            rayX += dirX * stepSize;
            rayY += dirY * stepSize;
        }

        return distance;
    }

    int calculateWallHeight(float distance, int angle) {

        int angleDiff = angle - m_player.angle;
        if (angleDiff > 180) {
            angleDiff -= 360;
        } else if (angleDiff < -180) {
            angleDiff += 360;
        }
        /* Correct the distance to avoid the fisheye effect */
        float correctedDistance = distance * cos_values[CLAMP(ABS(angleDiff), 0, 359)];

        /* Calculate wall height */
        int height = (int)(m_height-correctedDistance);

        return height;
    }

    int FixAng(int a){ if(a>359){ a-=360;} if(a<0){ a+=360;} return a;}
};

int main()
{
    Wolfstein t(200, 200);
    t.render();

    struct gfx_event e;
    while (1){
        gfx_get_event(&e, GFX_EVENT_BLOCKING); /* alt: GFX_EVENT_BLOCKING */
        switch (e.event)
        {
        case GFX_EVENT_RESOLUTION:
            /* update screensize */
            t.setSize(e.data, e.data2);
            t.render();
            break;
        case GFX_EVENT_EXIT:
            /* exit */
            return 0;
        case GFX_EVENT_KEYBOARD:
            /* keyboard event in e.data */
            t.input(e.data);
            break;
        case GFX_EVENT_MOUSE:
            /* mouse event in e.data and e.data2 */
            break;
        }

    }
    return 0;
}