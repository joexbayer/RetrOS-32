#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <util.h>
#include <colors.h>

#define COLOR_BG COLOR_VGA_BG
#define COLOR_TEXT COLOR_VGA_FG
#define COLOR_MISC COLOR_VGA_MISC

struct keyword {
	char word[10];
	color_t color;
};

int isAlpha(unsigned char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

int nextNewline(unsigned char* str)
{
	unsigned char* begin = str;
	while (*begin != '\n' && *begin != 0)
		begin++;
	
	while (*begin == '\n')
		begin++;
	
	return begin - str;
}

int prevNewline(unsigned char* str, unsigned char* limit)
{
	unsigned char* begin = str;
	while (*begin != '\n' && *begin != 0 && begin != limit)
		begin--;
	
	while (*begin == '\n' && begin != limit)
		begin--;
	
	return str - begin;
}

class Editor : public Window {  
public:  
	Editor() : Window(288, 248, "Editor") {
		m_x = 0;
		m_y = 0;
		m_textBuffer = (unsigned char*) malloc((c_width/8)*(c_height/8));
		m_bufferSize = (c_width/8)*(c_height/8);
		for (int i = 0; i < m_bufferSize; i++) m_textBuffer[i] = 0;
		gfx_draw_rectangle(0, 0, 288, 248, COLOR_BG);
		gfx_draw_line(0, 17, 248, 17, COLOR_BG+2);
		for (int i = 0; i < 248; i++)gfx_draw_format_text(0, i*8, COLOR_BG+4, "%s%d ", i < 10 ? " " : "", i);

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
	
	void drawChar(unsigned char c, color_t bg);
	void EditorLoop();
	void setColor(color_t color);

private:
	void highlightSyntax(unsigned char* start);

	int m_fd = -1;
	unsigned char* m_textBuffer;
	int m_bufferSize = 0;
	int m_fileSize = 0;
	int m_bufferHead = 1;
	int m_bufferEdit = 0;
	int m_x, m_y;

	color_t m_textColor;

	#define KEYWORD_TYPE COLOR_VGA_MISC
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
	const int c_width = 280-24;
	const int c_height = 240;

	void reDraw(int from, int to);
};

void Editor::reDraw(int from, int to)
{	
	m_x = 0;
	m_y = 0;
	from = from <= 0 ? 0 : from;
	/* Skip the unchanged chars */
	for (int i = 0; i < from; i++){
		m_x++;
		if(m_x > (c_width/8)){
			m_x = 0;
			m_y++;
		}
		if(m_textBuffer[i] == '\n'){
			m_x = 0;
			m_y++;
		}
	}
	
	for (int i = from; i < to; i++){
		if(i > 0 && (!isAlpha(m_textBuffer[i-1]) || m_textBuffer[i-1] == ' ')){
			highlightSyntax(&m_textBuffer[i]);
		}

		drawChar(m_textBuffer[i], i == m_bufferEdit ? COLOR_VGA_BG+5 : COLOR_BG);
	}
}

void Editor::Open(char* path)
{
	m_fd = open(path);
	if(m_fd <= 0)
		return;
	
	m_bufferHead = read(m_fd, m_textBuffer, (c_width/8)*(c_height/8));
	m_fileSize = m_bufferHead;
	reDraw(0, m_bufferHead);
}

void Editor::Save()
{
	if(m_fd <= 0)
		return;
	
	write(m_fd, m_textBuffer, m_bufferSize);
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

void Editor::drawChar(unsigned char c, color_t bg)
{
	if(c == '\n'){
		gfx_draw_rectangle(24 + m_x*8, m_y*8, c_width-(24 + m_x*8), 8, bg);
		m_x = 0;
		m_y++;
	} else {
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, bg);
		gfx_draw_char(24 + m_x*8, m_y*8, c, m_textColor);
		m_x++;
		if(m_x > (c_width/8)){
			m_x = 0;
			m_y++;
		}
	}

}

void Editor::highlightSyntax(unsigned char* start)
{
	unsigned char* begin = start;
	/* look ahead */
	while(isAlpha(*begin) && *begin != ' ')
		begin++;
	
	struct keyword* key;
	for (int i = 0; i < 20; i++){
		key = &keyWords[i];
		if(key->color == 0)
			break;

		if(memcmp(start , key->word, strlen(key->word)) == 0){
			setColor(key->color);
			return;
		}
	}
	setColor(COLOR_VGA_FG);
}

void Editor::putChar(unsigned char c)
{
	//gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BG);
	int line_start;
	int line_end;
	switch (c){
	case '\b':
		if(m_bufferEdit == 0) return;

		/* Insert text in the middle. */
		if(m_bufferEdit < m_bufferHead){
			int diff = m_bufferHead-m_bufferEdit;
			int newline = m_textBuffer[m_bufferEdit-1] == '\n' ? 1 : 0;

			/* move all lines down */
			memcpy(&m_textBuffer[m_bufferEdit-1], &m_textBuffer[m_bufferEdit], diff+1);
			m_bufferHead--;
			m_bufferEdit--;

			if(newline){
				reDraw(m_bufferEdit-line_start+1, m_bufferSize);
				return;
			}

			line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			line_end = nextNewline(&m_textBuffer[m_bufferEdit]);

			reDraw(m_bufferEdit-line_start+1, m_bufferEdit+line_end);
			return;
		}

		/* Append text to the end */
		m_bufferHead--;
		m_bufferEdit--;
		m_textBuffer[m_bufferHead] = 0;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);

		reDraw(m_bufferEdit-line_start+1, m_bufferEdit+line_end);
		return;
	case KEY_LEFT: /* LEFT */
		if(m_bufferEdit == 0) return;

		m_bufferEdit--;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end);
		return;
	case KEY_RIGHT: /* RIGHT */
		if(m_bufferEdit == m_bufferHead) return;

		m_bufferEdit++;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-line_start-1, m_bufferEdit+line_end);
		return;
	case KEY_DOWN:{
			if(m_bufferEdit == m_bufferHead) break;

			int moveto = nextNewline(&m_textBuffer[m_bufferEdit]);
			m_bufferEdit += moveto;

			line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
			reDraw(m_bufferEdit-moveto, m_bufferEdit+line_end);
		}
		return;
	case KEY_UP: {
			if(m_bufferEdit <= 0) break;

			int moveto = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			m_bufferEdit -= moveto;

			line_end = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			reDraw(m_bufferEdit-line_end, m_bufferEdit+moveto+1);
		}
		return;
	case KEY_F3:
		Save();
		return;
	case KEY_F1:{
			Open("add.c");
		}
		return;
	default: /* Default add character to buffer and draw it */
		if(c == 0) break;

		if(m_bufferEdit < m_bufferHead){
			int diff = m_bufferHead-m_bufferEdit;
			/* move all characters forward. */
			for (int i = 0; i < diff; i++){m_textBuffer[m_bufferHead-i] = m_textBuffer[m_bufferHead-i-1];}
		}

		m_textBuffer[m_bufferEdit++] = c;
		m_bufferHead++;
		//highlightSyntax(c);
		//drawChar(c);
		//m_x++;
		if(c == '\n'){
			reDraw(m_bufferEdit-1, m_bufferHead);
			break;
		}

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-(line_start+1), m_bufferEdit+line_end+1);
	}
}

int main()
{
	//for (int i = 0; i < argc; i++){
	//	printf("argv: %s\n", argv[i]);
	//}
	Editor s1;
	s1.EditorLoop();

	printf("Done\n");
	return 0;
}