#include <math.h>
#include "helpers.h"
#include "gltf_loader.h"


#define update_type(...) void (*__VA_ARGS__)(App_memory*, Audio_playback*, u32)
#define render_type(...) void (*__VA_ARGS__)(App_memory*, LIST(Renderer_request,), Int2 )
#define init_type(...) void (*__VA_ARGS__)(App_memory*, Init_data* )


// scale must be 1,1,1 by default

// struct Tex_uid{
// 	u32 tex_info_uid;
// 	// b32 is_atlas;
// };

#define OBJECT3D_STRUCTURE \
	u32 mesh_uid;\
	u32 texinfo_uid;\
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


	ENTITY_UNKNOWN,
	ENTITY_COUNT
};
enum UNIT_TYPE{
	UNIT_NOT_A_UNIT,
	UNIT_TANK,
	UNIT_SHOOTER,
	UNIT_SPAWNER,
	UNIT_MELEE,

	UNIT_UNKNOWN,
	UNIT_COUNT
};

struct Entity_handle
{
	u32 index;
	u32 generation; // this value updates when the entity is deleted
};

enum COLLIDER_TYPE{
	FORGOR_COLLIDER_TYPE,
	COLLIDER_TYPE_SPHERE,
	COLLIDER_TYPE_CUBE,
};


// ENTITY MEGA-STRUCT

struct Entity{
	u64 flags;
	ENTITY_TYPE type;
	UNIT_TYPE unit_type;
	COLLIDER_TYPE collider_type;

	f32 lifetime;

	s32 max_health;
	s32 health;

	// this is normalized and relative to the entity position 
	V3 target_move_pos;
	V3 velocity;

	// this is relative to the entity position
	V3 looking_at;
	// this is not relative to the entity position but to world position
	V3 target_pos;
	
	f32 action_cd_total_time;
	f32 action_cd_time_passed;
	
	// actions properties
	u32 action_count;//TODO:
	f32 action_angle;//TODO:

	f32 action_max_time;//TODO:
	f32 action_max_distance;//TODO:

	UNIT_TYPE spawn_unit_type;//TODO:

	f32 speed;

	Entity_handle parent_handle;
	u32 team_uid;
	
	f32 creation_delay_time; 
	f32 current_creation_time;
	
	u32 state; // for now this is only used by the boss

	union{
		Object3d object3d;
		struct{
			OBJECT3D_STRUCTURE
		};
	};
};
global_variable Entity nil_entity = {0}; 

internal u32
next_inactive_entity(Entity entities[], u32* last_inactive_i){
	u32 i = *last_inactive_i+1;
	for(; i != *last_inactive_i; i++){
		if(i == MAX_ENTITIES)
			i = 0;
		if(!entities[i].flags) break;
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
		if(!entities[i].flags) break;
	}
	ASSERT(i>=0);
	return i;
}

//TODO: use this
internal b32
is_handle_valid(u32 entity_generations[], Entity_handle handle)
{
	b32 result = entity_generations[handle.index] == handle.generation;
	ASSERT(result);
	return result;
}

internal Entity*
entity_from_handle(Entity* entities, u32* entity_generations, Entity_handle handle){
	if(entity_generations[handle.index] == handle.generation)
		return &entities[handle.index];
	else
		return &nil_entity;
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
		s32 buttons[30];//TODO: narrow this number to the amount of posible buttons
	};
};

//TODO: create a meshes array ??
//TODO: make all render calls for each mesh

//TODO: maybe load the default mesh/texture in index:0 from raw data instead of a file
// why? i don't remember
// and the indices from the file will start a 1
struct Meshes
{
	u32 default_mesh_uid;
	u32 ball_mesh_uid;
	u32 centered_cube_mesh_uid;
	u32 cube_mesh_uid;
	u32 centered_plane_mesh_uid;
	u32 plane_mesh_uid;
	u32 icosphere_mesh_uid;

	u32 player_mesh_uid;
	u32 spawner_mesh_uid;
	u32 boss_mesh_uid;
	u32 tank_mesh_uid;
	u32 shield_mesh_uid;
	u32 shooter_mesh_uid;
	u32 melee_mesh_uid;
};

struct Textures{
	u32 default_tex_uid;
	u32 white_tex_uid;
	u32 gradient_tex_uid;

	u32 font_atlas_uid;
};

struct VShaders{
	u32 default_vshader_uid;
	u32 ui_vshader_uid;
};
struct PShaders{
	u32 default_pshader_uid;
	u32 ui_pshader_uid;
};

struct Blend_states{
	u32 default_blend_state_uid;
};

struct Depth_stencils{
	u32 default_depth_stencil_uid;
	u32 ui_depth_stencil_uid;
};

struct Sounds{
	u32 wa_uid;
	u32 pe_uid;
	u32 pa_uid;
	u32 psss_uid;
};

struct App_memory
{
	b32 is_initialized;
	
	Memory_arena* permanent_arena;
	Memory_arena* temp_arena;

	Meshes meshes;
	Textures textures;
	Sounds sounds;

	LIST(Tex_info, tex_infos);
	u32 font_tex_infos_uids[CHARS_COUNT];

	VShaders vshaders;
	PShaders pshaders;

	Blend_states blend_states;
	Depth_stencils depth_stencils;

	f32 fov;
	f32 aspect_ratio;
	V3 camera_pos;
	V3 camera_rotation;

	User_input* input;
	User_input* holding_inputs;

	b32 is_window_in_focus;
	b32 lock_mouse;

	u32 highlighted_uid;
	u32 clicked_uid;
	u32 selected_uid;

	f32 update_hz;
	f32 delta_time;
	u32 time_ms; // this goes up to 1200 hours more or less 

	b32 is_paused;
	
	f32 spawn_timer; //TODO: this is not being used anymore

	u32 player_uid;

	s32 teams_resources[2];
	
	u32 creating_unit; // this is an index of the unit being created
	UNIT_TYPE possible_entities[4];
	s32 unit_creation_costs[UNIT_COUNT];

	u32 last_inactive_entity;
	Entity* entities;
	u32* entity_generations;
};

internal void 
default_object3d(Entity* out){
	out->scale = {1,1,1};
	out->color = {1,1,1,1};
}

// ENTITY "TYPES" 

global_variable u64 
	PROJECTILE_FLAGS = 
		E_VISIBLE|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|
		E_DOES_DAMAGE|E_HEALTH_IS_DAMAGE|E_DIE_ON_COLLISION|E_UNCLAMP_XZ,

	MELEE_FLAGS = 
	// if it hits at a certain rate without a care if there is an enemy
	// then use_cooldown, if it just hits when it detects an enemy 
	// and then can't hit until cooldown is restored, then dont use_cooldown
		E_VISIBLE|E_MELEE_ATTACK|E_LOOK_TARGET_WHILE_MOVING|
		E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|
		E_CAN_MOVE|E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST|E_FOLLOW_TARGET,

	SHOOTER_FLAGS = 
		E_VISIBLE|E_SELECTABLE|E_SHOOT|
		E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|
		E_CAN_MOVE,

	SHIELD_FLAGS = 
		E_VISIBLE|E_RECEIVES_DAMAGE,

	WALL_FLAGS = 
		E_VISIBLE|E_HAS_COLLIDER|E_SKIP_UPDATING;

internal void
default_shooter(Entity* out, App_memory* memory){
	default_object3d(out);
	out->flags = SHOOTER_FLAGS;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SHOOTER;
	out->speed = 50.0f;
	out->max_health = 3;
	out->health = out->max_health;
	out->action_cd_total_time = 2.0f;
	out->action_cd_time_passed = out->action_cd_total_time;
	out->mesh_uid = memory->meshes.shooter_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}
internal void
default_tank(Entity* out, App_memory* memory){
	default_object3d(out);
	out->speed = 40.0f;
	out->flags = E_VISIBLE|E_SELECTABLE;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_TANK;
	out->max_health = 4;
	out->health = out->max_health;
	out->action_cd_total_time = 5.0f;
	out->action_cd_time_passed = out->action_cd_total_time;
	out->mesh_uid = memory->meshes.tank_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}
	
internal void
default_shield(Entity* out, App_memory* memory){
	out->scale = {2,2,2};
	out->color = {1,1,1,0.5f};
	out->speed = 100.0f;
	out->flags = E_VISIBLE;
	out->type = ENTITY_SHIELD;
	out->max_health = 8;
	out->health = out->max_health;
	out->action_cd_total_time = 0.0f;
	out->action_cd_time_passed = out->action_cd_total_time;
	out->mesh_uid = memory->meshes.shield_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}

internal void
default_spawner(Entity* out, App_memory* memory){
	default_object3d(out);
	out->flags = E_VISIBLE|E_SELECTABLE;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SPAWNER;
	out->speed = 10.0f;
	out->max_health = 2;
	out->action_cd_total_time = 7.0f;
	out->action_cd_time_passed = out->action_cd_total_time;
	out->mesh_uid = memory->meshes.spawner_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}

internal void
default_projectile(Entity* out, App_memory* memory){
	out->lifetime = 5.0f;
	out->flags = E_VISIBLE;
	out->type = ENTITY_PROJECTILE;
	out->mesh_uid = memory->meshes.ball_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
	out->color = {0.6f,0.6f,0.6f,1};
	out->scale = {0.4f,0.4f,0.4f};
}

internal void
default_melee(Entity* out, App_memory* memory){
	out->flags = MELEE_FLAGS;

	out->color = {1,1,1,1};
	out->scale = {0.5f,0.5f,0.5f};

	out->unit_type = UNIT_MELEE;
	out->speed = 60.0f;
	out->max_health = 4;
	out->health = out->max_health;
	out->action_cd_total_time = 0.5f;
	out->action_cd_time_passed = out->action_cd_total_time;
	out->mesh_uid = memory->meshes.melee_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}


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
			OBJECT3D_STRUCTURE
		};
		struct{
			u32 vshader_uid;
			u32 pshader_uid;
			u32 blend_state_uid;
			u32 depth_stencil_uid;	
		};
	};
};

struct Sound_playback_request{
	u32 sound_uid;
};

internal V2
normalize_texture_size(Int2 client_size, Int2 tex_size){
	V2 result = {
		((2.0f*tex_size.x) / client_size.x),
		(2.0f*tex_size.y) / client_size.y
	};
	return result;
}

internal void
printo_world(App_memory* memory,Int2 screen_size, LIST(Renderer_request, render_list),String s, V3 pos, Color color){
	Renderer_request* request = 0;
	f32 xpos = pos.x;
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
			object->texinfo_uid = memory->font_tex_infos_uids[char_index];
			Tex_info* tex_info; LIST_GET(memory->tex_infos, object->texinfo_uid, tex_info);
			V2 normalized_scale = normalize_texture_size(screen_size, {tex_info->w, tex_info->h});
			object->scale = {30*normalized_scale.x, 30*normalized_scale.y, 1};

			object->pos = {xpos, pos.y, pos.z};
			object->rotation = {PI32/4,0,0};
			object->color = color;

			request->object3d.texinfo_uid = memory->font_tex_infos_uids[char_index];
			request->object3d.pos.y -= object->scale.y*2.0f;
			xpos += object->scale.x*16.0f/16;
		}
	}
}

internal void
printo_screen(App_memory* memory,Int2 screen_size, LIST(Renderer_request,render_list),String text, V2 pos, Color color){
	f32 line_height = 18;
	Renderer_request* request = 0;
	f32 xpos = pos.x;
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
			object->texinfo_uid = memory->font_tex_infos_uids[char_index];

			Tex_info* tex_info; LIST_GET(memory->tex_infos, object->texinfo_uid, tex_info);

			V2 normalized_scale = normalize_texture_size(screen_size, {tex_info->w, tex_info->h});
			object->scale = {normalized_scale.x, normalized_scale.y, 1};
			
			object->pos.x = xpos+((f32)(tex_info->xoffset)/screen_size.x);
			object->pos.y = pos.y-((2.0f*(line_height+tex_info->yoffset))/screen_size.y);

			object->rotation = {0,0,0};
			object->color = color;

			request->object3d.texinfo_uid = memory->font_tex_infos_uids[char_index];
			xpos += 16.0f / screen_size.x;
		}
	}
}

// REQUESTS STRUCTS AND FUNCTIONS BOILERPLATE
// TODO: UNIFY ALL THIS FUNCTIONS AND STRUCTS TO BE JUST ONE THING
enum Asset_request_type{
	FORGOR_TO_SET_ASSET_TYPE = 0,
	TEX_FROM_FILE_REQUEST,
	FONT_FROM_FILE_REQUEST,
	VERTEX_SHADER_FROM_FILE_REQUEST,
	PIXEL_SHADER_FROM_FILE_REQUEST,
	MESH_FROM_FILE_REQUEST,
	CREATE_BLEND_STATE_REQUEST,
	CREATE_DEPTH_STENCIL_REQUEST,
	SOUND_FROM_FILE_REQUEST,

	TEX_FROM_SURFACE_REQUEST,
	MESH_FROM_PRIMITIVES_REQUEST,
};

struct Asset_request{
	Asset_request_type type;
	u32* p_uid;
	union{
		struct {
			String filename;
			struct {
				u32 count;
				String* names;
				u32* sizes;
				//TODO: per vertex vs per instance;
			}ied; // input_element_desc
		};

		Mesh_primitive* mesh_primitives; 
		
		Surface tex_surface; 
		
		//TODO: THIS ARE NOT ASSETS BUT I DON'T KNOW WHERE TO PUT'EM
		b32 enable_alpha_blending;

		b32 enable_depth;
	};
};

struct Init_data
{
	// this is sent from the platform layer to the init function
	File_data meshes_serialization;
	File_data textures_serialization;
	File_data sounds_serialization;

	// this is the result from the init function to the platform layer
	LIST(Asset_request, asset_requests);
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

internal void
push_asset_request(App_memory* memory, Init_data* init_data, Asset_request* asset_request){
	ASSERT(asset_request->type);
	Asset_request* request;PUSH_BACK(init_data->asset_requests, memory->temp_arena, request);
	*request = *asset_request;
	*asset_request = {0};
}


// return value is -1 if substr is not a sub string of str, returns the pos it found the substring otherwise
internal s32
find_substring(String str, String substr){
	UNTIL(i, str.length-substr.length){
		if(compare_strings(substr, {str.text+i, substr.length})){
			return i;
		}
	}
	return -1;
}

struct String_index_pair{
	String str;
	u32* index_p;
};

internal void 
parse_assets_serialization_file(
	App_memory* memory, File_data file, 
	String_index_pair string_index_pairs[], u32 pairs_count,
	LIST(String, filenames)){
	char* scan = (char*)file.data;
	String key = string(":asset:");
	UNTIL(i, file.size - key.length){
		if(compare_strings(key, {scan+i, key.length})){
			i+=key.length;
 			String* filename; 
			PUSH_BACK(filenames, memory->temp_arena, filename);
			filename->text = scan+i;
			while(scan[i] != '\n' && scan[i]!= '\r'){
				filename->length++;
				i++;
			}
		}
	}

	String filestring = {(char*)file.data, file.size};
	UNTIL(i, pairs_count){
		s32 substring_pos = 0;
		u32 number_length = 0;

		substring_pos = find_substring(filestring, string_index_pairs[i].str);
		ASSERT(substring_pos >= 0);
		substring_pos += string_index_pairs[i].str.length;
		while(filestring.text[substring_pos+number_length] >= '0' && filestring.text[substring_pos+number_length] <= '9'){
			number_length++;
		}
		*string_index_pairs[i].index_p = string_to_int({filestring.text+substring_pos, number_length});
	}
}
