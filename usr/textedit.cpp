#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>

class Editor {  
public:  
	Editor() {
		m_x = 0;
		m_y = 0;
		textBuffer = (char*) malloc((c_width/8)*(c_height/8));
		gfx_create_window(c_width, c_height);
		gfx_draw_rectangle(0, 0, c_width, c_height, COLOR_WHITE);
		gfx_set_title("Editor");
	}

	~Editor() {
		free(textBuffer);
	}

	void Save();
	
	void Open(char* path) {
		m_fd = open(path);
		if(m_fd <= 0)
			return;
		
		int ret = read(m_fd, textBuffer, (c_width/8)*(c_height/8));
		for (int i = 0; i < ret; i++)
		{
			putChar(textBuffer[i]);
		}
		
	}

	void putChar(char c) {
		textBuffer[m_y*(c_width/8) + m_x] = c;

		gfx_draw_rectangle(m_x*8, m_y*8, 8, 8, COLOR_WHITE);
		gfx_draw_char(m_x*8, m_y*8, c, COLOR_BLACK);

		m_x++;
		if(m_x > c_width/8){
			m_x = 0;
			m_y++;
		}

		gfx_draw_char(m_x*8, m_y*8, '_', COLOR_BLACK);
	}

	void EditorLoop()
	{
		while (1)
		{
			struct gfx_event event;
			gfx_get_event(&event);
			switch (event.event)
			{
			case GFX_EVENT_KEYBOARD:
				switch (event.data)
				{
				case '\n':
					gfx_draw_rectangle(m_x*8, m_y*8, 8, 8, COLOR_WHITE);
					m_x = 0;
					m_y++;
					gfx_draw_char(m_x*8, m_y*8, '_', COLOR_BLACK);
					break;
				case '\b':
					gfx_draw_rectangle(m_x*8, m_y*8, 8, 8, COLOR_WHITE);
					m_x--;
					gfx_draw_rectangle(m_x*8, m_y*8, 8, 8, COLOR_WHITE);
					gfx_draw_char(m_x*8, m_y*8, '_', COLOR_BLACK);
					break;
				default:
					putChar(event.data);
					break;
				}
				break;
			default:
				break;
			}
		}
	}

private:
	int m_fd = -1;
	char* textBuffer;
	int m_x, m_y;

	/* Size is based on the fact that our filesystem can only handle 8kb files */
	const int c_width = 280;
	const int c_height = 240;
};

int main(void) {  
    Editor s1;
	s1.Open("/home");
	s1.EditorLoop();

	printf("Done\n");
	return 0;
}