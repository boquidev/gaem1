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
#define OBJECT3D_STRUCTURE \
	u32* p_mesh_uid;\
	u32* p_tex_uid;\
	V3 scale;\
	V3 pos;\
	V3 rotation;\
	Color color;

	OBJECT3D_STRUCTURE
};

#define MAX_ENTITIES 1000
struct Entity
{
	b32 visible;
	r32 lifetime;

	b32 selectable;

	u32 resources;

	b32 is_bullet;//TODO: make this the identifier of what kind of entity it is

	V3 target_move_pos;
	V3 velocity;

	V3 looking_at;
	V3 target_pos;
	r32 shooting_cooldown;
	r32 shooting_cd_time_left;

	//TODO: i don't like this
	r32 speed;

	// u32 parent_uid;
	u32 team_uid;

	r32 radius;

	union{
		Object3d object3d;
		struct{
			OBJECT3D_STRUCTURE
		};
	};
};

internal u32
next_inactive_entity(Entity entities[], u32* last_inactive_i){
	u32 i = *last_inactive_i+1;
	for(; i != *last_inactive_i; i++){
		if(i == MAX_ENTITIES)
			i = 0;
		if(!entities[i].visible) break;
	}
	ASSERT(i != *last_inactive_i);// there was no inactive entity
	*last_inactive_i = i;
	return i; 
}

struct Entity_handle
{
	u32 index;
	u32 generation;
};

//TODO: use this
internal b32
is_handle_valid(u32 entity_generations[], Entity_handle handle)
{
	b32 result = entity_generations[handle.index] == handle.generation;
	ASSERT(result);
	return result;
}

struct User_input
{
	V2 cursor_pos;
	V2 cursor_speed;

	union{
		struct {
			s32 cursor_primary;
			s32 cursor_secondary;

			s32 cancel;
			s32 move;

			// s32 forward;
			// s32 backward;
			s32 d_up;
			s32 d_down;
			s32 d_left;
			s32 d_right;

			s32 L;
			s32 R;
		};
		s32 buttons[20];//TODO: narrow this number to the number of posible buttons
	};
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

	u32* p_ball_uid;

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

struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;
	Textures textures;

	r32 fov;
	r32 aspect_ratio;
	V3 camera_pos;
	V3 camera_rotation;

	User_input* input;
	User_input* holding_inputs;

	b32 is_window_in_focus;
	b32 lock_mouse;

	u32 highlighted_uid;
	u32 clicked_uid;
	u32 selected_uid;

	r32 update_hz;
	r32 delta_time;
	u32 time_ms; // this goes up to 1200 hours more or less 

	u32 player_uid;
	
	b32 creating_unit;

	u32 last_inactive_entity;
	Entity entities[MAX_ENTITIES];
	u32 entity_generations[MAX_ENTITIES];
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