#include "app.h"

void update(App_memory* memory)
{
	u8 red = 0;
	u8 green = 0;
	u8 color_step = 255/ARRAY_COUNT(memory->tilemap);
	until(y, ARRAY_COUNT(memory->tilemap))
	{
		until(x, ARRAY_COUNT(memory->tilemap[y]))
		{
			memory->tilemap[y][x] = {red, green, 128, 255};
			red += color_step;
		}
		green += color_step;
	}
}

void render(App_memory* memory, Int2 screen_size)
{
	until(y, ARRAY_COUNT(memory->tilemap))
	{
		until(x, ARRAY_COUNT(memory->tilemap[y]))
		{
			//draw_rectangle(x, y, 10,10, tilemap[y][x]);
			
		}
	}
	Int2 rect_pos = memory->input->cursor_pos;
	// draw_rectangle(rect_pos.x, rect_pos.y, 15,15);
}

void init(App_memory* memory)
{

}