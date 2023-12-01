#include <utils/Graphics.hpp>
#include <util.h>
#include <colors.h>
#include <math.h>

#define MAP_WIDTH 24
#define MAP_HEIGHT 24
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 200

// Simple map: 1 represents walls, 0 represents empty space
int worldMap[MAP_WIDTH][MAP_HEIGHT] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};


class FrameRenderer : public Window {
public:
    FrameRenderer(int width, int height) : Window(width, height, "Frame", 1) {
        this->width = width;
        this->height = height;
        // Initialize player position and direction
        posX = 22.0; posY = 12.0; dirX = -1.0; dirY = 0.0;
        planeX = 0.0; planeY = 0.66;
    }

    void renderFrame() {
        gfx_draw_rectangle(0, 0, width, height, COLOR_VGA_LIGHTEST_GRAY);

        for (int x = 0; x < width; x++) {
            // Calculate ray position and direction
            double cameraX = 2 * x / double(width) - 1;
            double rayDirX = dirX + planeX * cameraX;
            double rayDirY = dirY + planeY * cameraX;

            // Which box of the map we're in
            int mapX = int(posX);
            int mapY = int(posY);

            // Length of ray from current position to next x or y-side
            double sideDistX;
            double sideDistY;

            // Length of ray from one x or y-side to next x or y-side
            double deltaDistX = (rayDirX == 0) ? 1e30 : ABS(1 / rayDirX);
            double deltaDistY = (rayDirY == 0) ? 1e30 : ABS(1 / rayDirY);
            double perpWallDist;

            // What direction to step in x or y-direction (either +1 or -1)
            int stepX;
            int stepY;

            int hit = 0; // Was there a wall hit?
            int side; // Was a NS or a EW wall hit?

            // Calculate step and initial sideDist
            if (rayDirX < 0) {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            } else {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0) {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            } else {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

           // Perform DDA (Digital Differential Analysis)
            while (hit == 0) {
                // Jump to next map square, OR in x-direction, OR in y-direction
                if (sideDistX < sideDistY) {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                } else {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                // Clamp mapX and mapY to prevent out-of-bounds access
                if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) {
                    hit = 1;  // Assume hit if out of bounds
                    continue;
                }

                // Check if ray has hit a wall
                if (worldMap[mapX][mapY] > 0) hit = 1;
            }


            // Calculate distance projected on camera direction
            if (side == 0) perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
            else           perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;

            // Calculate height of line to draw on screen
            int lineHeight = int(height / perpWallDist);

            // calculate lowest and highest pixel to fill in current stripe
            int drawStart = -lineHeight / 2 + height / 2;
            if(drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + height / 2;
            if(drawEnd >= height) drawEnd = height - 1;

            // Choose wall color
            unsigned char color = COLOR_VGA_DARKEST_GRAY; // Shade of wall color

            // Draw the pixels of the stripe as a vertical line
            gfx_draw_line(drawEnd, x, drawStart, x, color);
        }
    }

private:
    double posX, posY, dirX, dirY, planeX, planeY;
    int width, height;
};

int main() {
    FrameRenderer renderer(SCREEN_WIDTH, SCREEN_HEIGHT);
    renderer.renderFrame();

    while(1);

    return 0;
}
