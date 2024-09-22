/**
 * @file Graphics.hpp
 * @author Joe Bayer (joexbayer)
 * @brief Graphics library for C++
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef __GRAPHICS_CPP_H
#define __GRAPHICS_CPP_H

#include <lib/graphics.h>
#include <colors.h>
#include <args.h>

#include <utils/Thread.hpp>

class Window {
public:
	Window(int width, int height, const char* name, int flags) {
		gfx_create_window(width, height, flags);
		gfx_set_title(name);
	}

    void drawRect(int x, int y, int width, int height, unsigned char color)
    {
        gfx_draw_rectangle(x, y, width, height, color);
    }

    void drawCircle(int x, int y, int r, unsigned char color, char fill)
    {
        gfx_draw_circle(x, y, r, color, fill);
    }

    void drawLine(int x0, int y0, int x1, int y1, unsigned char color)
    {
        gfx_draw_line(x0, y0, x1, y1, color);
    }

    void drawText(int x, int y, const char* text, unsigned char color)
    {
        gfx_draw_text(x, y, text, color);
    }

    void drawChar(int x, int y, char data, unsigned char color)
    {
        gfx_draw_char(x, y, data, color);
    }

    void drawHeaderTable(int start, int width)
    {
        drawRect(start, 0, width, 10, 30);

        drawRect(start+1, 1, width-1, 1, 30+1);
        drawRect(start+1, 1, 1, 10, 30+1);

        drawRect(start+1+width-2, 1, 1, 10, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        drawRect(start+1, 10, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5); 
        drawRect(start+1, 11, width-1, 1, COLOR_BLACK);
        drawRect(start+1, 12, width-1, 1, 30+1); 
    }

    void drawContouredRect(int x, int y, int width, int height) {
        drawRect(x, y, width, height, 30);
        
        // Top Inner Border
        drawRect(x, y, width-1, 1, 31);
        
        // Left Inner Border
        drawRect(x, y, 1, height, 31);
        
        // Right Inner Border
        drawRect(x+width-1, y, 1, height, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        
        // Bottom Inner Borders
        drawRect(x, y+height-1, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        drawRect(x, y+height, width-1, 1, 31);
    }

    void drawContouredBox(int x, int y, int width, int height, color_t color) {
        drawRect(x, y, width, height, color);
        
        // Top Inner Border
        drawRect(x, y, width-1, 1, 31);
        
        // Left Inner Border
        drawRect(x, y, 1, height, 31);
        
        // Right Inner Border
        drawRect(x+width-1, y, 1, height, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        
        // Bottom Inner Borders
        drawRect(x, y+height-1, width-1, 1, COLOR_VGA_MEDIUM_DARK_GRAY+5);
        drawRect(x, y+height, width-1, 1, 31);
    }

    void drawFormatText(int x, int y, unsigned char color, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        gfx_draw_format_text(x, y, color, fmt, args);
        va_end(args);
    }

    void drawPixel(int x, int y, unsigned char color)
    {
        gfx_draw_pixel(x, y, color);
    }
	
	void setTitle(char* name){
		gfx_set_title(name);
	}

    void setHeader(const char* header)
    {
        gfx_set_header(header);
    }

private:
	
};

#endif // __GRAPHICS!



