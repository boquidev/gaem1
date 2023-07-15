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

#define OBJECT3D_STRUCTURE \
	u32 mesh_uid;\
	Tex_uid tex_uid;\
	\
	V3 scale;\
	V3 pos;\
	V3 rotation;\
	Color color;

struct  Object3d{
	OBJECT3D_STRUCTURE
};

#define MAX_ENTITIES 5000
enum ENTITY_TYPE{
	ENTITY_FORGOR_TO_ASSIGN_TYPE,
	ENTITY_UNIT,
	ENTITY_BOSS,
	ENTITY_PROJECTILE,
	ENTITY_SHIELD, // this is a type of entity and not of unit because it doesn't have collision responses
	ENTITY_OBSTACLE,
};
enum UNIT_TYPE{
	UNIT_NOT_A_UNIT,
	UNIT_TANK,
	UNIT_SHOOTER,
	UNIT_SPAWNER,


	UNIT_COUNT
};

struct Entity{
	b32 visible;
	b32 active;
	b32 selectable;
	ENTITY_TYPE type;
	UNIT_TYPE unit_type;
	u32 state;

	r32 lifetime;

	s32 max_health;
	s32 health;

	V3 target_move_pos;
	V3 velocity;

	V3 looking_at;
	V3 target_pos;
	
	r32 shooting_cooldown;
	r32 shooting_cd_time_left;

	r32 speed;

	u32 parent_uid;
	u32 team_uid;
	r32 current_scale; //TODO: this is becoming troublesome

	union{
		Object3d object3d;
		struct{
			OBJECT3D_STRUCTURE
		};
	};
};

// internal void
// test_collision(Entity* e1, Entity* e2, r32 delta_time){
// 	V3 pos_difference = e2->pos-e1->pos;
// 	r32 collision_magnitude = v3_magnitude(pos_difference);
// 	//sphere vs sphere simplified
// 	r32 overlapping = (e1->current_scale+e2->current_scale) - collision_magnitude;
// 	if(overlapping > 0){
// 		V3 collision_direction = v3_normalize(pos_difference);
// 		if(!collision_magnitude)
// 			collision_direction = {1.0f,0,0};
// 		overlapping =  MIN(MIN(e1->current_scale, e2->current_scale),overlapping);
// 		e1->velocity = e1->velocity - (((overlapping/delta_time)/2) * collision_direction);
// 		e2->velocity = e2->velocity + (((overlapping/delta_time)/2) * collision_direction);
// 	}
// }

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

// this is cuz transparent objects should not be rendered before other objects cuz they would 
// just not show them like if they were opaque
internal u32
last_inactive_entity(Entity entities[]){
	u32 i = MAX_ENTITIES-1;
	for(; i >= 0; i--){
		if(!entities[i].visible) break;
	}
	ASSERT(i>=0);
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

			s32 reset;

			s32 cancel;
			s32 move;

			s32 d_up;
			s32 d_down;
			s32 d_left;
			s32 d_right;

			s32 L;
			s32 R;

			// this is probably temporal
			s32 k1;
			s32 k2;
			s32 k3;
			s32 k4;
			s32 k5;
			s32 k6;
		};
		s32 buttons[30];//TODO: narrow this number to the number of posible buttons
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
	u32 centered_cube_mesh_uid;
	u32 plane_mesh_uid;
	u32 shield_mesh_uid;
	u32 shooter_mesh_uid;

	u32 ball_mesh_uid;
	u32 icosphere_mesh_uid;
	u32 cube_mesh_uid;

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

	b32 is_paused;
	
	r32 spawn_timer; //TODO: this is not being used anymore

	u32 player_uid;

	s32 teams_resources[2];
	
	UNIT_TYPE creating_unit; // this is an index of the unit being created
	s32 unit_creation_costs[UNIT_COUNT];

	u32 last_inactive_entity;
	Entity* entities;
	u32* entity_generations;
};
enum RENDERER_REQUEST_TYPE_FLAGS{
	REQUEST_FLAG_RENDER_OBJECT 		= 1 << 0,
	REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN		= 1 << 1,
	REQUEST_FLAG_RENDER_IMAGE_TO_WORLD		= 1 << 2,
	REQUEST_FLAG_SET_VS 					= 1 << 3,
	REQUEST_FLAG_SET_PS					= 1 << 4,
	REQUEST_FLAG_SET_BLEND_STATE		= 1 << 5,
	REQUEST_FLAG_SET_DEPTH_STENCIL	= 1 << 6,
	//TODO: instancing request type
	//TODO: set texture or mesh request type for instancing
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

internal void
printo_world(App_memory* memory,Int2 screen_size, LIST(Renderer_request, render_list),String s, V3 pos, Color color){
	Renderer_request* request = 0;
	r32 xpos = pos.x;
	UNTIL(c, s.length){
		char current_char = s.text[c];
		char char_index = CHAR_TO_INDEX(current_char);

		if(current_char == ' ')
			xpos += 16.0f/16;
		else{
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_WORLD;

			Object3d* object = &request->object3d;
			object->mesh_uid = memory->meshes.plane_mesh_uid;
			object->tex_uid = memory->textures.font_atlas_uid;
			object->scale = {.25f,.25f,1};
			object->pos = {xpos, pos.y, pos.z};
			object->rotation = {PI32/4,0,0};
			object->color = color;

			request->object3d.tex_uid.rect_uid = char_index;
			request->object3d.pos.y -= object->scale.y*2.0f;
			xpos += object->scale.x*16.0f/16;
		}
	}
}

internal void
printo_screen(App_memory* memory,Int2 screen_size, LIST(Renderer_request,render_list),String text, V2 pos, Color color){
	r32 line_height = 18;
	Renderer_request* request = 0;
	r32 xpos = pos.x;
	UNTIL(c, text.length){
		char current_char = text.text[c];
		char char_index = CHAR_TO_INDEX(current_char);

		if(current_char == ' ')
			xpos += 16.0f/screen_size.x;
		else{			
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
			Object3d* object = &request->object3d;
			object->mesh_uid = memory->meshes.plane_mesh_uid;
			object->tex_uid = memory->textures.font_atlas_uid;
			object->scale = {1,1,1};
			object->pos = {xpos,pos.y,0};
			object->rotation = {0,0,0};
			object->color = color;

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

internal void 
default_object3d(Entity* out){
	out->scale = {1,1,1};
	out->color = {1,1,1,1};
}

internal void
default_entity(Entity* out){
	out->visible = true;
	default_object3d(out);
}

internal void
default_shooter(Entity* out, App_memory* memory){
	default_entity(out);
	out->current_scale = MIN(1.0f, memory->delta_time);
	out->selectable = true;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SHOOTER;
	out->speed = 40.0f;
	out->max_health = 2;
	out->health = out->max_health;
	out->shooting_cooldown = 0.9f;
	out->shooting_cd_time_left = out->shooting_cooldown;
	out->mesh_uid = memory->meshes.shooter_mesh_uid;
	out->tex_uid = memory->textures.white_tex_uid;
}
internal void
default_tank(Entity* out, App_memory* memory){
	default_entity(out);
	out->speed = 40.0f;
	out->current_scale = MIN(1.0f, memory->delta_time);
	out->selectable = true;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_TANK;
	out->max_health = 10;
	out->health = out->max_health;
	out->shooting_cooldown = 2.0f;
	out->shooting_cd_time_left = out->shooting_cooldown;
	out->mesh_uid = memory->meshes.centered_cube_mesh_uid;
	out->tex_uid = memory->textures.white_tex_uid;
}
	
internal void
default_shield(Entity* out, App_memory* memory){
	default_entity(out);
	out->scale = {2,2,2};
	out->color = {1,1,1,0.5f};
	out->speed = 100.0f;
	out->current_scale = MIN(1.0f, memory->delta_time);
	out->selectable = false;
	out->type = ENTITY_SHIELD;
	out->max_health = 20;
	out->health = out->max_health;
	out->shooting_cooldown = 0.0f;
	out->shooting_cd_time_left = out->shooting_cooldown;
	out->mesh_uid = memory->meshes.shield_mesh_uid;
	out->tex_uid = memory->textures.white_tex_uid;
}

internal void
default_spawner(Entity* out, App_memory* memory){
	default_entity(out);
	out->current_scale = MIN(1.0f, memory->delta_time);
	out->selectable = true;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SPAWNER;
	out->speed = 10.0f;
	out->max_health = 2;
	out->shooting_cooldown = 5.0f;
	out->shooting_cd_time_left = out->shooting_cooldown;
	out->mesh_uid = memory->meshes.test_orientation2_uid;
	out->tex_uid = memory->textures.white_tex_uid;
}

internal void
default_projectile(Entity* out, App_memory* memory){
	default_entity(out);
	out->lifetime = 5.0f;
	out->active = true;
	out->current_scale = 1.0f;
	out->type = ENTITY_PROJECTILE;
	out->mesh_uid = memory->meshes.ball_mesh_uid;
	out->tex_uid = memory->textures.white_tex_uid;
	out->color = {0.6f,0.6f,0.6f,1};
	out->scale = {0.4f,0.4f,0.4f};
}