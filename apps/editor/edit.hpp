#ifndef D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF
#define D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF

#include <lib/printf.h>
#include <utils/Graphics.hpp>
#include <gfx/events.h>
#include <libc.h>
#include <colors.h>
#include <fs/ext.h>
#include <fs/fs.h>

#define COLOR_BG COLOR_VGA_LIGHTEST_GRAY
#define COLOR_TEXT COLOR_VGA_MEDIUM_DARK_GRAY
#define COLOR_MISC COLOR_VGA_MISC

#include <utils/TreeView.hpp>

#define TREE_VIEW_WIDTH 130  /* Width for the tree view panel */
#define HEADER_HEIGHT 10     /* Height for the header */


struct keyword {
	char word[10];
	color_t color;
};

class Editor : public Window {  
public:  
	Editor() : Window(288+TREE_VIEW_WIDTH, 248+12, "Editor", 1) {
		m_x = 0;
		m_y = 0;
		
		m_textBuffer = (unsigned char*) malloc(10*1024);

	
		vm_data = (char*) malloc((c_width/4)*(c_height/4));
		vm_text = (int*) malloc((c_width/4)*(c_height/4));

		m_bufferSize = 10*1024;
		for (int i = 0; i < m_bufferSize; i++) m_textBuffer[i] = 0;
		m_textBuffer[1] = '\n';
		m_bufferHead = 1;

		gfx_draw_rectangle(0, 0, 288+TREE_VIEW_WIDTH, c_height, COLOR_BG);

		/* Loading text middle of room */
		m_textColor = COLOR_TEXT;
		gfx_draw_format_text(20, 20, COLOR_VGA_MEDIUM_GRAY, "Loading...");

		treeView = new TreeView(0, 0, TREE_VIEW_WIDTH, 248+12);

		setColor(COLOR_TEXT);
		reDraw(0, 0);
		reDrawHeader();

	}

	~Editor() {
		free(m_textBuffer);
		//close(m_fd);
	}

	void Save();
	void Help();
	void Open(char* path);
	void putChar(unsigned char c);
	void Lex();
	void Quit();
	void SaveMsg();
	void setFd(int fd);
	
	void drawChar(unsigned char c, color_t bg);
	void EditorLoop();
	void FileChooser();
	void setColor(color_t color);
	void Reset();
	void reDrawHeader();

	void scroll(int);
	int countLines();

private:
	void highlightSyntax(unsigned char* start);

	TreeView* treeView;

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

	int scrollY = 0;

	color_t m_textColor;

	#define KEYWORD_TYPE COLOR_VGA_LIGHT_BLUE
	#define KEYWORD_SYS COLOR_VGA_PURPLE
	#define KEYWORD_BRANCH COLOR_VGA_RED
	#define KEYWORD_FUNC COLOR_VGA_MEDIUM_DARK_GRAY

	struct keyword keyWords[20] = {
		{"char", KEYWORD_TYPE},
		{"else", KEYWORD_BRANCH}, {"enum", KEYWORD_TYPE}, {"if", KEYWORD_BRANCH}, 
		{"int", KEYWORD_TYPE}, {"return", KEYWORD_SYS}, {"sizeof", KEYWORD_FUNC},
		{"while", KEYWORD_BRANCH}, {"open", KEYWORD_FUNC}, {"printf", KEYWORD_FUNC},
		{"malloc", KEYWORD_FUNC}, {"main", KEYWORD_FUNC},{"void", KEYWORD_TYPE}
	};

	int c_width = (288)-24;
	int c_height = (248)+12;

	void reDraw(int from, int to);
};

#endif /* D2830F9E_59AE_4E2A_B9AB_EB0EC93872EF */
