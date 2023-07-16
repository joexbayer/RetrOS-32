#include <lib/printf.h>
#include <lib/graphics.h>
#include <gfx/events.h>
#include <colors.h>

#define PIXELS_PER_BLOCK 8
#define WIDTH (16*PIXELS_PER_BLOCK)
#define HEIGHT ((255/16) * PIXELS_PER_BLOCK) + 32

class ColorPicker : public Window {  
public:  
	ColorPicker() : Window(WIDTH, HEIGHT, "ColorPicker", 0) {
		int x = 0, y = 0;
		for (int i = 0; i < 256; i++){
			gfx_draw_rectangle(x*PIXELS_PER_BLOCK, y*PIXELS_PER_BLOCK, PIXELS_PER_BLOCK, PIXELS_PER_BLOCK, i);
			x++;
			if(x == 16){
				x = 0;
				y++;
			}
		}
	}

	void Run()
	{
		while (1){
			struct gfx_event event;
			int ret = gfx_get_event(&event, GFX_EVENT_BLOCKING);
			if(ret == -1) continue;
			unsigned char color;

			switch (event.event){
			case GFX_EVENT_MOUSE:
				color = ((event.data2/PIXELS_PER_BLOCK)*16) + (event.data/PIXELS_PER_BLOCK);

				gfx_draw_rectangle(0, HEIGHT-16, WIDTH, 16, color);
				gfx_draw_format_text(0, HEIGHT-12, COLOR_WHITE, "Color: %d (0x%x)", color, color);
				break;
			case GFX_EVENT_EXIT:
				exit();
				break;
			default:
				break;
			}
		}
	}
};

int main(void) {  
    ColorPicker s1;
	s1.Run();

	while (1)
	{
		/* code */
	}
	

	printf("Done\n");
	return 0;
}