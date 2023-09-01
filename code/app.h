#include <math.h>
#include "helpers.h"
#include "gltf_loader.h"


#define update_type(...) void (*__VA_ARGS__)(App_memory*, Audio_playback*, u32, Int2)
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
#define MAX_UI 100
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

struct Element_handle
{
	u32 index;
	u32 generation; // this value updates when the entity is deleted
};
typedef Element_handle Entity_handle;

internal b32 
operator ==(Element_handle h1, Element_handle h2){
	return ((h1.index || h1.generation) && (h2.index || h2.generation)) && (h1.index == h2.index && h1.generation == h2.generation);
}
internal b32
operator !=(Element_handle h1, Element_handle h2){
	return (!(h1.index || h1.generation) || !(h2.index || h2.generation)) || (h1.index != h2.index || h1.generation != h2.generation);
}
internal b32 
compare_entity_handles(Element_handle h1, Element_handle h2){
	return h1 == h2;
}

enum COLLIDER_TYPE{
	// for now it is imposible to forger about the collider type okei?
	// FORGOR_COLLIDER_TYPE,
	COLLIDER_TYPE_SPHERE,
	COLLIDER_TYPE_CUBE,
};

enum ENTITY_ELEMENT_TYPE{
	EET_WATER = 1<<0,
	EET_FIRE = 1<<1,
	EET_ICE = 1<<2,
	EET_ELECTRIC = 1<<3,

	EET_HEAL = 1<<4,

	EE_ALL_TYPES = EET_WATER|EET_FIRE|EET_ICE|EET_ELECTRIC|EET_HEAL,

	EE_REACTIVE_ELEMENTS = EET_WATER|EET_FIRE|EET_ICE|EET_ELECTRIC,
};

// ENTITY MEGA-STRUCT

struct Entity{
	u64 flags;
	u16 element_type;
	u16 element_effect;

	f32 fog_debuff_time_left;

	ENTITY_TYPE type;
	UNIT_TYPE unit_type;
	COLLIDER_TYPE collider_type;

	f32 lifetime;

	f32 max_health;
	f32 health;// replace this with damage (maybe not), when health is damage it's tedious, or maybe i am dumb i will try it again

	// this is normalized and relative to the entity position 
	V3 normalized_accel;
	V3 velocity;

	f32 speed;
	f32 friction;
	f32 weight; // currently this is just for the gravity field

	// this is relative to the entity position
	V3 looking_direction;
	// this is relative to the entity position 
	V3 target_direction;
	

	// ACTION PROPERTIES

	f32 action_cd_total_time;
	f32 action_cd_time_passed;

	union{
		f32 action_power;	
		f32 total_power;
	};

	u32 action_count;
	f32 action_angle;

	f32 action_max_time;
	f32 action_range;

		f32 damage_redness;

	f32 aura_radius;

	f32 healing_cd;

	f32 toxic_time_left;
	f32 toxic_tick_damage_cd;

	f32 freezing_time_left;

	f32 gravity_field_time_left;

	f32 shield_cd;
	b32 shield_active;

	UNIT_TYPE spawn_unit_type;

	V3 ignore_sphere_pos;
	f32 ignore_sphere_radius;
	V3 ignore_sphere_target_pos;

	b32 jump_change_direction;

	Element_handle parent_handle;
	u32 team_uid;
	
	Entity_handle entity_to_stick;
	f32 relative_angle;
	b32 is_grabbing;
	
	f32 creation_size;
	f32 creation_delay;
	// each frame  creation_delay_time -= delta_time and
	// increment creation_size by (1-creation_size) / (creation_delay_time/delta_time)
	
	u32 state; // this WAS only used by the boss to handle phases

	union{
		Object3d object3d;
		struct{
			OBJECT3D_STRUCTURE
		};
	};
};
global_variable Entity nil_entity = {0}; 


internal f32
calculate_power(Entity* entity)
{
	f32 water_effect_multiplier = (entity->element_effect & EET_WATER) ? 0.8f : 1.0f;
	f32 fog_multiplier = (entity->fog_debuff_time_left) ? 0.7f : 1.0f;
	//TODO: benchmark the difference between this 2
	// f32 healer_multiplier = 1+(-2.0f*(entity->flags & E_HEALER)/E_HEALER); 
	f32 healer_multiplier = (entity->flags & E_HEALER) ? -1.0f : 1.0f; 
	return entity->action_power * water_effect_multiplier * healer_multiplier * fog_multiplier;
}

//TODO: this is in case reactions can be triggered by something different than a projectile
internal void 
calculate_elemental_reaction(void)
{

}

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
is_handle_valid(Element_handle handle, u32 entity_generations[])
{
	if(!handle.index && !handle.generation) return false;
	else return entity_generations[handle.index] == handle.generation;
}

internal Entity*
entity_from_handle(Element_handle handle, Entity* entities, u32* entity_generations){
	if(entity_generations[handle.index] == handle.generation)
		return &entities[handle.index];
	else
		return &nil_entity;
}

struct Ui_element{
	u32 parent_uid;

	String text;

	Int2 pos;
	Int2 size;

	Color color;

	f32 rotation;

	u64 flags;
};

internal b32
ui_is_point_inside(Ui_element* ui, Int2 p){
	b32 x_inside = IS_VALUE_BETWEEN(ui->pos.x, p.x, ui->pos.x + ui->size.x);
	b32 y_inside = IS_VALUE_BETWEEN(ui->pos.y, p.y, ui->pos.y + ui->size.y);
	return x_inside && y_inside;
	
	// return (
	// 		IS_VALUE_BETWEEN(ui->rect.x, p.x, ui->rect.x + ui->rect.w) &&
	// 		IS_VALUE_BETWEEN(ui->rect.y - ui->rect.h, p.y, ui->rect.y)
	// 	);
} 

struct User_input
{
	V2 cursor_pos;
	V2 cursor_speed;

	Int2 cursor_pixels_pos;

	union{
		struct {
			s32 cursor_primary;
			s32 cursor_secondary;

			s32 T;

			s32 F;
			s32 space_bar;
			s32 pause;

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

			s32 debug_up;
			s32 debug_down;
			s32 debug_left;
			s32 debug_right;
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
	u32 blank_entity_mesh_uid;

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
	
	u32 test_tex_uid;
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

	f32 update_hz;
	f32 delta_time;
	u32 time_ms; // this goes up to 1200 hours more or less 

	b32 is_paused;
	
	f32 spawn_timer; //TODO: this is not being used anymore

	u32 player_uid;

	s32 teams_resources[2];
	
	u32 creating_unit; // this is an index of the unit being created
	UNIT_TYPE possible_entities[7];
	s32 unit_creation_costs[UNIT_COUNT];

	Entity* entities;

	s32 clicked_uid;
	s32 selected_uid;
	u32 last_inactive_entity;

	Entity_handle closest_entity;
	b32 is_valid_grab;

	u32* entity_generations;

	Int2 radial_menu_pos;

	Ui_element* ui_elements;
	u32* ui_generations;

	s32 ui_selected_uid;
	s32 ui_clicked_uid;

	s32 debug_active_entities_count;
};

internal void 
default_object3d(Entity* out){
	out->scale = {1,1,1};
	out->color = {1,1,1,1};
}

// ENTITY "TYPES" 

global_variable u64 
	E_CANNOT_MANUALLY_AIM = 
		E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST|E_CANNOT_AIM
		,

	E_MELEE_FLAGS = 
	// if it hits at a certain rate without a care if there is an enemy
	// then use_cooldown, if it just hits when it detects an enemy 
	// and then can't hit until cooldown is restored, then dont use_cooldown
		E_VISIBLE|E_MELEE_ATTACK|
		E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|
		E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST|E_FOLLOW_TARGET
		,

	E_SHOOTER_FLAGS = 
		E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|
		E_RECEIVES_DAMAGE|E_CAN_MANUALLY_MOVE|E_SHOOT
		,

	E_TANK_FLAGS = 
		E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|
		E_RECEIVES_DAMAGE|E_CAN_MANUALLY_MOVE
		,

	E_SPAWNER_FLAGS = 
		E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|
		E_RECEIVES_DAMAGE|E_CAN_MANUALLY_MOVE|E_SPAWN_ENTITIES

		,

	E_SHIELD_FLAGS = 
		E_VISIBLE|E_RECEIVES_DAMAGE
		,

	E_WALL_FLAGS = 
		E_VISIBLE|E_HAS_COLLIDER|E_SKIP_UPDATING|E_NOT_TARGETABLE;

internal void
default_shooter(Entity* out, App_memory* memory){
	default_object3d(out);
	out->flags = E_SHOOTER_FLAGS;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SHOOTER;
	out->speed = 50.0f;
	out->friction = 4.0f;
	out->max_health = 3;
	out->health = out->max_health;
	out->action_cd_total_time = 2.0f;
	out->action_count = 5;
	out->action_angle = TAU32/8;
	out->mesh_uid = memory->meshes.shooter_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}
internal void
default_tank(Entity* out, App_memory* memory){
	default_object3d(out);
	out->flags = E_TANK_FLAGS;
	out->speed = 40.0f;
	out->friction = 4.0f;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_TANK;
	out->max_health = 4;
	out->health = out->max_health;
	out->action_cd_total_time = 5.0f;
	out->mesh_uid = memory->meshes.tank_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}
	
internal void
default_shield(Entity* out, App_memory* memory){
	out->flags = E_SHIELD_FLAGS;
	out->scale = {2,2,2};
	out->color = {1,1,1,0.5f};
	out->speed = 100.0f;
	out->friction = 4.0f;
	out->type = ENTITY_SHIELD;
	out->max_health = 8;
	out->health = out->max_health;
	out->action_cd_total_time = 0.0f;
	out->mesh_uid = memory->meshes.shield_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}

internal void
default_spawner(Entity* out, App_memory* memory){
	default_object3d(out);
	out->flags = E_SPAWNER_FLAGS;
	out->type = ENTITY_UNIT;
	out->unit_type = UNIT_SPAWNER;
	out->speed = 30.0f;
	out->friction = 4.0f;
	out->max_health = 2;
	out->action_cd_total_time = 2.0f;
	out->action_range = 5.0f;
	out->action_count = 2;
	out->action_angle = TAU32/2;
	out->spawn_unit_type = UNIT_MELEE;
	out->mesh_uid = memory->meshes.spawner_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}


internal void
default_melee(Entity* out, App_memory* memory){
	out->flags = E_MELEE_FLAGS;

	out->color = {1,1,1,1};
	out->scale = {0.5f,0.5f,0.5f};

	out->unit_type = UNIT_MELEE;
	out->speed = 60.0f;
	out->friction = 10.0f;
	out->max_health = 4;
	out->health = out->max_health;
	out->action_cd_total_time = 1.0f;
	out->action_power = 1;
	out->mesh_uid = memory->meshes.melee_mesh_uid;
	out->texinfo_uid = memory->textures.white_tex_uid;
}

internal void
default_wall(Entity* out, App_memory* memory){
	out->flags = E_WALL_FLAGS;
	out->type = ENTITY_OBSTACLE;
	out->mesh_uid = memory->meshes.cube_mesh_uid;
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
				//TODO: per vertex vs per instance
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
