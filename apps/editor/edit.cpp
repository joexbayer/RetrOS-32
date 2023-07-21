#include "edit.hpp"

/* Helper functions */
static int isAlpha(unsigned char c) {
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

static int nextNewline(unsigned char* str)
{
	unsigned char* begin = str;
	while (*begin != '\n' && *begin != 0)
		begin++;
	
	begin++;
	
	return begin - str;
}

static int prevNewline(unsigned char* str, unsigned char* limit)
{
	unsigned char* begin = str;
	while (*begin != '\n' && begin != limit)
		begin--;
	
	return str - begin;
}


void Editor::Reset()
{
	for (int i = 0; i < this->m_bufferSize; i++) this->m_textBuffer[i] = 0;
	gfx_draw_rectangle(0, 0, this->c_width, this->c_height, COLOR_BG);
	gfx_draw_line(0, 17, this->c_height, 17, COLOR_VGA_MEDIUM_GRAY);
	for (int i = 0; i < this->c_height/8; i++)gfx_draw_format_text(0, i*8, COLOR_VGA_MEDIUM_GRAY, "%s%d ", i < 10 ? " " : "", i);
}

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

	int line = 0;
	int col = 0;
	
	for (int i = from; i < to; i++){
		if(i > 0 && (!isAlpha(m_textBuffer[i-1]) || m_textBuffer[i-1] == ' ')){
			highlightSyntax(&m_textBuffer[i]);
		}

		drawChar(m_textBuffer[i], i == m_bufferEdit ? COLOR_VGA_LIGHT_GRAY : COLOR_BG);
		if(i == m_bufferEdit){
			line = m_y;
			col = m_x;
		}
	}
	
	gfx_draw_rectangle(24, c_height-8, c_width-24, 8, COLOR_BG);
	gfx_draw_format_text(24, c_height-8, COLOR_VGA_MEDIUM_DARK_GRAY, "W:%p Line:%p Col:%p\n", m_bufferHead, line, col);
}

void Editor::Lex()
{
	if(m_bufferHead > 0){
		program(vm_text, vm_data, (char*)m_textBuffer);
		gfx_draw_rectangle(24, c_height-8, c_width-24, 8, COLOR_BG);
		gfx_draw_format_text(24, c_height-8, COLOR_BLACK, "%d: %s\n", lex_get_error_line(), lex_get_error());
	}
}

void Editor::Quit()
{
	if(m_fd > 0){
		/* TODO: Check for unsaved changes */
		fclose(m_fd);
	}

	free(m_textBuffer);
	free(vm_data);
	free(vm_text);
	exit();
}

void Editor::Open(char* path)
{
	m_fd = open(path, FS_FLAG_CREATE);
	if(m_fd <= 0)
		return;
	
	setHeader(path);

	m_bufferHead = read(m_fd, m_textBuffer, 1000);
	m_fileSize = m_bufferHead;
	reDraw(0, m_bufferHead);
}

void Editor::Save()
{
	if(m_fd <= 0)
		return;
	
	write(m_fd, m_textBuffer, m_bufferSize);
}

void Editor::FileChooser()
{
	char filename[127];
	int i = 0;

	if(m_fd > 0){
		/* TODO: Check for unsaved changes */
		fclose(m_fd);
	}

	gfx_draw_rectangle(24, c_height-8, c_width-24, 8, COLOR_BG);
	gfx_draw_format_text(24, c_height-8, COLOR_VGA_YELLOW, "Open file: ");

	while (1){
		struct gfx_event event;
		gfx_get_event(&event, GFX_EVENT_BLOCKING);
		switch (event.event){
			case GFX_EVENT_KEYBOARD: {
				switch (event.data){
				case '\n':{
						filename[i] = 0;
						Reset();
						Open(filename);
						return;
					}
					break;
				case '\b':
					gfx_draw_rectangle(24 + (11*8) + (i*8), c_height-8, 8, 8, COLOR_BG);
					filename[i--] = 0;
					break;
				default:
					if(i == 127) return;
					filename[i++] = event.data;
					gfx_draw_char(24 + (11*8) + (i*8), c_height-8, event.data, COLOR_TEXT);
					break;
				}
			}
			break;
		case GFX_EVENT_RESOLUTION:
			c_width = event.data;
			c_height = event.data2;
			gfx_draw_rectangle(0, 0, c_width, c_height, COLOR_BG);
			gfx_draw_format_text(24, c_height-8, COLOR_VGA_YELLOW, "Open file: ");

			for (int j = 0; j < i; j++){
				gfx_draw_char(24 + (11*8) + (i*8), c_height-8, filename[i], COLOR_TEXT);
			}
			break;
		case GFX_EVENT_EXIT:
			Quit();
		default:
			break;
		}
	}
}

void Editor::EditorLoop()
{
	gfx_draw_rectangle(0, 0, 24, c_height, COLOR_BG);
	gfx_draw_line(0, 17, c_height, 17, COLOR_BG+2);
	for (int i = 0; i < c_height/8; i++)gfx_draw_format_text(0, i*8, COLOR_BG+4, "%s%d ", i < 10 ? " " : "", i);

	while (1){
		struct gfx_event event;
		gfx_get_event(&event, GFX_EVENT_BLOCKING);
		switch (event.event){
		case GFX_EVENT_KEYBOARD:
			putChar(event.data);
			if(event.data == '{'){
				putChar('}');
			}
			break;
		case GFX_EVENT_RESOLUTION:
			c_width = event.data;
			c_height = event.data2;
			gfx_draw_rectangle(0, 0, c_width, c_height, COLOR_BG);
			gfx_draw_line(0, 17, c_height, 17, COLOR_BG+2);
			for (int i = 0; i < c_height/8; i++)gfx_draw_format_text(0, i*8, COLOR_BG+4, "%s%d ", i < 10 ? " " : "", i);

			reDraw(0, m_bufferSize);
			break;;
		case GFX_EVENT_EXIT:
			Quit();
			return;
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
	if(m_x*8 > c_width || m_y*8 > c_height) return;

	if(c == '\n'){
		gfx_draw_rectangle(24 + m_x*8, m_y*8, c_width-(24 + m_x*8), 8, bg);
		//gfx_draw_char(24 + m_x*8, m_y*8, '\\', m_textColor);
		m_x = 0;
		m_y++;
	} else {
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, bg);
		gfx_draw_char(24 + m_x*8, m_y*8, c, m_textColor);
		m_x++;
		if(m_x >= ((c_width-24)/8)){
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
	setColor(COLOR_TEXT);
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
		if(m_bufferEdit < m_bufferHead-1){
			int diff = m_bufferHead-m_bufferEdit;
			int newline = m_textBuffer[m_bufferEdit-1] == '\n' ? 1 : 0;

			/* move all lines down */
			memcpy(&m_textBuffer[m_bufferEdit-1], &m_textBuffer[m_bufferEdit], diff+1);
			m_bufferHead--;
			m_bufferEdit--;

			line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			line_end = nextNewline(&m_textBuffer[m_bufferEdit]);

			if(newline){
				reDraw(m_bufferEdit-line_start+1, m_bufferSize);
				return;
			}

			reDraw(m_bufferEdit-(line_start+1), m_bufferEdit+line_end+1);
			return;
		}

		/* Append text to the end */
		m_bufferHead--;
		m_bufferEdit--;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		m_textBuffer[m_bufferEdit] = 0;

		reDraw(m_bufferEdit-(line_start+1), m_bufferEdit+line_end+1);
		return;
	case KEY_LEFT: /* LEFT */
		if(m_bufferEdit == 0) return;

		m_bufferEdit--;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end+2);
		return;
	case KEY_RIGHT: /* RIGHT */
		if(m_bufferEdit == m_bufferHead) return;

		m_bufferEdit++;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end+2);
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

			int moveto = prevNewline(&m_textBuffer[m_bufferEdit-1], m_textBuffer);
			m_bufferEdit -= moveto+1;

			line_end = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			reDraw(m_bufferEdit-line_end, m_bufferEdit+moveto+3);
		}
		return;
	case KEY_F3:
		Save();
		return;
	
	case KEY_F2:
		Lex();
		return;
	case KEY_F1:{
			FileChooser();
		}
		return;
	default: /* Default add character to buffer and draw it */
		if(c == 0) break;

		if(m_bufferEdit < m_bufferHead-1){
			int diff = m_bufferHead-m_bufferEdit;
			/* move all characters forward. */
			for (int i = 0; i < diff; i++){m_textBuffer[m_bufferHead-i] = m_textBuffer[m_bufferHead-i-1];}
		}

		m_textBuffer[m_bufferEdit] = c;
		m_bufferEdit++;
		m_bufferHead++;

		if(c == '\n'){
			reDraw(m_bufferEdit-1, m_bufferHead);
			break;
		}

		line_start = prevNewline(&m_textBuffer[m_bufferEdit-1], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end);
	}
}

int main(int argc, char* argv[])
{
	printf("argc: %d\n", argc);
	for (int i = 0; i < argc; i++){
		printf("argv: %s\n", argv[i]);
	}

	Editor s1;
	s1.FileChooser();
	s1.EditorLoop();

	printf("Done\n");
	return 0;
}