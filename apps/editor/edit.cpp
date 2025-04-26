/**
 * @file edit.cpp
 * @author Joe Bayer (joexbayer)
 * @brief A text editor app
 * @version 0.1
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "edit.hpp"
#include <utils/cppUtils.hpp>
#include <utils/Graphics.hpp>
#include <utils/MsgBox.hpp>

#define TEXT_WIDTH_OFFSET TREE_VIEW_WIDTH + 24
#define TEXT_SIDE_NUMBERS TREE_VIEW_WIDTH

#define HEADER_OFFSET 13
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


void Editor::reDrawHeader()
{
	gfx_draw_rectangle(TEXT_WIDTH_OFFSET-24, 0, this->c_width+24, this->c_height, COLOR_BG);

	gfx_draw_line(TEXT_SIDE_NUMBERS+17, 0, TEXT_SIDE_NUMBERS+17, this->c_height, 0);
	for (int i = 0; i < this->c_height/8; i++)gfx_draw_format_text(TEXT_SIDE_NUMBERS, HEADER_OFFSET+ i*8, COLOR_VGA_MEDIUM_GRAY, "%s%d ", i < 10 ? " " : "", i);

	drawHeaderTable(TREE_VIEW_WIDTH, c_width+24);
	gfx_draw_format_text(TREE_VIEW_WIDTH+2, 2, COLOR_BLACK, "< >");
	gfx_draw_format_text(TREE_VIEW_WIDTH+c_width+24-strlen("Help (F9)")*8, 2, COLOR_BLACK, "%s", "Help (F9)");
}


void Editor::scroll(int lines) {
	scrollY += lines;
	// Ensure that the scroll position stays within valid range
	if (scrollY < 0) scrollY = 0;

	gfx_draw_rectangle(TREE_VIEW_WIDTH, 0, 17, this->c_height, COLOR_BG);
	gfx_draw_line(TREE_VIEW_WIDTH+17, 0, TREE_VIEW_WIDTH+17, this->c_height, COLOR_VGA_MEDIUM_GRAY);
	for (int i = scrollY; i < (this->c_height/8) + scrollY; i++)gfx_draw_format_text(TEXT_SIDE_NUMBERS, HEADER_OFFSET+ (i-scrollY)*8, COLOR_VGA_MEDIUM_GRAY, "%s%d ", i < 10 ? " " : "", i);
}

void Editor::Reset()
{
	for (int i = 0; i < this->m_bufferSize; i++) this->m_textBuffer[i] = 0;
	this->m_textBuffer[1] = '\n';
	this->m_bufferHead = 2;
	reDrawHeader();

}

void Editor::reDraw(int from, int to) {
    /* Clamp from and to values */
    from = from < 0 ? 0 : from;
    to = to > m_bufferSize ? m_bufferSize : to;

	m_x = 0;
	m_y = 0;

    /* Calculate the number of characters scrolled */
    int linesScrolled = scrollY;
    int charsScrolled = 0;
    for (int i = 0; linesScrolled > 0; i++) {
        if (m_textBuffer[i] == '\n') {
            linesScrolled--;
        } 
        charsScrolled++;
    }

	if(from < charsScrolled){
		to = to + (charsScrolled-from)+1;
		from = charsScrolled;
	}

	printf("from: %d, to: %d\n", from, to);	

    /* Draw text in the range from 'from' to 'to' */
    int line = 0, col = 0;
    for (int i = from; i < to; i++) {
        if (i > 0 && (!isAlpha(m_textBuffer[i - 1]) || m_textBuffer[i - 1] == ' ')) {
            highlightSyntax(&m_textBuffer[i]);
        }
        drawChar(m_textBuffer[i], i == m_bufferEdit ? COLOR_VGA_LIGHT_GRAY : COLOR_BG);

        /* Update cursor position */
        if (i == m_bufferEdit) {
            line = m_y;
            col = m_x;
        }
    }

    /* Draw status bar */
    gfx_draw_rectangle(TREE_VIEW_WIDTH, c_height - 8, c_width - TEXT_WIDTH_OFFSET, 8, COLOR_BG);
    gfx_draw_format_text(TEXT_WIDTH_OFFSET, c_height - 8, COLOR_VGA_MEDIUM_DARK_GRAY, "W:%p/%p Line:%p Col:%p\n", m_bufferHead, m_bufferEdit, line, col);
}

void Editor::Lex()
{
}

void Editor::SaveMsg()
{
	MsgBox* msg = new MsgBox("Save changes?", "Save changes to file?", MSGBOX_YES_NO_CANCEL);
	MsgBoxResult ret = msg->show();
	if(ret == MSGBOX_RESULT_OK){
		Save();
	}
}

void Editor::Quit()
{
	SaveMsg();
	fclose(m_fd);

	free(m_textBuffer);
	free(vm_data);
	free(vm_text);
	exit();
}

void Editor::Open(char* path)
{
	if(m_fd > 0){
		SaveMsg();
		fclose(m_fd);
	}

	printf("Opening file: %s\n", path);
	m_fd = open(path, FS_FILE_FLAG_CREATE | FS_FILE_FLAG_READ | FS_FILE_FLAG_WRITE);
	if(m_fd < 0)
		return;
	
	
	setTitle(path);

	m_bufferHead = read(m_fd, m_textBuffer, m_bufferSize);
	if(m_bufferHead < 0){
		m_bufferHead = 0;
	}
	m_fileSize = m_bufferHead;
	reDraw(0, m_bufferHead);
}

void Editor::setFd(int fd){
	m_fd = fd;
}

void Editor::Save()
{
	if(m_fd < 0){
		return;
	}

	write(m_fd, m_textBuffer, m_bufferHead);
	gfx_draw_rectangle(TEXT_WIDTH_OFFSET, c_height-8, c_width-TEXT_WIDTH_OFFSET, 8, COLOR_BG);
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, c_height-8, COLOR_VGA_MEDIUM_DARK_GRAY, "Saved.");
}

void Editor::FileChooser()
{
	char filename[127];
	int i = 0;

	if(m_fd > 0){
		/* TODO: Check for unsaved changes */
		fclose(m_fd);

	}

	gfx_draw_rectangle(TEXT_WIDTH_OFFSET, c_height/2-4, c_width-TEXT_WIDTH_OFFSET, 8, COLOR_BG);
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, c_height/2-4, COLOR_BLACK, "Open file: ");

	treeView->drawTree(this);
	while (1){
		struct gfx_event event;
		gfx_get_event(&event, GFX_EVENT_BLOCKING);
		switch (event.event){
			case GFX_EVENT_KEYBOARD: {
				switch (event.data){

				case '\n':{
						filename[i] = 0;
						Open(filename);
						return;
					}
					break;
				case '\b':
					gfx_draw_rectangle(TEXT_WIDTH_OFFSET + (11*8) + (i*8), c_height/2-4, 8, 8, COLOR_BG);
					filename[i--] = 0;
					break;
				default:
					if(i == 127) return;
					filename[i++] = event.data;
					gfx_draw_char(TEXT_WIDTH_OFFSET + (11*8) + (i*8), c_height/2-4, event.data, COLOR_TEXT);
					break;
				}
			}
			break;
		case GFX_EVENT_RESOLUTION:
			c_width = event.data-TEXT_WIDTH_OFFSET-48;
			c_height = event.data2 % 8 == 0 ? event.data2 : event.data2 - (event.data2 % 8);
			reDrawHeader();
			break;
		case GFX_EVENT_MOUSE:
			printf("Mouse event\n");
			/* Check if tree view is clicked */
			if(event.data < TREE_VIEW_WIDTH){
				const char* file = treeView->click(event.data, event.data2);
				if(file != NULL){
					printf("Opening file: %s\n", file);
					Reset();
					Open((char*)file);
					return;
				}
			}
			break;
		case GFX_EVENT_EXIT:
			Quit();
		default:
			break;
		}
	}
}

void Editor::Help()
{
	reDrawHeader();
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET, COLOR_TEXT, "Help");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+8, COLOR_TEXT, "F1: Save");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+16, COLOR_TEXT, "F2: Scroll up");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+24, COLOR_TEXT, "F3: Scroll down");

	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+40, COLOR_TEXT, "Arrow keys: Move cursor");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+48, COLOR_TEXT, "Backspace: Delete character");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+56, COLOR_TEXT, "Enter: New line");
	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+64, COLOR_TEXT, "Mouse: Click on file to open");

	gfx_draw_format_text(TEXT_WIDTH_OFFSET, HEADER_OFFSET+80, COLOR_TEXT, "Press any 'q' to exit help");

	while (1){
		struct gfx_event event;
		gfx_get_event(&event, GFX_EVENT_BLOCKING);
		switch (event.event){
		case GFX_EVENT_KEYBOARD:
			if(event.data == 'q'){
				return;
			}
			break;
		case GFX_EVENT_MOUSE:
			if(event.data < TREE_VIEW_WIDTH){
				treeView->click(event.data, event.data2);
			}
			break;
		case GFX_EVENT_RESOLUTION:
			c_width = event.data-TEXT_WIDTH_OFFSET-48;
			c_height = event.data2 % 8 == 0 ? event.data2 : event.data2 - (event.data2 % 8);
		
			delete treeView;
			treeView = new TreeView(0, 0, TREE_VIEW_WIDTH, c_height);
			treeView->drawTree(this);

			reDrawHeader();
			break;;
		case GFX_EVENT_EXIT:
			Quit();
			return;
		default:
			break;
		}
	}
}

void Editor::EditorLoop()
{
	treeView->drawTree(this);
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
		case GFX_EVENT_MOUSE:
			printf("Mouse event\n");
			/* Check if tree view is clicked */
			if(event.data < TREE_VIEW_WIDTH){
				printf("Mouse event on treeview\n");
				const char* file = treeView->click(event.data, event.data2);
				if(file != nullptr){
					printf("Opening file: %s\n", file);
					Reset();
					Open((char*)file);
				}
			}
			break;
		case GFX_EVENT_RESOLUTION:
			c_width = event.data-TEXT_WIDTH_OFFSET-48;
			c_height = event.data2 % 8 == 0 ? event.data2 : event.data2 - (event.data2 % 8);

			delete treeView;
			treeView = new TreeView(0, 0, TREE_VIEW_WIDTH, c_height);
			treeView->drawTree(this);

			reDrawHeader();

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
		gfx_draw_rectangle(TEXT_WIDTH_OFFSET + m_x*8, HEADER_OFFSET+ m_y*8, c_width-(TEXT_WIDTH_OFFSET + m_x*8), 8, COLOR_BG);
		gfx_draw_rectangle(TEXT_WIDTH_OFFSET + m_x*8, HEADER_OFFSET+ m_y*8, 8, 8, bg);
		m_x = 0;
		m_y++;
	} else {
		gfx_draw_rectangle(TEXT_WIDTH_OFFSET + m_x*8,HEADER_OFFSET+ m_y*8, 8, 8, bg);
		gfx_draw_char(TEXT_WIDTH_OFFSET + m_x*8, HEADER_OFFSET+m_y*8, c, m_textColor);
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
	
	//gfx_draw_rectangle(TEXT_WIDTH_OFFSET + m_x*8, m_y*8, 8, 8, COLOR_BG);
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
				//reDraw(m_bufferEdit-line_start+1, m_bufferSize);
				reDraw(0, m_bufferSize);
				return;
			}

			//reDraw(m_bufferEdit-(line_start+1), m_bufferEdit+line_end+1);
			reDraw(0, m_bufferSize);
			return;
		}

		/* Append text to the end */
		m_bufferHead--;
		m_bufferEdit--;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		m_textBuffer[m_bufferEdit] = 0;

		//reDraw(m_bufferEdit-(line_start+1), m_bufferEdit+line_end+1);
		reDraw(0, m_bufferSize);
		return;
	case KEY_LEFT: /* LEFT */
		if(m_bufferEdit == 0) return;
		if(m_bufferEdit < 0){
			m_bufferEdit = 0;
			return;
		}

		m_bufferEdit--;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		//reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end+2);
		reDraw(0, m_bufferSize);
		return;
	case KEY_RIGHT: /* RIGHT */
		if(m_bufferEdit == m_bufferHead) return;
		if(m_bufferEdit > m_bufferHead) {
			m_bufferEdit = m_bufferHead;
			return;
		}

		m_bufferEdit++;

		line_start = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		//reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end+2);
		reDraw(0, m_bufferSize);
		return;
	case KEY_DOWN:{
			if(m_bufferEdit == m_bufferHead) break;
			if(m_bufferEdit > m_bufferHead) {
				m_bufferEdit = m_bufferHead;
				return;
			}

			int prev = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			int moveto = nextNewline(&m_textBuffer[m_bufferEdit-1]);
			m_bufferEdit += moveto;

			line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
			//reDraw(prev, m_bufferEdit+line_end);
			reDraw(0, m_bufferSize);
		}
		return;
	case KEY_UP: {
			if(m_bufferEdit <= 0) break;

			int moveto = prevNewline(&m_textBuffer[m_bufferEdit-1], m_textBuffer);
			m_bufferEdit -= moveto+1;

			line_end = prevNewline(&m_textBuffer[m_bufferEdit], m_textBuffer);
			//reDraw(m_bufferEdit-line_end+1, m_bufferEdit+moveto+3);
			reDraw(0, m_bufferSize);
		}
		return;
	case KEY_F3:
		scroll(1);
		reDraw(0, m_bufferSize);	
		return;
	case KEY_F2:
		scroll(-1);
		reDraw(0, m_bufferSize);	
		return;
	case KEY_F9:
		Help();
		reDraw(0, m_bufferSize);
		return;
	case KEY_F1:{
			Save();
		}
		return;
	default: /* Default add character to buffer and draw it */
		if(c == 0) break;

		if(m_bufferEdit < m_bufferHead){
			int diff = m_bufferHead-m_bufferEdit+1;
			/* move all characters in m_textBuffer forward */
			for (int i = 0; i < diff; i++){m_textBuffer[m_bufferHead-i] = m_textBuffer[m_bufferHead-i-1];}
		}

		m_textBuffer[m_bufferEdit] = c;
		m_bufferEdit++;
		m_bufferHead++;

		if(c == '\n'){
			//reDraw(m_bufferEdit-2, m_bufferHead);
			reDraw(0, m_bufferSize);
			break;
		}

		line_start = prevNewline(&m_textBuffer[m_bufferEdit-1], m_textBuffer);
		line_end = nextNewline(&m_textBuffer[m_bufferEdit]);
		//reDraw(m_bufferEdit-(line_start+2), m_bufferEdit+line_end);
		reDraw(0, m_bufferSize);
	}
}

extern "C" int main(int argc, char* argv[])
{
	printf("Starting editor...\n");
	printf("argc: %d\n", argc);
	for(int i = 0; i < argc; i++){
		printf("argv[%d]: %s\n", i, argv[i]);
	}

	Editor* s1 = new Editor();
	if(argc > 1){
		s1->Open(argv[1]);
	} else {
		s1->FileChooser();
	}
	s1->EditorLoop();
	while (1)
	{
		/* code */
	}
	

	printf("Done\n");
	return 0;
}