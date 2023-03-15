#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>

class DisplayViewer : public Window {  
public:  
	DisplayViewer() : Window(140, 140, "DisplayViewer") {
		m_color = 1;
		UpdateColor();
	}

	void UpdateColor(){
		gfx_draw_rectangle(0, 0, c_width, c_height, COLOR_GRAY_DEFAULT);
		gfx_draw_rectangle(5, 5, c_width-7, c_height-20, m_color);
		gfx_draw_format_text(5, c_height-12, COLOR_BLACK, "Color: 0x%x", m_color);
		m_color++;
	}
	void Run()
	{
		while (1)
		{
			struct gfx_event event;
			gfx_get_event(&event);

			UpdateColor();
		}
	}

private:
	int m_color;

	/* Size is based on the fact that our filesystem can only handle 8kb files */
	const int c_width = 140;
	const int c_height = 140;
};

int main(void) {  
    DisplayViewer s1;
	s1.Run();

	printf("Done\n");
	return 0;
}