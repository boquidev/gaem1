#include "helpers.h"
#include "gltf_loader.h"

#define update_type(name) void (*name)(App_memory*)
#define render_type(name) void (*name)(App_memory*, Int2, List* )
#define init_type(name) void (*name)(App_memory*, Init_data* )


// scale must be 1,1,1 by default
struct Object3d
{
	u32 mesh_uid;
	V3 pos;
	V3 rotation;
	V3 scale;
};

internal Object3d
object3d(u32 mesh_uid, V3 pos, V3 rotation, V3 scale)
{
	Object3d result;
	result.mesh_uid = mesh_uid;
	result.pos = pos;
	result.rotation = rotation;
	result.scale = scale;
	return result;
}

struct User_input
{
	Int2 cursor_pos;
	b32 A;
};

//TODO: create a meshes array ??
// or make all render calls for each mesh

struct Meshes
{
	u32* triangle_mesh_uid;
	u32* ogre_mesh_uid;
};

struct Mesh_from_file_request
{
	u32* p_mesh_uid;
	String filename;
};

struct Mesh_from_primitives_request
{
	u32* p_mesh_uid;
	Mesh_primitive* primitives;
};

struct Init_data
{
	List mesh_from_file_requests;
	List mesh_from_primitives_requests;
};

struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;

	Color32 tilemap[32][32];
	User_input* input;
};

internal u32
test(Mesh_primitive aaa[])
{	
	return sizeof(aaa);
}

// THIS 2 FUNCTIONS ARE BASICALLY THE SAME EXCEPT FOR 2 THINGS
internal u32*
push_mesh_from_primitives_request(App_memory* memory, Init_data* init_data, Mesh_primitive* primitives)
{
	u32* result = ARENA_PUSH_STRUCT(memory->permanent_arena, u32);
	Mesh_from_primitives_request* request = LIST_PUSH_BACK_STRUCT(&init_data->mesh_from_primitives_requests,Mesh_from_primitives_request, memory->temp_arena);
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
	Mesh_from_file_request* request = LIST_PUSH_BACK_STRUCT(&init_data->mesh_from_file_requests,Mesh_from_file_request, memory->temp_arena);
	request->p_mesh_uid = result;
	request->filename = filename;

	return result;
}
