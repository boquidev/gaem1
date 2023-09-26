#include <math.h>
#include "helpers.h"
#include "gltf_loader.h"


#define update_type(...) void (*__VA_ARGS__)(App_memory*, Audio_playback*, u32, Int2)
#define render_type(...) void (*__VA_ARGS__)(App_memory*, LIST(Renderer_request,), Int2 )
#define init_type(...) void (*__VA_ARGS__)(App_memory*, Init_data* )

#define MAX_ENTITIES 5000
#define MAX_UI 100

struct Element_handle
{
	u32 index;
	u32 generation; // this value updates when the entity is deleted

	b32 is_valid(u32* generations)
	{
		// generation at index 0 must be different than 0
		if(!index && !generation || ((s32)index) < 0) return false;
		else return generations[index] == generation;
	}
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

struct Particle
{
	/* possible flags:
			accelerate in a direction
			accelerate inwards
			accelerate outwards
			rotate around creation pos
			accelerate towards a point 
			accelerate towards an entity

	*/
	u32 flags;
	u32 tex_uid;

	V3 position;
	V3 velocity;
	V3 acceleration;
	f32 friction;

	f32 lifetime;

	Color color;
	Color target_color;
	f32 color_delta_multiplier;

	f32 angle;
	f32 angle_speed;
	f32 angle_accel;
	f32 angle_friction;

	f32 scale;
	f32 target_scale;
	f32 scale_delta_multiplier;

	Entity_handle target_entity_h;
};


struct Particle_emitter
{
	u64 particle_flags;
	u32 particles_count;
	f32 emit_cooldown;
	u32 tex_uid;

	V3 initial_pos_offset;
	V3 initial_pos_rng;
	f32 velocity_yrotation_rng;
	f32 initial_speed_rng;
	f32 friction;
	V3 acceleration;

	Color color_rng;
	Color target_color;
	f32 color_delta_multiplier;

	f32 particle_lifetime;

	f32 initial_angle_rng;
	f32 angle_speed;
	f32 angle_initial_speed_rng;
	f32 angle_accel;
	f32 angle_friction;
	
	f32 initial_scale;
	f32 initial_scale_rng;
	f32 target_scale;
	f32 scale_delta_multiplier;

	//TODO: texinfo_uid;

	//TODO: this would be something metaprogramming could help with
	void fill_data(
		u64 _particle_flags,
		u32 _particles_count,
		f32 _emit_cooldown,
		u32 _tex_uid,

		V3 _initial_pos_offset,
		V3 _initial_pos_rng,
		f32 _velocity_yrotation_rng,
		f32 _initial_speed_rng,
		f32 _friction,
		V3 _acceleration,

		Color _color_rng,
		Color _target_color,
		f32 _color_delta_multiplier,

		f32 _particle_lifetime,

		f32 _initial_angle_rng,
		f32 _angle_speed,
		f32 _angle_initial_speed_rng,
		f32 _angle_accel,
		f32 _angle_friction,
		
		f32 _initial_scale,
		f32 _initial_scale_rng,
		f32 _target_scale,
		f32 _scale_delta_multiplier
	){
		STRUCT_FILLER(this, 
		->particle_flags = _particle_flags,
		->particles_count = _particles_count,
		->emit_cooldown = _emit_cooldown,
		->tex_uid = _tex_uid,


		->initial_pos_offset = _initial_pos_offset,
		->initial_pos_rng = _initial_pos_rng,
		->velocity_yrotation_rng = _velocity_yrotation_rng,
		->initial_speed_rng = _initial_speed_rng,
		->friction = _friction,
		->acceleration = _acceleration,

		->color_rng = _color_rng,
		->target_color = _target_color,
		->color_delta_multiplier = _color_delta_multiplier,

		->particle_lifetime = _particle_lifetime,

		->initial_angle_rng = _initial_angle_rng,
		->angle_speed = _angle_speed,
		->angle_initial_speed_rng = _angle_initial_speed_rng,
		->angle_accel = _angle_accel,
		->angle_friction = _angle_friction,

		->initial_scale = _initial_scale,
		->initial_scale_rng = _initial_scale_rng,
		->target_scale = _target_scale,
		->scale_delta_multiplier = _scale_delta_multiplier
		);
	}

	void emit_particle(Particle* particle, V3 position, V3 initial_velocity, Color color, RNG* rng)
	{
		particle->flags = (u32)particle_flags;
		particle->tex_uid = tex_uid;
		V3 pos_result_rng = {
			rng->next(initial_pos_rng.x)-(initial_pos_rng.x/2),
			rng->next(initial_pos_rng.y)-(initial_pos_rng.y/2),
			rng->next(initial_pos_rng.z)-(initial_pos_rng.z/2),
			};
		particle->position = position + initial_pos_offset + pos_result_rng;
		V3 final_initial_velocity = (1-rng->next(initial_speed_rng))*initial_velocity;
		particle->velocity = v3_rotate_y(final_initial_velocity, 
			rng->next(velocity_yrotation_rng)-(velocity_yrotation_rng/2));
		particle->acceleration = acceleration;
		particle->friction = friction;

		particle->color = {
			color.r + (rng->next(color_rng.r)-(color_rng.r/2)),
			color.g + (rng->next(color_rng.g)-(color_rng.g/2)),
			color.b + (rng->next(color_rng.b)-(color_rng.b/2)),
			color.a
		};
		particle->target_color = target_color;
		particle->color_delta_multiplier = color_delta_multiplier;

		particle->lifetime = particle_lifetime;

		particle->angle = rng->next(initial_angle_rng) - (initial_angle_rng/2);
		particle->angle_speed = angle_speed + (rng->next(angle_initial_speed_rng) - (angle_initial_speed_rng/2));
		particle->angle_accel = angle_accel;
		particle->angle_friction = angle_friction;

		particle->scale = initial_scale + rng->next(initial_scale_rng) - (initial_scale_rng/2);
		particle->target_scale = target_scale;
		particle->scale_delta_multiplier = scale_delta_multiplier;
	}
};

internal Particle* 
get_new_particle(Particle* particles, u32 max_particles, u32* last_used_index)
{
	u32 current_index = *last_used_index;
	UNTIL(i, max_particles)
	{
		current_index = (current_index+1) % max_particles;
		if(!particles[current_index].flags)
		{
			return &particles[current_index]; 
		}
	}
	ASSERT(false); // max particles reached
	*last_used_index = current_index;
	return 0;
}

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


enum COLLIDER_TYPE{
	// for now it is imposible to forger about the collider type okei?
	// FORGOR_COLLIDER_TYPE,
	COLLIDER_TYPE_SPHERE,
	COLLIDER_TYPE_CUBE,
};

enum ENTITY_ELEMENT_TYPE{
	EET_WATER = 1<<0,
	EET_HEAT = 1<<1,
	EET_COLD = 1<<2,
	EET_ELECTRIC = 1<<3,

	EET_HEAL = 1<<4,

	EE_ALL_TYPES = EET_WATER|EET_HEAT|EET_COLD|EET_ELECTRIC|EET_HEAL,

	EE_REACTIVE_ELEMENTS = EET_WATER|EET_HEAT|EET_COLD|EET_ELECTRIC,
};

internal Color 
element_color(u32 element)
{
	Color result = {1,0,1,0.5};
	switch(element)
	{
		case EET_WATER:
		{
			result = {0, 0.4f, 0.8f, 1};
		}break;
		case EET_HEAT:
		{
			result = {1, 0.4f, 0, 1};
		}break;
		case EET_COLD:
		{
			result = {0.7f, 0.7f, 1, 1};
		}break;
		case EET_ELECTRIC:
		{
			result = {1, 1, 0, 1};
		}break;
		case EET_HEAL:
		{
			result = {0,1,0,1};
		}break;
		case 0:
		{
			result = {0.5f,0.5f,0.5f,1};
		}break;
		default:
			ASSERT(false);
	}
	return result;
}

// ENTITY MEGA-STRUCT

struct Entity{
	u64 flags;
	u16 element_type;
	u16 element_effect;

	s32 total_upgrades_cost_value;

	f32 elemental_effect_duration;
	f32 elemental_damage_duration; // this is used to know how much will the duration effect last when this entity does damage

	f32 fog_debuff_time_left;

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

	//TODO: check if normalizing looking direction is better
	V3 looking_direction; // this is relative to the entity position

	V3 target_pos;// this is relative to world
	
	

	// ACTION PROPERTIES

	f32 action_cd_total_time;
	f32 action_cd_time_passed;

	union{
		f32 _action_power;	
		f32 total_power;
	};

	u32 action_count;
	f32 action_angle;

	f32 action_range;

	f32 aura_radius;

	f32 reaction_cooldown;

	f32 generate_resource_cd;

	f32 toxic_time_left;

	f32 freezing_time_left;

	f32 gravity_field_time_left;
	f32 gravity_field_radius;

	f32 shield_cd;
	b32 shield_active;

	V3 ignore_sphere_pos;
	f32 ignore_sphere_radius;
	V3 ignore_sphere_target_pos;

	b32 jump_change_direction;

	Element_handle parent_handle;
	u32 team_uid;
	
	Entity_handle grabbed_entity;
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

	f32 particle_timer;
	

};
global_variable Entity nil_entity = {0}; 

internal void fill_entity(
	Entity* e,
	u64 flags, 
	u16 element_type, 
	f32 posx, f32 posy,f32 posz,
	f32 rcolor,f32 gcolor,f32 bcolor,
	f32 scalex,f32 scaley,f32 scalez,
	f32 speed,
	f32 friction,
	f32 weight,
	f32 max_health,
	f32 total_power,
	f32 action_cd_total_time,
	f32 action_range,
	f32 aura_radius,
	Entity_handle parent_handle,
	u32 team_uid,
	f32 target_posx,f32 target_posy,f32 target_posz,
	u32 mesh_uid,
	u32 texinfo_uid
	)
{
	e->flags = flags;
	e->element_type = element_type;

	e->color = {rcolor,gcolor,bcolor,1};
	e->scale = {scalex,scaley,scalez};

	e->speed = speed;
	e->friction = friction;
	e->weight = weight;
	e->max_health = max_health;
	e->health = max_health;
	e->total_power = total_power;
	e->action_cd_total_time = action_cd_total_time;
	e->action_range = action_range;
	e->aura_radius = aura_radius;

	e->parent_handle = parent_handle;
	e->team_uid = team_uid;

	e->pos = {posx,posy,posz};
	
	e->target_pos = {target_posx, target_posy, target_posz};
	
	e->mesh_uid = mesh_uid;
	e->texinfo_uid = texinfo_uid;	
}

internal f32
calculate_power(Entity* entity)
{
	f32 water_effect_multiplier = (entity->element_effect & EET_WATER) ? 0.8f : 1.0f;
	f32 fog_multiplier = (entity->fog_debuff_time_left) ? 0.7f : 1.0f;
	//TODO: benchmark the difference between this 2
	// f32 healer_multiplier = 1+(-2.0f*(entity->flags & E_HEALER)/E_HEALER); 
	f32 healer_multiplier = (entity->element_type & EET_HEAL) ? 0.5f : 1.0f; 
	return entity->_action_power * water_effect_multiplier * healer_multiplier * fog_multiplier;
}

internal f32 
calculate_total_range(Entity* entity)
{
	f32 range_multiplier = 1.0f + (2.0f * !!(entity->flags & E_EXTRA_RANGE));
	return (range_multiplier * entity->action_range);	
}

internal Entity*
get_new_entity(Entity entities[], u32* last_inactive_i){
	u32 last_inactive_value = *last_inactive_i;
	u32 i = last_inactive_value+1;
	for(; i != last_inactive_value; i++){
		if(i == MAX_ENTITIES)
			i = 0;
		if(!entities[i].flags) break;
	}
	ASSERT(i != last_inactive_value);// there was no inactive entity
	*last_inactive_i = i;
	return &entities[i]; 
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
ui_is_point_inside(Ui_element* ui, Int2 p)
{
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

			s32 debug_next_level;
			s32 debug_previous_level;
			
			s32 debug_increase_spawn_charges;

			s32 debug_speed_up_delta_time;
			s32 debug_slow_down_delta_time;
		};
		s32 buttons[40];//TODO: narrow this number to the amount of posible buttons
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
	u32 ice_tex_uid;
	
	u32 test_tex_uid;
	u32 font_atlas_uid;
	// u32 PARTICLES_TEXTURE_ATLAS;
};

struct VShaders{
	u32 default_vshader_uid;
	u32 ui_vshader_uid;
	u32 postprocessing_vshader_uid;
};
struct PShaders{
	u32 default_pshader_uid;
	u32 ui_pshader_uid;
	u32 circle_pshader_uid;
	u32 circle_wave_pshader_uid;
	u32 postprocessing_pshader_uid;
};

struct Blend_states{
	u32 default_blend_state_uid;
};

struct Render_target_views
{
	u32 default_rtv;
	u32 post_processing_rtv;
	u32 depth_rtv;
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


enum SPAWN_TYPE
{
	ST_FORGOR_TO_SET_SPAWN_TYPE,
	ST_INDIVIDUAL,
	ST_TOGETHER,
};

enum ELEMENT_POOL_ASSIGNMENT_TYPE
{
	EPAT_FORGOR_TO_SET_ASSIGNMENT_TYPE,
	EPAT_ONE_TO_ONE,
	EPAT_RANDOM,
};

enum BOSS_ACTION_TYPE
{
	BAT_FORGOR_TO_SET_ACTION,
	BAT_SPAWN_ENTITIES,
	BAT_WAIT_TIME,
	BAT_MOVE,
};
struct Boss_action
{
	BOSS_ACTION_TYPE action_type;

	union
	{

		struct // BAT_SPAWN_ENTITIES
		{
			u32 entities_to_spawn_count;
			
			SPAWN_TYPE spawn_type;
			struct {
				V3 positions[16];
				f32 relative_angles[16];
			};

			ELEMENT_POOL_ASSIGNMENT_TYPE element_pool_assignment_type;
			u16 elements_pool[16];

		}spawn_properties;

		struct // BAT_WAIT_TIME
		{
			f32 wait_timer;
		};
		
		struct // BAT_MOVE
		{
			V3 move_position;
		};
	};
};

internal Boss_action
boss_action_wait(f32 wait_duration)
{
	Boss_action result;
	result.action_type = BAT_WAIT_TIME;
	result.wait_timer = wait_duration;
	return result;
}

internal Boss_action
boss_action_move(V3 move_position)
{
	Boss_action result;
	result.action_type = BAT_MOVE;
	result.move_position = move_position;
	return result;
}

struct Boss_sequence
{
	f32 minimum_life_percent_threshold;
	LIST(Boss_action, actions_pool);
	LIST(u32, action_indices_sequence);
};

struct Level_state{
	u32 boss_phase;
	u32 boss_state;
	f32 boss_timer;
	
	// TODO: this could be done immediate mode instead of retained

	// f32 spawned_entities_attack_cd;
	// f32 spawned_entities_attack_damage;
	// f32 spawned_entities_speed;
	// f32 spawned_entities_health;
	
	// u32 possible_elements_count;
	// u16 possible_elements [10];
	// Boss_sequence* Boss_sequence;
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
	Render_target_views render_target_views;
	Depth_stencils depth_stencils;

	RNG rng;

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
	f32 time_s;

	b32 is_paused;

	u32 player_uid;

	s32 teams_resources[2];
	f32 add_resource_current_time;
	f32 add_resource_total_cd;
	s32 team_spawn_charges[2];

	b32 creating_entity;
	V3 new_entity_pos;

	Entity* entities;
	u32* entity_generations;
	// this is just so that entities that are grabbing cannot be grabbed by other entities
	b8 entities_grabbing_property_list [MAX_ENTITIES];

	Entity_handle clicked_entity_h;
	Entity_handle selected_entity_h;
	u32 last_used_entity_index;

	Entity_handle closest_entity_to_grab_h;
	b32 is_valid_grab;


	Int2 radial_menu_pos;

	Ui_element* ui_elements;
	s32 ui_costs [MAX_UI];

	s32 ui_pressed_uid;
	s32 ui_clicked_uid;

	u32 current_level;
	// u32 levels_count;
	Level_state level_state;

	s32 debug_active_entities_count;

	Particle* particles;
	u32 particles_max;
	u32 last_used_particle_index;
};

internal void 
calculate_elemental_reaction(Entity* entity, Entity* entity2, App_memory* memory, LIST(Entity,entities_to_create))
{
	// CHEKING IF A ELEMENTAL REACTION OCURRED
	u16 elemental_damage = entity2->element_type & EE_REACTIVE_ELEMENTS;
	if(elemental_damage)
	{
		u16 current_element = (entity->element_type|entity->element_effect) & (EE_REACTIVE_ELEMENTS);
		if(!current_element || entity->element_effect == elemental_damage)
		{
			entity->element_effect = elemental_damage;
			entity->elemental_effect_duration = entity2->elemental_damage_duration;

			if(!current_element)
			{
				Color initial_color = element_color(elemental_damage);
				Color target_color = initial_color;
				target_color.a = 0;


				Particle_emitter particle_emitter;
				particle_emitter.fill_data(
					PARTICLE_ACTIVE,
					30,
					0,
					0,
					{0},
					{0},
					TAU32,
					0,
					10.0f,
					{0},
					{0},
					target_color,
					1.0f,
					1.0f,
					0,0,0,0,0,
					.3f,
					0,
					.1f,
					1.0f
				);
				UNTIL(current_particle, particle_emitter.particles_count)
				{
					particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
						entity->pos,{40.0f, 0,0}, initial_color, &memory->rng);
				}

				particle_emitter.fill_data(
					PARTICLE_ACTIVE,
					1,
					0,
					0,
					{0},
					{0},
					0,
					0,
					0,
					{0},
					{0},
					{1,1,1,1},
					1.0f,
					.3f,
					0,0,0,0,0,
					3.0f,
					0,
					.0f,
					1.0f
				);
				particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
					entity->pos,{0}, initial_color, &memory->rng);
			}

			return;
		}
		if(!entity->reaction_cooldown)
		{
			u16 reaction = elemental_damage | current_element;
			b32 reaction_ocurred = true;
			
			switch(reaction)
			{
				case EET_WATER|EET_HEAT:{
					Entity* smoke_screen; PUSH_BACK(entities_to_create, memory->temp_arena, smoke_screen);
					smoke_screen->flags = E_VISIBLE|E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_NOT_TARGETABLE|
						E_SHRINK_WITH_LIFETIME|E_SMOKE_SCREEN
						;
					smoke_screen->element_type = EET_WATER;
					smoke_screen->elemental_damage_duration = 2*memory->delta_time;
					smoke_screen->mesh_uid = memory->meshes.icosphere_mesh_uid;
					smoke_screen->texinfo_uid = memory->textures.white_tex_uid;

					smoke_screen->color = {1,1,1,0.2f};
					smoke_screen->scale = {10,10,10};
					smoke_screen->creation_delay = 0.3f;
					smoke_screen->lifetime = 4.8f;

					smoke_screen->pos = entity->pos;
					
					Particle_emitter particle_emitter = {0};

					particle_emitter.particle_flags = PARTICLE_ACTIVE;
					particle_emitter.particles_count = 1;
					particle_emitter.emit_cooldown = memory->delta_time*2;

					particle_emitter.velocity_yrotation_rng = TAU32;
					particle_emitter.friction = 10.0f;
					particle_emitter.initial_speed_rng = 0.8f;
					
					particle_emitter.acceleration = {0,20.0f, 0};
					
					particle_emitter.color_rng = {0.2f, 0.2f, 0.2f};
					particle_emitter.target_color = {0.3f,0.3f,1,0};
					particle_emitter.color_delta_multiplier = 1.0f;

					particle_emitter.particle_lifetime = 1.5f;

					particle_emitter.initial_angle_rng = PI32;
					particle_emitter.angle_initial_speed_rng = 20.0f;
					particle_emitter.angle_friction = 4.0f;
					
					particle_emitter.initial_scale = 0.2f;
					particle_emitter.target_scale = .7f;
					particle_emitter.scale_delta_multiplier = 1.0f;

					UNTIL(i, 20)
					{
						Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
						// Color particles_color = {0.5f, 0.5f, 0.5f, 1};
						Color particles_color = {1,0.4f,0.2f,1};
						particle_emitter.emit_particle(new_particle, entity->pos, {100.0f,0.0f,0}, particles_color, &memory->rng);
					}
				}break;
				case EET_WATER|EET_COLD:{
					entity->freezing_time_left = 5.0f;
					
					Particle_emitter particle_emitter = {0};

					particle_emitter.particle_flags = PARTICLE_ACTIVE;
					particle_emitter.particles_count = 1;

					particle_emitter.target_color = {.5f,.6f,1,0};
					particle_emitter.color_delta_multiplier = 5.0f;

					particle_emitter.particle_lifetime = 1.0f;
					
					particle_emitter.initial_scale = 3.0f;
					particle_emitter.target_scale = 1.5f;
					particle_emitter.scale_delta_multiplier = 20.0f;

					Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
					// Color particles_color = {0.5f, 0.5f, 0.5f, 1};
					Color particles_color = {1,1,1,1};
					particle_emitter.emit_particle(new_particle, entity->pos, {0,0,0}, particles_color, &memory->rng);
				}break;
				case EET_WATER|EET_ELECTRIC:{
					Entity* e; // jumping lightning
					PUSH_BACK(entities_to_create, memory->temp_arena, e);
					e->flags = E_VISIBLE|E_NOT_TARGETABLE|E_DOES_DAMAGE
						|E_UNCLAMP_XZ|E_TOXIC_DAMAGE_INMUNE|E_IGNORE_ALLIES
						|P_JUMP_BETWEEN_TARGETS|E_EMIT_PARTICLES
						;

					e->element_type = EET_ELECTRIC;
					e->elemental_damage_duration = 1.0f;
					e->jump_change_direction = true;

					e->max_health = 2;
					e->health = e->max_health;
					e->speed = 60;
					e->friction = 0.0f;
					e->lifetime = 5.0f;
					e->weight = 100.0f;
					e->total_power = 2.0f*entity2->total_power;
					e->aura_radius = 2.0f;

					e->pos = entity->pos;
					e->velocity = {e->speed, 0, 0};
					e->team_uid = entity2->team_uid;

					e->ignore_sphere_pos = entity2->ignore_sphere_pos;
					e->ignore_sphere_radius = entity2->ignore_sphere_radius;
					e->ignore_sphere_target_pos = entity2->ignore_sphere_target_pos;

					e->mesh_uid = memory->meshes.ball_mesh_uid;
					e->texinfo_uid = memory->textures.white_tex_uid;
					e->color = {1.0f, 1.0f, 0.0f, 1};
					e->scale = {0.4f, 0.4f, 0.4f};

					Particle_emitter particle_emitter = {0}; 
					particle_emitter.particle_flags = PARTICLE_ACTIVE;
					particle_emitter.particles_count = 1;

					particle_emitter.target_color = {1,1,0,1};
					particle_emitter.color_delta_multiplier = 30.0f;

					//TODO: implement freeze frames/stutter to make this  more visible
					particle_emitter.particle_lifetime = memory->delta_time*3;
					
					particle_emitter.initial_scale = 5.0f;
					
					Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
					particle_emitter.emit_particle(new_particle, entity->pos, {0,0,0}, {1,1,1,1}, &memory->rng);



					particle_emitter = {0};

					particle_emitter.particle_flags = PARTICLE_ACTIVE;
					particle_emitter.particles_count = 7;

					particle_emitter.velocity_yrotation_rng = TAU32;
					particle_emitter.friction = 10.0f;
					particle_emitter.color_rng = {.3f, .3f, .3f};
					particle_emitter.target_color = {1,1,0,1};
					particle_emitter.particle_lifetime = .6f;

					particle_emitter.initial_scale = .3f;
					particle_emitter.initial_scale_rng = .2f;
					particle_emitter.target_scale = .05f;
					particle_emitter.scale_delta_multiplier = 1.0f;

					UNTIL(particle_counter, particle_emitter.particles_count)
					{
						
						particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
							entity->pos, {50.0f, 0,0}, {1,1,0,1}, &memory->rng
						);
					}

				}break;
				case EET_HEAT|EET_COLD:{
					Entity* explosion_hitbox; PUSH_BACK(entities_to_create, memory->temp_arena, explosion_hitbox);

					explosion_hitbox->flags = E_VISIBLE|E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_EXPLOSION;

					explosion_hitbox->color = {1.0f, 1.0f, 0, 1.0f};
					explosion_hitbox->scale = {8,8,8};
					explosion_hitbox->creation_size = 1.0f;

					explosion_hitbox->lifetime = memory->delta_time;

					// explosion_hitbox->parent_handle.index = i;
					// explosion_hitbox->parent_handle.generation = generations[i];
					// explosion_hitbox->team_uid = entity->team_uid;
					explosion_hitbox->total_power = 15.0f;

					explosion_hitbox->pos = entity->pos;

					// explosion_hitbox->mesh_uid = memory->meshes.centered_plane_mesh_uid;
					explosion_hitbox->mesh_uid = memory->meshes.icosphere_mesh_uid;
					explosion_hitbox->texinfo_uid = memory->textures.white_tex_uid;

					
					Particle_emitter particle_emitter = {0};
					particle_emitter.fill_data(
						PARTICLE_ACTIVE,
						1,
						0,
						0,
						{0},
						{0},
						0,
						0,
						0,
						{0},
						{0},
						{1.0f, 0, 0, 0},
						1.0f,
						0.2f,
						0,0,0,0,0,
						8.0f, 0, 5.0f, 1.0f
					);
					particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
						entity->pos, {0,0,0}, {1,1,0,1}, &memory->rng
					);

					particle_emitter.fill_data(
						PARTICLE_ACTIVE,
						8,
						0,
						0,
						{0},
						{0},
						TAU32,
						0,
						10.0f,
						{0, 20.0f, 0},
						{0},
						{1.0f,.5f,.5f,0},
						1.0f,
						1.0f,
						0,0,0,0,0,
						0.5f, 0, 1.0f,1.0f
					);
					UNTIL(particle_i, particle_emitter.particles_count)
					{
						particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
							entity->pos, {50.0f, 0,0}, {1,0.9f,0,1}, &memory->rng
						);
					}

				}break;
				case EET_HEAT|EET_ELECTRIC:{
					entity->toxic_time_left = 8.0f;

					Particle_emitter particle_emitter;
					particle_emitter.fill_data(
						PARTICLE_ACTIVE, 
						20, 
						0,
						0,
						{0},
						{0},
						TAU32,
						1.0f,
						10.0f,
						{0,20.0f,0},
						{0},
						{1,0,1,0},
						1.0f,
						1.5f,
						0,0,0,0,0,
						0.2f, 0, 0.5f,1.0f
					);
					UNTIL(particle_count, particle_emitter.particles_count)
					{
						particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
							entity->pos, {50.0f, 0,0}, {0.5f,0,1,1}, &memory->rng
						);
					}

				}break;
				case EET_COLD|EET_ELECTRIC:{
					entity->gravity_field_time_left = 10.0f;
					entity->gravity_field_radius = 10.0f;
					
					Particle_emitter particle_emitter;
					particle_emitter.fill_data(
						PARTICLE_ACTIVE,
						1,
						0,
						0,
						{0},
						{0},
						0,
						0,
						0,
						{0},
						{0},
						{0,0,0,1},
						5.0f,
						2.0f,
						0,0,0,0,0,
						10.0f,
						0,
						0.01f,
						5.0f
					);

					particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
						entity->pos, {0}, {1,1,1,1}, &memory->rng
					);
				}break;
				// THE SAME ELEMENT
				case EET_WATER:
				case EET_HEAT:
				case EET_COLD:
				case EET_ELECTRIC: 
					reaction_ocurred = false;
				break;
				default: // OTHER
					ASSERT(false);
			}
			if(reaction_ocurred){
				entity->reaction_cooldown = 5.0f;
				entity->element_effect = 0;
			}
		}
	}
}

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



enum RENDERER_REQUEST_TYPE_FLAGS
{//TODO: invert the order so that i can set render pipeline in the same request as rendering an object
//for that i would need to unwrap the Renderer_request union into a full size struct
	REQUEST_FLAG_RENDER_OBJECT 		= 1 << 0,
	REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN		= 1 << 1,
	REQUEST_FLAG_RENDER_IMAGE_TO_WORLD		= 1 << 2,
	REQUEST_FLAG_SET_VS 					= 1 << 3,
	REQUEST_FLAG_SET_PS					= 1 << 4,
	REQUEST_FLAG_SET_BLEND_STATE		= 1 << 5,
	REQUEST_FLAG_SET_DEPTH_STENCIL	= 1 << 6,
	REQUEST_FLAG_RENDER_PARTICLES		= 1 << 7, //TODO: turn this into instancing
	REQUEST_FLAG_SET_DEPTH_WRITING 	= 1 << 8,
	REQUEST_FLAG_SET_TIME 				= 1 << 9,


	REQUEST_FLAG_POSTPROCESSING 		= 1 << 30,

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
			u32 render_target_view_uid;
			u32 depth_stencil_uid;	
			f32 depth_writing;
			f32 new_time;
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
			object->color = color;

			request->object3d.texinfo_uid = memory->font_tex_infos_uids[char_index];
			// request->object3d.pos.y -= object->scale.y*2.0f;
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
	CREATE_RENDER_TARGET_VIEW,
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
	LIST(String, filenames))
{
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
