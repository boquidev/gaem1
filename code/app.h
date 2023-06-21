#include <math.h>
#include "helpers.h"
#include "gltf_loader.h"


#define update_type(name) void (*name)(App_memory*)
#define render_type(name) void (*name)(App_memory*, LIST(Renderer_request,), Int2 )
#define init_type(name) void (*name)(App_memory*, Init_data* )


// scale must be 1,1,1 by default

struct Tex_uid{
	u32 uid;
	b32 is_atlas;
	u32 rect_uid;
};

struct  Object3d{
#define OBJECT3D_STRUCTURE \
	u32 mesh_uid;\
	Tex_uid tex_uid;\
	\
	V3 scale;\
	V3 pos;\
	V3 rotation;\
	Color color;

	OBJECT3D_STRUCTURE
};
#define DEFAULT_OBJECT3D(objectp) \
	objectp->scale = {1,1,1};\
	objectp->color = {1,1,1,1};

enum RENDERER_REQUEST_TYPE_FLAGS{
	REQUEST_FLAG_RENDER_OBJECT 		= 1 << 0,
	REQUEST_FLAG_RENDER_IMAGE			= 1 << 1,
	REQUEST_FLAG_SET_VS 					= 1 << 2,
	REQUEST_FLAG_SET_PS					= 1 << 3,
	REQUEST_FLAG_SET_BLEND_STATE		= 1 << 4,
	REQUEST_FLAG_SET_DEPTH_STENCIL	= 1 << 5,
	//TODO: instancing request type
	//TODO: set texture or mesh request type
};

struct Renderer_request{
	u32 type_flags;
	union{
		Object3d object3d;
		struct{
			u32 vshader_uid;
			u32 pshader_uid;
			u32 blend_state_uid;
			u32 depth_stencil_uid;	
		};
	};
};

#define MAX_ENTITIES 5000
enum ENTITY_TYPE{
	ENTITY_FORGOR_TO_ASSIGN_TYPE,
	ENTITY_QUEEN,
	ENTITY_UNIT,
	ENTITY_PROJECTILE
};
enum UNIT_TYPE{
	UNIT_NOT_A_UNIT,
	UNIT_TURRET,
	UNIT_SPAWNER
};
struct Entity{
	b32 visible;
	b32 active;
	b32 selectable;
	ENTITY_TYPE type;
	UNIT_TYPE unit_type;

	r32 lifetime;

	s32 health;

	V3 target_move_pos;
	V3 velocity;

	V3 looking_at;
	V3 target_pos;
	
	r32 shooting_cooldown;
	r32 shooting_cd_time_left;

	r32 speed;

	// u32 parent_uid;
	u32 team_uid;
	r32 current_scale;

	union{
		Object3d object3d;
		struct{
			OBJECT3D_STRUCTURE
		};
	};
};

#define DEFAULT_ENTITY(entityp) \
	entityp->visible = true;\
	DEFAULT_OBJECT3D(entityp)

internal void
test_collision(Entity* e1, Entity* e2, r32 delta_time){
	if(e2->visible && e2->type != ENTITY_PROJECTILE ){
		V3 pos_difference = e2->pos-e1->pos;
		r32 collision_magnitude = v3_magnitude(pos_difference);
		//sphere vs sphere simplified
		r32 overlapping = (e1->current_scale+e2->current_scale) - collision_magnitude;
		if(overlapping > 0){
			V3 collision_direction = v3_normalize(pos_difference);
			if(!collision_magnitude)
				collision_direction = {1.0f,0,0};
			overlapping =  MIN(MIN(e1->current_scale, e2->current_scale),overlapping);
			e1->velocity = e1->velocity - (((overlapping/delta_time)/2) * collision_direction);
			e2->velocity = e2->velocity + (((overlapping/delta_time)/2) * collision_direction);
		}
	}
}

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
	u32 generation; // this value updates when the entity is deleted
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
	u32 triangle_mesh_uid;
	u32 ogre_mesh_uid;
	u32 female_mesh_uid;
	u32 turret_mesh_uid;
	u32 plane_mesh_uid;

	u32 ball_uid;
	u32 icosphere_uid;

	u32 test_orientation_uid;
	u32 test_orientation2_uid;
};

struct Textures
{
	Tex_uid default_tex_uid;
	Tex_uid white_tex_uid;
	Tex_uid ogre_tex_uid;
	Tex_uid font_atlas_uid;
};

struct VShaders
{
	u32 default_vshader_uid;
	u32 ui_vshader_uid;
};
struct PShaders
{
	u32 default_pshader_uid;
	u32 ui_pshader_uid;
};

struct Blend_states
{
	u32 default_blend_state_uid;
};

struct Depth_stencils
{
	u32 default_depth_stencil_uid;
	u32 ui_depth_stencil_uid;
};

struct App_memory
{
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;
	Textures textures;

	Packed_tex_info font_charinfo[CHARS_COUNT];

	VShaders vshaders;
	PShaders pshaders;

	Blend_states blend_states;
	Depth_stencils depth_stencils;

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
	
	r32 spawn_timer;

	u32 player_uid;

	s32 teams_resources[2];
	
	u32 creating_unit;

	u32 last_inactive_entity;
	Entity* entities;
	u32* entity_generations;
};

internal void
printo_screen(App_memory* memory,Int2 screen_size, LIST(Renderer_request,render_list),String text, V2 pos){
	r32 line_height = 18;
	Renderer_request* request = 0;
	r32 xpos = pos.x;
	UNTIL(c, text.length){
		char current_char = text.text[c];
		char char_index = current_char - FIRST_CHAR;

		if(current_char == ' ')
			xpos += 16.0f/screen_size.x;
		else{			
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_IMAGE;
			request->object3d = {
				memory->meshes.plane_mesh_uid,
				memory->textures.font_atlas_uid,
				{1,1,1},
				{xpos,pos.y,0},
				{0,0,0},
				{1,1,1,1}
			};
			request->object3d.tex_uid.rect_uid = char_index;
			request->object3d.pos.y -= (2.0f*line_height)/screen_size.y;
			xpos += 16.0f / screen_size.x;
		}
	}
}

// REQUESTS STRUCTS AND FUNCTIONS BOILERPLATE
// TODO: UNIFY ALL THIS FUNCTIONS AND STRUCTS TO BE JUST ONE THING

struct From_file_request
{
	u32* p_uid;
	String filename;
};

struct Mesh_from_primitives_request
{
	u32* p_mesh_uid;
	Mesh_primitive* primitives; //TODO: this could be the whole struct instead of a pointer
};

struct Tex_from_surface_request{
	Tex_uid* p_uid;
	Surface surface; 
};

struct Tex_from_file_request{
	Tex_uid* p_uid;
	String filename;
};

struct Vertex_shader_from_file_request
{
	u32* p_uid;
	String filename;
	u32 ie_count;
	String* ie_names;
	u32* ie_sizes;
	//TODO: per vertex vs per instance;
};
struct Create_blend_state_request
{
	u32* p_uid;
	b32 enable_alpha_blending;
};
struct Create_depth_stencil_request
{
	u32* p_uid;
	b32 enable_depth;
};

struct Init_data
{
	LIST(From_file_request, mesh_from_file_requests);
	LIST(Mesh_from_primitives_request, mesh_from_primitives_requests);
	LIST(Tex_from_surface_request, tex_from_surface_requests);
	LIST(Tex_from_file_request, tex_from_file_requests);
	LIST(Vertex_shader_from_file_request, vs_ff_requests);
	LIST(From_file_request, ps_ff_requests);
	LIST(Create_blend_state_request, create_blend_state_requests);
	LIST(Create_depth_stencil_request, create_depth_stencil_requests);
	LIST(Tex_from_file_request, load_font_requests);
};

//TODO: IS THERE A WAY TO JUST PUT VERTICES AND INDICES ARRAYS AND EVERYTHING ELSE JUST GETS SOLVED??
// PROBABLY WITH A MACRO
internal Mesh_primitive*
save_primitives(Memory_arena* arena, void* vertices, u32 v_size, u32 v_count, u16* indices, u32 indices_count)
{
	Mesh_primitive* result = ARENA_PUSH_STRUCT(arena, Mesh_primitive);
	*result = {0};
	result->vertices = arena_push_data(arena, vertices, v_count*v_size);
	result->vertex_count = v_count;
	result->vertex_size = v_size;

	result->indices = (u16*)arena_push_data(arena, indices, indices_count*sizeof(u16));
	result->indices_count = indices_count;

	return result;
}
// TODO: MAKE A SINGLE STRUCT FOR BOTH FROM FILE AND FROM DATA
// AND I HAVE 4 FUNCTIONS THAT ARE VERY SIMILAR

internal void
push_tex_from_surface_request(App_memory* memory, Init_data* init_data,Tex_uid* index_handle, u32 width, u32 height, u32* pixels)
{
	Tex_from_surface_request* request; 
	PUSH_BACK(init_data->tex_from_surface_requests,memory->temp_arena, request);
	request->p_uid = index_handle;
	request->surface = {width,height};
	request->surface.data = arena_push_data(memory->temp_arena, pixels, width*height*sizeof(u32));
}
// returns the address reserved for the mesh's uid
internal void
push_tex_from_file_request(App_memory* memory, Init_data* init_data, Tex_uid* index_handle, String filename)
{
	// pushes the request in the initdata in the temp arena
	Tex_from_file_request* request;
	PUSH_BACK(init_data->tex_from_file_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->filename = filename;
}

internal void
push_load_font_request(App_memory* memory, Init_data* init_data, Tex_uid* index_handle, String filename){
	// THIS IS EXACTLY THE SAME AS THE FUNCTION ABOVE EXCEPT FOR ONE THING FUCKK
	Tex_from_file_request* request;
	PUSH_BACK(init_data->load_font_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->filename = filename;
}

// THIS 2 FUNCTIONS ARE BASICALLY THE SAME EXCEPT FOR 2 THINGS
internal void
push_mesh_from_primitives_request(App_memory* memory, Init_data* init_data, u32* index_handle, Mesh_primitive* primitives)
{
	Mesh_from_primitives_request* request;
	PUSH_BACK(init_data->mesh_from_primitives_requests, memory->temp_arena, request);
	request->p_mesh_uid = index_handle;
	request->primitives = primitives;
}

// returns the address reserved for the mesh's uid
internal void
push_mesh_from_file_request(App_memory* memory, Init_data* init_data, u32* index_handle, String filename)
{
	// pushes the request in the initdata in the temp arena
	From_file_request* request;
	PUSH_BACK(init_data->mesh_from_file_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->filename = filename;
}

//TODO: why is this one different from the others
internal void
push_vertex_shader_from_file_request(App_memory* memory, Init_data* init_data, Vertex_shader_from_file_request request){
	Vertex_shader_from_file_request* result;
	PUSH_BACK(init_data->vs_ff_requests, memory->temp_arena, result);
	*result = request;
}

internal void
push_pixel_shader_from_file_request(App_memory* memory, Init_data* init_data, u32* index_handle, String filename){
	From_file_request* request;
	PUSH_BACK(init_data->ps_ff_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->filename = filename;
}

internal void
push_create_blend_state_request(App_memory* memory, Init_data* init_data, u32* index_handle, b32 enable_alpha_blending){
	Create_blend_state_request* request;
	PUSH_BACK(init_data->create_blend_state_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->enable_alpha_blending = enable_alpha_blending;
}

internal void
push_create_depth_stencil_request(App_memory* memory, Init_data* init_data, u32* index_handle, b32 enable_depth){
	Create_depth_stencil_request* request;
	PUSH_BACK(init_data->create_depth_stencil_requests, memory->temp_arena, request);
	request->p_uid = index_handle;
	request->enable_depth = enable_depth;
}

// internal void
// draw(LIST(Render_request, render_list), Memory_arena* arena, 
// 	Object3d* object3d
// // 	u32 mesh_uid,
// // 	u32 texture_uid,
// // 	u32 vshader_uid,
// // 	u32 pshader_uid,
// // 	u32 blend_state_uid,
// // 	u32 depth_stencil_uid,
// // 	V3 scale,
// // 	V3 pos,
// // 	V3 rotation,
// // 	Color color
// ){
// 	Object3d* render_object = LIST_PUSH_BACK_STRUCT(render_list, Object3d, arena);
// 	*render_object = *object3d;
// 	// *render_object = {
// 	// 	mesh_uid,
// 	// 	texture_uid,
// 	// 	vshader_uid,
// 	// 	pshader_uid,
// 	// 	blend_state_uid,
// 	// 	depth_stencil_uid,
// 	// 	scale,
// 	// 	pos,
// 	// 	rotation,
// 	// 	color
// 	// };
// }