#ifndef D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF
#define D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF

#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include "../../interp/lex.h"
#include <util.h>
#include <colors.h>
#include <fs/ext.h>

#define COLOR_BG COLOR_VGA_LIGHTEST_GRAY
#define COLOR_TEXT COLOR_VGA_MEDIUM_DARK_GRAY
#define COLOR_MISC COLOR_VGA_MISC

struct keyword {
	char word[10];
	color_t color;
};

class Editor : public Window {  
public:  
	Editor() : Window(288, 248+12, "Editor", 1) {
		m_x = 0;
		m_y = 0;
		
		m_textBuffer = (unsigned char*) malloc((c_width/8)*(c_height/8));
	
		vm_data = (char*) malloc((c_width/4)*(c_height/4));
		vm_text = (int*) malloc((c_width/4)*(c_height/4));

		m_bufferSize = (c_width/8)*(c_height/8);
		for (int i = 0; i < m_bufferSize; i++) m_textBuffer[i] = 0;
		m_textBuffer[1] = '\n';
		m_bufferHead = 1;

		gfx_draw_rectangle(0, 0, c_width+24, c_height, COLOR_BG);
		gfx_draw_line(0, 17, c_height, 17, COLOR_BG+2);
		for (int i = 0; i < c_height/8; i++)gfx_draw_format_text(0, i*8, COLOR_BG+4, "%s%d ", i < 10 ? " " : "", i);

		setColor(COLOR_TEXT);
		reDraw(0, 0);

	}

	~Editor() {
		free(m_textBuffer);
		//close(m_fd);
	}

	void Save();
	void Open(char* path);
	void putChar(unsigned char c);
	void Lex();
	void Quit();
	void setFd(int fd);
	
	void drawChar(unsigned char c, color_t bg);
	void EditorLoop();
	void FileChooser();
	void setColor(color_t color);
	void Reset();
	void reDrawHeader();

private:
	void highlightSyntax(unsigned char* start);

	int m_fd = -1;
	unsigned char* m_textBuffer;
	int m_bufferSize = 0;
	int m_fileSize = 0;
	int m_bufferHead = 1;
	int m_bufferEdit = 0;
	int m_x, m_y;
	int m_saved;

	int* vm_text;
	char* vm_data;

	color_t m_textColor;

	#define KEYWORD_TYPE COLOR_VGA_LIGHT_BLUE
	#define KEYWORD_SYS COLOR_VGA_PURPLE
	#define KEYWORD_BRANCH COLOR_VGA_RED
	#define KEYWORD_FUNC COLOR_VGA_YELLOW

	struct keyword keyWords[20] = {
		{"char", KEYWORD_TYPE},
		{"else", KEYWORD_BRANCH}, {"enum", KEYWORD_TYPE}, {"if", KEYWORD_BRANCH}, 
		{"int", KEYWORD_TYPE}, {"return", KEYWORD_SYS}, {"sizeof", KEYWORD_FUNC},
		{"while", KEYWORD_BRANCH}, {"open", KEYWORD_FUNC}, {"printf", KEYWORD_FUNC},
		{"malloc", KEYWORD_FUNC}, {"main", KEYWORD_FUNC},{"void", KEYWORD_TYPE}
	};

	/* Size is based on the fact that our filesystem can only handle 8kb files */
	int c_width = (288)-24;
	int c_height = (248)+12;

	void reDraw(int from, int to);
};

#endif /* D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF */
