#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <util.h>
#include <colors.h>

#define COLOR_BG COLOR_VGA_BG
#define COLOR_TEXT COLOR_VGA_FG
#define COLOR_MISC COLOR_VGA_MISC

class Editor : public Window {  
public:  
	Editor() : Window(288, 248, "Editor") {
		m_x = 0;
		m_y = 0;
		m_textBuffer = (unsigned char*) malloc((c_width/8)*(c_height/8));
		m_bufferSize = (c_width/8)*(c_height/8);
		for (int i = 0; i < m_bufferSize; i++) m_textBuffer[i] = 0;
		setColor(COLOR_TEXT);
		reDraw();

	}

	~Editor() {
		free(m_textBuffer);
		//close(m_fd);
	}

	void Save();
	void Open(char* path);
	void putChar(unsigned char c);
	void drawChar(unsigned char c);
	void EditorLoop();
	void setColor(color_t color);

private:
	void highlightSyntax(unsigned char c);

	int m_fd = -1;
	unsigned char* m_textBuffer;
	int m_bufferSize = 0;
	int m_bufferHead = 0;
	int m_bufferEdit = -1;
	int m_x, m_y;

	color_t m_textColor;

	/* Size is based on the fact that our filesystem can only handle 8kb files */
	const int c_width = 280-24;
	const int c_height = 240;

	void reDraw();
};

void Editor::reDraw()
{	
	SAVE_AND_RESTORE(m_x, {

		SAVE_AND_RESTORE(m_y, {
			m_x = 0;
			m_y = 0;
			gfx_draw_rectangle(0, 0, 288, 248, COLOR_BG);
			gfx_draw_line(0, 17, 248, 17, COLOR_BG+2);
			for (int i = 0; i < 248; i++)gfx_draw_format_text(0, i*8, COLOR_BG+4, "%s%d ", i < 10 ? " " : "", i);

			for (int i = 0; i < m_bufferSize; i++){
				drawChar(m_textBuffer[i]);
			}
		});
	});

	gfx_draw_char(24 + m_x*8, m_y*8, '_', m_textColor);
}

void Editor::Open(char* path)
{
	m_fd = open(path);
	if(m_fd <= 0)
		return;
	
	read(m_fd, m_textBuffer, (c_width/8)*(c_height/8));
	for (int i = 0; i < m_bufferSize; i++) putChar(m_textBuffer[i]);
	reDraw();
}


void Editor::EditorLoop()
{
	while (1){
		struct gfx_event event;
		gfx_get_event(&event);
		switch (event.event){
		case GFX_EVENT_KEYBOARD:
			putChar(event.data);
			break;
		default:
			break;
		}
	}
}

void Editor::setColor(color_t color)
{
	m_textColor = color;
}

void Editor::drawChar(unsigned char c)
{
	gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BG);

	switch (c){
	default: /* Default add character to buffer and draw it */
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BG);
		gfx_draw_char(24 + m_x*8, m_y*8, c == '\n' ? '\\' : c, m_textColor);
		m_x++;
		if(m_x == (c_width/8)){
			m_x = 0;
			m_y++;
		}
		break;
	}
}

void Editor::highlightSyntax(unsigned char c)
{
	/* Set different colors for different syntax elements */
	switch (c) {
		case '/':
			/* Highlight preprocessor directives */
			setColor(COLOR_MISC);
			break;
		case '"':
		case '\'':
			/* Highlight string literals and character literals */
			setColor(COLOR_MISC);
			break;
		default:
			setColor(COLOR_TEXT);
			break;
	}
}

void Editor::putChar(unsigned char c)
{
	//gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BG);

	switch (c){
	case '\n':
		m_textBuffer[m_y*(c_width/8) + m_x] = c;
		highlightSyntax(c);
		drawChar(c);
		m_x = 0;
		m_y++;
		break;
	case '\b':
		m_x--;
		if(m_x - 1 < 0){
			if(m_y != 0)
				m_y--;
			m_x = (c_width/8);
		}
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BG);
		m_textBuffer[m_y*(c_width/8) + m_x] = 0;
		reDraw();
		return;
	case 252:
		m_x--;
		if(m_x - 1 < 0){
			if(m_y != 0)
				m_y--;
			m_x = (c_width/8);
		}
		reDraw();
		return;
	case 251:
		drawChar(m_textBuffer[m_y*(c_width/8) + m_x]);
		m_x++;
		if(m_x == (c_width/8)){
			m_x = 0;
			m_y++;
		}
		reDraw();
		return;
	case 253:
		m_y++;
		reDraw();
		return;
	case 254:
		m_y--;
		reDraw();
		return;
	default: /* Default add character to buffer and draw it */
		m_textBuffer[m_y*(c_width/8) + m_x] = c;
		highlightSyntax(c);
		drawChar(c);
	}
	gfx_draw_char(24 + m_x*8, m_y*8, '_', m_textColor);
}

int main()
{
	//for (int i = 0; i < argc; i++){
	//	printf("argv: %s\n", argv[i]);
	//}
	

    Editor s1;
	//s1.Open("/home");
	s1.EditorLoop();

	printf("Done\n");
	return 0;
}