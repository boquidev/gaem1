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

struct Init_data
{
	List meshes_uid_list;
};

struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;

	Color32 tilemap[32][32];
	User_input* input;
};
