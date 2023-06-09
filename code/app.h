#include "definitions.h"
#include "string.h"

#define update_type(name) void (*name)(App_memory*)
#define render_type(name) void (*name)(App_memory*, Int2)
#define init_type(name) void (*name)(App_memory*)

internal void 
set_mem(void* mem, u32 size, u8 value)
{
	u8* scan = (u8*)mem;
	until(i, size)
	{
		*scan = value;
		scan++;
	}
}

struct Int2
{
	s32 x;
	s32 y;
};

struct Color32
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct Memory_arena
{
	u8* data;
	u32 used;
	u32 size;
};

internal u8*
arena_push_size(Memory_arena* arena, u32 size)
{
	u8* result = arena->data+arena->used;
	arena->used += size;
	return result;
}

internal void
arena_pop_size(Memory_arena* arena, u32 size)
{
	arena->used -= size;
	set_mem(arena->data+arena->used, size, 0);
}

struct User_input
{
	Int2 cursor_pos;
	b32 A;
};

struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Color32 tilemap[32][32];
	User_input* input;
};
