#include <math.h>
#include "helpers.h"
#include "gltf_loader.h"

#include "defined_lists.h"

#define update_type(name) void (*name)(App_memory*)
#define render_type(name) void (*name)(App_memory*, Int2, List* )
#define init_type(name) void (*name)(App_memory*, Init_data* )


// scale must be 1,1,1 by default
struct Object3d
{
	u32* p_mesh_uid;
	u32* p_tex_uid;
	V3 scale;
	V3 pos;
	V3 rotation;
	Color color;
	b32 visible;
};

struct User_input
{
	V2 cursor_pos;
	V2 cursor_speed;

	b32 A;
	b32 forward;
	b32 backward;
	b32 left;
	b32 right;
	b32 up;
	b32 down;

	// b32 test;
};

//TODO: create a meshes array ??
// or make all render calls for each mesh

struct Meshes
{
	u32* p_triangle_mesh_uid;
	u32* p_ogre_mesh_uid;
	u32* p_female_mesh_uid;
	u32* p_turret_mesh_uid;
	u32* p_plane_mesh_uid;
	u32* p_test_orientation_uid;
	u32* p_test_orientation2_uid;
};

struct Textures
{
	u32* p_default_tex_uid;
	u32* p_white_tex_uid;
	u32* p_test_uid;
};

struct Mesh_from_file_request
{
	u32* p_mesh_uid;
	String filename;
};

struct Mesh_from_primitives_request
{
	u32* p_mesh_uid;
	Mesh_primitive* primitives; //TODO: this could be the whole struct instead of a pointer
};

struct Tex_from_surface_request
{
	u32* p_tex_uid;
	Surface surface; 
};

struct Tex_from_file_request
{
	u32* p_tex_uid;
	String filename;
};

DEFINE_LIST(Mesh_from_file_request);
DEFINE_LIST(Mesh_from_primitives_request);
DEFINE_LIST(Tex_from_surface_request);
DEFINE_LIST(Tex_from_file_request) ;
struct Init_data
{
	LIST(Mesh_from_file_request) mesh_from_file_requests;
	LIST(Mesh_from_primitives_request) mesh_from_primitives_requests;
	LIST(Tex_from_surface_request) tex_from_surface_requests;
	LIST(Tex_from_file_request) tex_from_file_requests;
};

#define MAX_ENTITIES 100
struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;
	Textures textures;

	V3 camera_pos;
	V3 camera_rotation;

	User_input* input;

	b32 is_window_in_focus;
	b32 lock_mouse;

	u32 player_uid;
	Object3d entities[MAX_ENTITIES];
};

internal u32
test(Mesh_primitive aaa[])
{	
	return sizeof(aaa);
}

//TODO: IS THERE A WAY TO JUST PUT VERTICES AND INDICES ARRAYS AND EVERYTHING ELSE JUST GETS SOLVED??
internal Mesh_primitive*
save_primitives(Memory_arena* arena, void* vertices, u32 vertex_size, u32 vertices_count, u16* indices, u32 indices_count)
{
	Mesh_primitive* result = ARENA_PUSH_STRUCT(arena, Mesh_primitive);
	*result = {0};
	result->vertices = arena_push_data(arena, vertices, vertices_count*vertex_size);
	result->vertices_count = vertices_count;
	result->vertex_size = vertex_size;

	result->indices = (u16*)arena_push_data(arena, indices, indices_count*sizeof(u16));
	result->indices_count = indices_count;

	return result;
}
// TODO: MAKE A SINGLE STRUCT FOR BOTH FROM FILE AND FROM DATA
// AND I HAVE 4 FUNCTIONS THAT ARE VERY SIMILAR

internal u32*
push_tex_from_surface_request(App_memory* memory, Init_data* init_data, u32 width, u32 height, u32* pixels)
{
	u32* result = ARENA_PUSH_STRUCT(memory->permanent_arena, u32);
	Tex_from_surface_request* request = init_data->tex_from_surface_requests.push_back(memory->temp_arena);
	request->p_tex_uid = result;
	request->surface = {width,height};
	request->surface.data = arena_push_data(memory->permanent_arena, pixels, width*height*sizeof(u32));

	return result;
}
// returns the address reserved for the mesh's uid
internal u32*
push_tex_from_file_request(App_memory* memory, Init_data* init_data, String filename)
{
	// reserves space for the uid in the permanent_arena
	u32* result = ARENA_PUSH_STRUCT(memory->permanent_arena, u32);
	// pushes the request in the initdata in the temp arena
	Tex_from_file_request* request = init_data->tex_from_file_requests.push_back(memory->temp_arena);
	request->p_tex_uid = result;
	request->filename = filename;

	return result;
}

// THIS 2 FUNCTIONS ARE BASICALLY THE SAME EXCEPT FOR 2 THINGS
internal u32*
push_mesh_from_primitives_request(App_memory* memory, Init_data* init_data, Mesh_primitive* primitives)
{
	u32* result = ARENA_PUSH_STRUCT(memory->permanent_arena, u32);
	Mesh_from_primitives_request* request = init_data->mesh_from_primitives_requests.push_back(memory->temp_arena);
	request->p_mesh_uid = result;
	request->primitives = primitives;

	return result;
}

// returns the address reserved for the mesh's uid
internal u32*
push_mesh_from_file_request(App_memory* memory, Init_data* init_data, String filename)
{
	// reserves space for the uid in the permanent_arena
	u32* result = ARENA_PUSH_STRUCT(memory->permanent_arena, u32);
	// pushes the request in the initdata in the temp arena
	Mesh_from_file_request* request = init_data->mesh_from_file_requests.push_back(memory->temp_arena);
	request->p_mesh_uid = result;
	request->filename = filename;

	return result;
}