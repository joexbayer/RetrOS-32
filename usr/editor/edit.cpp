#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <util.h>
#include <colors.h>

class Editor : public Window {  
public:  
	Editor() : Window(288, 248, "Editor") {
		m_x = 0;
		m_y = 0;
		m_textBuffer = (unsigned char*) malloc((c_width/8)*(c_height/8));
		m_bufferSize = (c_width/8)*(c_height/8);
		for (int i = 0; i < m_bufferSize; i++) m_textBuffer[i] = 0;

	}

	~Editor() {
		free(m_textBuffer);
		close(m_fd);
	}

	void Save();
	void Open(char* path);
	void putChar(unsigned char c);
	void drawChar(unsigned char c);
	void EditorLoop();

private:
	int m_fd = -1;
	unsigned char* m_textBuffer;
	int m_bufferSize = 0;
	int m_bufferHead = 0;
	int m_bufferEdit = -1;
	int m_x, m_y;

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
			gfx_draw_rectangle(0, 0, 288, 248, COLOR_BOX_LIGHT_FG);
			gfx_draw_line(0, 17, 248, 17, 0xff);
			for (int i = 0; i < 248; i++)gfx_draw_format_text(0, i*8, 0xff, "%s%d ", i < 10 ? " " : "", i);

			for (int i = 0; i < m_bufferHead; i++)drawChar(m_textBuffer[i]);

			gfx_draw_char(24 + m_x*8, m_y*8, '_', COLOR_BOX_BG);
		
			gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);
		});
	});

	gfx_draw_char(24 + m_x*8, m_y*8, '_', COLOR_BOX_BG);
}

void Editor::Open(char* path)
{
	m_fd = open(path);
	if(m_fd <= 0)
		return;
	
	read(m_fd, m_textBuffer, (c_width/8)*(c_height/8));
	for (int i = 0; i < m_bufferSize; i++) putChar(m_textBuffer[i]);
	
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

void Editor::drawChar(unsigned char c)
{
	gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);

	switch (c){
	case '\n':
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);
		gfx_draw_char(24 + m_x*8, m_y*8, '\\', COLOR_BOX_BG);
		m_x = 0;
		m_y++;
		break;
	case 252:
		gfx_draw_rectangle(24 + (m_x+1)*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);
		gfx_draw_char(24 + (m_x+1)*8, m_y*8, m_textBuffer[m_bufferEdit+2], COLOR_BOX_BG);

		gfx_draw_char(24 + m_x*8, m_y*8, m_textBuffer[m_bufferEdit+1], COLOR_BOX_BG);
		break;
	case 251:
		gfx_draw_rectangle(24 + (m_x-1)*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);
		gfx_draw_char(24 + (m_x-1)*8, m_y*8, m_textBuffer[m_bufferEdit], COLOR_BOX_BG);
		gfx_draw_char(24 + (m_x)*8, m_y*8, m_textBuffer[m_bufferEdit+1], COLOR_BOX_BG);
		break;
	default: /* Default add character to buffer and draw it */
		gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);
		gfx_draw_char(24 + m_x*8, m_y*8, c, COLOR_BOX_BG);
		m_x++;
		if(m_x > (c_width/8)){
			m_x = 0;
			m_y++;
		}
		break;
	}

	gfx_draw_char(24 + m_x*8, m_y*8, '_', COLOR_BOX_BG);
}

void Editor::putChar(unsigned char c)
{
	//gfx_draw_rectangle(24 + m_x*8, m_y*8, 8, 8, COLOR_BOX_LIGHT_FG);

	switch (c){
	case '\n':
		m_textBuffer[m_bufferHead++] = c;
		m_bufferEdit++;
		break;
	case '\b':
		memcpy(&m_textBuffer[m_bufferEdit], &m_textBuffer[m_bufferEdit]+1, m_bufferHead - m_bufferEdit);
		m_bufferHead--;
		reDraw();
		return;
	case 252:
		m_bufferEdit--;
		m_x--;
		if(m_x - 1 < 0){
			m_y--;
			m_x = (c_width/8);
		}
		break;
	
	case 251:
		if(m_bufferEdit == m_bufferHead-1) break;
		m_bufferEdit++;
		m_x++;
		if(m_x > (c_width/8)){
			m_x = 0;
			m_y++;
		}

		break;
	case 0:
		return;
	default: /* Default add character to buffer and draw it */
		m_textBuffer[m_bufferHead++] = c;
		m_bufferEdit++;
	}

	drawChar(c);
}

int main()
{
	//for (int i = 0; i < argc; i++){
	//	printf("argv: %s\n", argv[i]);
	//}
	

    Editor s1;
	s1.Open("/home");
	s1.EditorLoop();

	printf("Done\n");
	return 0;
}