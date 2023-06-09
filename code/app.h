#include "helpers.h"
#include "gltf_loader.h"

#define update_type(name) void (*name)(App_memory*)
#define render_type(name) void (*name)(App_memory*, Int2)
#define init_type(name) void (*name)(App_memory*)

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
