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
        /* draw map */
        gfx_draw_rectangle(0, 0, m_width, m_height, 0xf);

        drawRays();

        // for (int y = 0; y < 8; y++){
        //     for (int x = 0; x < 8; x++){
        //         if (m_map[y*8+x] == 1){
        //             gfx_draw_rectangle(x*10, y*10, 10, 10, COLOR_VGA_DARKEST_GRAY);
        //         }
        //     }
        // }

        /* draw player */
        //gfx_draw_rectangle(m_player.pos.x*10, m_player.pos.y*10, 10, 10, COLOR_VGA_GREEN);

        /* draw player direction */
        // gfx_draw_line(m_player.pos.x*10, m_player.pos.y*10, m_player.pos.x*10 + m_player.dir.x*10, m_player.pos.y*10 + m_player.dir.y*10, COLOR_VGA_RED);

        // /* draw rays */
        // for(int i = -30; i < 30; i++){
        //     float dirX = cos(FixAng(m_player.angle + i));
        //     float dirY = -sin(FixAng(m_player.angle + i));
        //     gfx_draw_line(m_player.pos.x*10, m_player.pos.y*10, m_player.pos.x*10 + dirX*10, m_player.pos.y*10 + dirY*10, COLOR_VGA_GREEN);
        // }

        // gfx_draw_format_text(0, 100, COLOR_BLACK, "x: %d, y: %d, angle: %d", (int)m_player.pos.x, (int)m_player.pos.y, (int)m_player.angle);

    }

    /* input */
    void input(char key){
        switch (key){
        case 'a':{
                m_player.angle -= 6;
                m_player.angle = FixAng(m_player.angle);
                m_player.dir.x = cos(m_player.angle);
                m_player.dir.y = -sin(m_player.angle);
            }
            break;
        case 'd':{
                m_player.angle += 6;
                m_player.angle = FixAng(m_player.angle);

                m_player.dir.x = cos(m_player.angle);
                m_player.dir.y = -sin(m_player.angle);
            }
            break;
        case 'w':{
                m_player.pos.x += m_player.dir.x * 0.2;
                m_player.pos.y += m_player.dir.y * 0.2;
            }
            break;
        case 's':{
                m_player.pos.x -= m_player.dir.x * 0.2;
                m_player.pos.y -= m_player.dir.y * 0.2;
            }
            break;
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

    float cos(int angle){
        return cos_values[CLAMP(angle, 0, 359)];
        // int index = clamp_angle(angle) / 6;
        // return cos_60[index]; 
    }

    float sin(int angle){
        return sin_values[CLAMP(angle, 0, 359)];
        // int index = clamp_angle(angle) / 6;
        // return sin_60[index];
    }

    char m_map[8*8] = {
        1,1,1,1,1,1,1,1,
        1,0,1,0,0,0,0,1,
        1,0,1,0,0,0,0,1,
        1,0,1,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,1,0,1,
        1,0,0,0,0,0,0,1,
        1,1,1,1,1,1,1,1,	
    };

    void drawRays() {
        int rays = 60;
        int i = 0;
        int rectSize = (m_width + rays - 1) / rays; // Adjust to ensure full coverage

        for (int rayIndex = -30; rayIndex < 30; rayIndex++) {
            /* Calculate ray's angle */
            int rayAngle = FixAng(m_player.angle + rayIndex); /* Adjust ray angle based on player angle */

            /* Perform raycasting */
            float hitDistance = castRay(rayAngle);
            int lineHeight = calculateWallHeight(hitDistance, rayAngle);

            float normalizedHeight = static_cast<float>(lineHeight) / (m_height-25);

            int colorIntensity = static_cast<int>(17 + normalizedHeight * (31 - 17));   

            colorIntensity = MIN(MAX(colorIntensity, 17), 31);

            // Select color based on lineHeight
            int color = COLOR_VGA_MEDIUM_GRAY;

            // Calculate the dimensions for the rectangle
            int rectX = i * rectSize; // X position
            int rectWidth = MIN(rectSize, m_width - rectX); // Adjust width for the last ray
            int lineOffset = (m_height - lineHeight) / 2; // Y position
            int rectHeight = lineHeight; // Height of the rectangle
            i++;

            // Draw the vertical rectangle for this ray
            gfx_draw_rectangle(rectX, lineOffset, rectWidth, rectHeight, colorIntensity);
        }

    }

    float castRay(int angle) {
        /* Constants */
        const float MAX_DISTANCE = m_height-25; /* Maximum raycast distance */

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
        for (distance = 0; distance < MAX_DISTANCE; distance += distanceStep) {
            int mapX = ((int)rayX);
            int mapY = ((int)rayY);

            /* Check if ray is out of bounds, one dimension */
            if (mapX < 0 || mapX >= 8 || mapY < 0 || mapY >= 8) {
                break;
            }

            /* Check if ray hits a wall */
            if (m_map[mapY * 8 + mapX] == 1) { /* Assuming 1 represents a wall */
                break;
            }

            /* Move the ray forward */
            rayX += dirX * stepSize;
            rayY += dirY * stepSize;
        }

        return distance;
    }

    int calculateWallHeight(float distance, int angle) {
        int angleDiff = FixAng(angle - m_player.angle);

        // Correct the distance to avoid the fisheye effect
        // Convert angle difference to radians for cosine calculation
        float correctedDistance = distance * cos(angleDiff);

        /* Calculate wall height */
        int height =  (int)(m_height-correctedDistance);

        return height;
    }


    int clamp_angle(int angle) {
        angle = angle % 360;
        if (angle < 0) {
            angle += 360;
        }

        /* Round to nearest multiple of 6 */
        int nearest_multiple = (int)((angle + 3) / 6) * 6; /* Adding 3 for rounding to nearest multiple */

        /* Ensure the result is again within [0, 360) */
        nearest_multiple = nearest_multiple % 360;

        return nearest_multiple;
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
            t.render();
            break;
        case GFX_EVENT_MOUSE:
            /* mouse event in e.data and e.data2 */
            break;
        }

    }
    return 0;
}