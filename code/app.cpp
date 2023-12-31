#include "app.h"

#define MAX_SPAWN_DISTANCE 5.0f
#define BOSS_INDEX 1

global_variable Element_handle global_boss_handle = {0};
global_variable Element_handle global_player_handle = {0};

global_variable u64 DEFAULT_UNIT_FLAGS = E_LOOK_IN_THE_MOVING_DIRECTION
	|E_SHOOT
	|E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|E_GIVE_LOOT
	|E_EMIT_PARTICLES
	;

void update(App_memory* memory, Audio_playback* playback_list, u32 sample_t, Int2 client_size)
{
	if(!memory->is_initialized)
	{
		memory->is_initialized = true;

		// RESETING DEFAULT VALUES
		
		set_mem(memory->entities, MAX_ENTITIES*sizeof(Entity), 0);
		set_mem(memory->entity_generations, MAX_ENTITIES*sizeof(u32), 0);
		memory->entity_generations[0] = 1;

		memory->camera_rotation.x = PI32/4;
		memory->camera_rotation.y = 0;
		memory->camera_rotation.z = 0;
		memory->camera_pos.y = 32.0f;
		memory->is_paused = 0;

		memory->last_used_entity_index = 0;

		global_player_handle.index = memory->player_uid;
		global_player_handle.generation = memory->entity_generations[memory->player_uid];

		memory->level_state = {0};
		memory->teams_resources[0] = 0;
		memory->teams_resources[1] = 0;
		memory->add_spawn_charge_timer = 0;
		memory->team_spawn_charges[0] = 0;
		memory->team_spawn_charges[1] = 0;

		// INITIALIZING MAIN ENTITIES

		if(memory->current_level)
		{
			
			Entity* player = &memory->entities[memory->player_uid];
			default_object3d(player);
			player->flags = E_VISIBLE|E_AUTO_AIM_BOSS
				|E_EMIT_PARTICLES
				|E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|E_NOT_MOVE
				;
			player->pos = {-25, 0, 0};
			player->max_health = 100;
			player->health = player->max_health;
			player->team_uid = 0;

			player->total_power = 1.0f;
			player->action_count = 1;
			player->action_angle = TAU32/4;
			player->action_cd_total_time = .1f;
			player->action_range = 5.0f;

			player->aura_radius = 3.0f;

			memory->teams_resources[player->team_uid] = 50;
			memory->base_resources_per_second = 0.5f;
			player->creation_delay = 0.2f;

			player->team_uid = 0;
			player->speed = 5.0f;
			player->friction = 4.0f;
			player->weight = 10.0f;

			player->mesh_uid = memory->meshes.player_mesh_uid;
			player->texinfo_uid = memory->textures.white_tex_uid;
			
			global_boss_handle.index = BOSS_INDEX;
			global_boss_handle.generation = memory->entity_generations[BOSS_INDEX];

			memory->level_state.boss_timer = 0; 

			Entity* boss = &memory->entities[BOSS_INDEX];
			default_object3d(boss);
			u64 common_boss_flags = 
				E_VISIBLE|E_DETECT_COLLISIONS|E_HAS_COLLIDER|E_RECEIVES_DAMAGE|E_NOT_MOVE
				|E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST
				|E_EMIT_PARTICLES
				;
			if(memory->current_level == 1)
			{
				boss->flags = common_boss_flags;
				boss->speed = 60.0f;
				boss->friction = 10.0f;

				boss->max_health = 100;
				boss->health = boss->max_health;
				boss->pos = {25, 0, 0};
				boss->team_uid = 1;
				boss->creation_delay = 0.2f;

				boss->total_power = 1.0f;
				boss->action_count = 1;
				boss->action_angle = TAU32/4;
				boss->action_cd_total_time = 10.5f;
				boss->action_range = 5.0f;
				boss->aura_radius = 3.0f;

				boss->weight = 10.0f;

				boss->mesh_uid = memory->meshes.boss_mesh_uid;;
				boss->texinfo_uid = memory->textures.default_tex_uid;
			}else if (memory->current_level == 2){
				boss->flags = common_boss_flags | E_SHOOT;
				boss->speed = 60.0f;
				boss->friction = 10.0f;

				boss->max_health = 500;
				boss->health = boss->max_health;
				boss->pos = {25, 0, 0};
				boss->team_uid = 1;
				boss->creation_delay = 0.2f;

				boss->total_power = 1.0f;
				boss->action_count = 1;
				boss->action_angle = TAU32/4;
				boss->action_cd_total_time = 1.0f;
				boss->action_range = 5.0f;
				boss->aura_radius = 3.0f;

				boss->weight = 10.0f;

				boss->mesh_uid = memory->meshes.boss_mesh_uid;;
				boss->texinfo_uid = memory->textures.default_tex_uid;
			}
		}

		//TODO: turn this into handles
		memory->selected_entity_h = {0};
		memory->ui_clicked_uid = -1;
		memory->ui_pressed_uid = -1;
	}

	User_input* input = memory->input;
	User_input* holding_inputs = memory->holding_inputs;
	Ui_element* ui_elements = memory->ui_elements;


	// UI

	// CALCULATE UI_ELEMENT RECTANGLES ?
	// this was an autolayout thing that ryan fleury proposed in his ui series


	// FIND OUT WHOS THE ACTIVE UI_ELEMENT

	
	s32 ui_hot_element = -1;
	UNTIL(i, MAX_UI)
	{
		if(!ui_elements[i].flags) continue;
		
		if(ui_is_point_inside(&ui_elements[i], input->cursor_pixels_pos))
		{
			ui_hot_element = i;
		}
	}


	if(input->cursor_primary == 1)
	{
		memory->ui_pressed_uid = ui_hot_element;
		memory->creating_entity = false;
	}
	
	if(input->cursor_primary == -1)
	{
		if(memory->ui_pressed_uid == ui_hot_element){
			memory->ui_clicked_uid = ui_hot_element;
		}else{//TODO: this is probably unnecessary 
			memory->ui_clicked_uid = -1;
		}
		memory->ui_pressed_uid = -1;
	}else{
		memory->ui_clicked_uid = -1;
	}
	b32 is_cursor_in_ui = ui_hot_element >= 0;


	// CLEANING LAST FRAME UI_ELEMENTS
	
	UNTIL(i, MAX_UI){
		ui_elements[i] = {0};
		memory->ui_costs[i] = 0;
	}
	s32 ui_last = 0;

	//  INITIALIZING INPUT, SELECTED ENTITY, DELTA_TIME AND LEVEL


	Entity* entities = memory->entities;
	u32* generations = memory->entity_generations;
	RNG* rng = &memory->rng;
	
	b32 exists_selected_entity = memory->selected_entity_h.is_valid(generations);
	Entity* selected_entity = &entities[memory->selected_entity_h.index];
	
	if(input->debug_speed_up_delta_time == 1)
	{
		memory->delta_time *= 1.5f;
	}
	if(input->debug_slow_down_delta_time == 1)
	{
		memory->delta_time /= 1.5f;
	}
	f32 world_delta_time = memory->delta_time;

	Entity* player_entity = &entities[memory->player_uid];
	Entity* boss_entity = &entities[BOSS_INDEX];

	if(input->T == 1)
	{
		memory->is_initialized = false;
	}
	if(input->debug_next_level == 1 )
	{
		memory->is_initialized = false;
		memory->current_level = (memory->current_level+1) % memory->levels_count;
	}
	if(input->debug_previous_level == 1 )
	{
		memory->is_initialized = false;
		memory->current_level = (memory->current_level+memory->levels_count-1) % memory->levels_count;
	}

	f32 sensitivity = 2.0f;

	memory->camera_rotation.y += sensitivity*(f32)input->cursor_speed.x;
	memory->camera_rotation.x += -sensitivity*(f32)input->cursor_speed.y;
	memory->camera_rotation.x = CLAMP(-PI32/2, memory->camera_rotation.x, PI32/2);	

	memory->camera_pos = v3_rotate_y(v3_rotate_x({0,0,-32}, memory->camera_rotation.x), memory->camera_rotation.y);


	V3 screen_cursor_pos = { // fov is from side to side of the screen, not from center to side
		memory->aspect_ratio*(memory->fov/2)*input->cursor_pos.x,
		(memory->fov/2)*input->cursor_pos.y, 
		0
	};


	// MAIN MENU 


	if(!memory->current_level || memory->is_paused)
	{
		s32 buttons_size = 100;
		u32 levels_count = memory->levels_count;
		s32 buttons_total_length = buttons_size * levels_count;
		for(u32 lvl_button = 0; lvl_button < levels_count; lvl_button++)
		{

			if(memory->ui_clicked_uid == ui_last)
			{
				memory->current_level = lvl_button;
				memory->is_initialized = 0;
			}
		
			Ui_element* ui_element;
			ui_element = &ui_elements[ui_last++];
			
			ui_element->color = {1,1,1,1};

			ui_element->flags = 1;
			if(!lvl_button){
				ui_element->text = string("MAIN MENU");
			}else{
				ui_element->text = number_to_string(lvl_button, memory->temp_arena); 
			}

			ui_element->size = {buttons_size, buttons_size};

			ui_element->pos.x = ((client_size.x - buttons_total_length)/2) + (lvl_button*buttons_size);
			ui_element->pos.y = (client_size.y - ui_element->size.y)/2;
		}
		

		if(memory->ui_clicked_uid == ui_last)
		{
			*memory->global_running = false;
		}
		Ui_element* ui_element = &ui_elements[ui_last++];

		ui_element->color = {1,1,1,1};
		ui_element->flags = 1;
		ui_element->text = string("exit");

		ui_element->size.x = 220;
		ui_element->size.y = 100;
		ui_element->pos = {
			(client_size.x - ui_element->size.x)/2,
			ui_element->size.y+10+(client_size.y - ui_element->size.y)/2};

		
		// highlight selected buttons

		if(ui_hot_element >= 0){
			ui_elements[ui_hot_element].color = colors_product(ui_elements[ui_hot_element].color, {0.7f,0.7f,1.0f,1.0f});
		}
		if(memory->ui_pressed_uid >= 0){
			ui_elements[memory->ui_pressed_uid].color = {1,1, 0.5f, 1};
		}

	} 	
	if(!memory->current_level){
		memory->bg_color = {0.05f, 0.05f, 0.05f, 1};
		return;
	} 
	else memory->bg_color = {0.3f, 0.3f, 0.3f, 1};

	// CREATING UI BUTTONS

	// CREATING RADIAL MENU

	#define MENU_RADIUS 300

	if(input->R == 1){
		// memory->radial_menu_pos = input->cursor_pixels_pos;
		memory->radial_menu_pos.x = CLAMP(MENU_RADIUS, input->cursor_pixels_pos.x, client_size.x - (MENU_RADIUS));
		memory->radial_menu_pos.y = CLAMP(MENU_RADIUS+25, input->cursor_pixels_pos.y, client_size.y - (25+MENU_RADIUS));
	}

	if(input->R > 0){

		u64 possible_upgrades[] = {
			E_DEFEND_BOSS,
			E_SHOOT,
			E_GENERATE_RESOURCE,
			E_HAS_SHIELD,
			E_EXTRA_RANGE,
			E_FOLLOW_TARGET,
			E_AUTO_AIM_CLOSEST,
			// extra damage,

			
			/*
			// E_CAN_MANUALLY_MOVE,
			E_AUTO_AIM_BOSS,
			E_PROJECTILE_PIERCE_TARGETS,
			E_HOMING_PROJECTILES,
			E_LIFE_STEAL,
			E_PROJECTILE_JUMPS_BETWEEN_TARGETS,
			E_TOXIC_EMITTER|E_TOXIC_DAMAGE_INMUNE,
			E_PROJECTILE_EXPLODE,
			E_FREEZING_ACTIONS,
			E_HIT_SPAWN_GRAVITY_FIELD,
			E_STICK_TO_ENTITY,
			E_HEALER,
			*/
		};

		char* button_text[] = {
			"DEFEND",
			"shoot",
			"generate resources",
			"add shield",
			"extra range",
			"move forward",
			"autoaim",
			
			/*
			// "manually move",
			"melee attack",
			E_MELEE_ATTACK,
			"autoaim boss",
			"piercing projectiles",
			"homing projectiles",
			"lifesteal",
			"projectile jump between targets",
			"toxic",
			"explode projectiles",
			"freezing abilities",
			"hits spawn gravity field",
			"stick to parent",
			"healer",
			*/
		};

		s32 upgrade_costs[] = 
		{
			0,
			10,
			40,
			20,
			20,
			5,
			10,
		};

		f32 angle_step = TAU32 / ARRAYCOUNT(button_text);
		V2 initial_position = {0,-MENU_RADIUS};

		s32 cost_multiplier = 1;
		if(exists_selected_entity)
		{
			UNTIL(i, ARRAYCOUNT(possible_upgrades))
			{
				if(entities[memory->selected_entity_h.index].flags & possible_upgrades[i])
				{
					cost_multiplier *= 2;
				}
			}
		}

		UNTIL(i, ARRAYCOUNT(button_text))
		{
			s32 total_cost = upgrade_costs[i] * cost_multiplier;

			b32 property_already_set = exists_selected_entity ? 
				!((selected_entity->flags & possible_upgrades[i]) ^ possible_upgrades[i]) :
				false;
			
			if(!property_already_set)
			{
				memory->ui_costs[ui_last] = total_cost;
			}
			if(ui_last == memory->ui_clicked_uid)
			{
				if(exists_selected_entity && 
					(
						memory->teams_resources[player_entity->team_uid] >= total_cost ||
						property_already_set
					)
				){
					if(!property_already_set)
					{
						memory->teams_resources[player_entity->team_uid] -= total_cost;
						selected_entity->total_upgrades_cost_value += upgrade_costs[i];
					}
					else
					{
						selected_entity->total_upgrades_cost_value -= upgrade_costs[i];
					}

					if(possible_upgrades[i] & E_TOXIC_EMITTER) // toxic emitter
					{
						if(selected_entity->flags & E_TOXIC_EMITTER){
							selected_entity->flags &= ~possible_upgrades[i];
						}else{
							selected_entity->flags |= possible_upgrades[i];
						}
					}
					else // default case
					{
						selected_entity->flags ^= possible_upgrades[i];
					}
				}
			}


			Ui_element* ui_element = &ui_elements[ui_last++];
			
			
			if(exists_selected_entity){
				if(property_already_set){
					ui_element->color = {1.0f,1.0f,1.0f,1};
				}else{
					if(memory->teams_resources[player_entity->team_uid] < total_cost)
					{
						ui_element->color = {1, 0.6f, 0.6f, 1};
					}
					else
					{
						ui_element->color = {0.6f,0.6f,0.6f,1};
					}
				}
			}else{
				ui_element->color = {0.2f,0.2f,0.2f,1};
			}

			ui_element->flags = 1;
			ui_element->text = string(button_text[i]); 
			
			V2 current_position = v2_rotate(initial_position, i*angle_step);

			ui_element->size.x = 110;
			ui_element->size.y = 50;

			ui_element->pos.x = (s32)(memory->radial_menu_pos.x + current_position.x)-(ui_element->size.x/2);
			ui_element->pos.y = (s32)(memory->radial_menu_pos.y + current_position.y)-(ui_element->size.y/2);
		}

	}

	if(input->R > 0)
	{
		ENTITY_ELEMENT_TYPE possible_types_to_select[] = {
			EET_HEAL,
			EET_WATER,
			EET_HEAT,
			EET_COLD,
			EET_ELECTRIC,
		};

		char* button_text[] = {
			"heal",
			"water",
			"fire",
			"ice",
			"electric",
		};

		f32 angle_step = TAU32 / ARRAYCOUNT(button_text);
		V2 initial_position = {0,-MENU_RADIUS/3};

		UNTIL(i, ARRAYCOUNT(button_text))
		{
			if(exists_selected_entity && selected_entity->element != EET_NULL)
			{
				memory->ui_costs[ui_last] = 10;
			}
 
			if(ui_last == memory->ui_clicked_uid)
			{
				if(exists_selected_entity)
				{
					if(selected_entity->element != EET_NULL)
					{
						selected_entity->element = possible_types_to_select[i];
					}
					else if(memory->teams_resources[player_entity->team_uid] >= memory->ui_costs[ui_last])
					{
						memory->teams_resources[player_entity->team_uid] -= memory->ui_costs[ui_last];

						if(selected_entity->element == possible_types_to_select[i])
						{
							selected_entity->element = EET_NULL;
						}else{
							selected_entity->element = possible_types_to_select[i];
						}
					}
				}
			}


			Ui_element* ui_element = &ui_elements[ui_last++];
			
			
			if(exists_selected_entity){
				if(selected_entity->element == possible_types_to_select[i]){
					ui_element->color = {1.0f,1.0f,1.0f,1};
				}else{
					ui_element->color = {0.6f,0.6f,0.6f,1};
				}
			}else{
				ui_element->color = {0.2f,0.2f,0.2f,1};
			}

			ui_element->flags = 1;
			ui_element->text = string(button_text[i]); 
			
			V2 current_position = v2_rotate(initial_position, i*angle_step);

			ui_element->size.x = 110;
			ui_element->size.y = 50;

			ui_element->pos.x = (s32)(memory->radial_menu_pos.x + current_position.x)-(ui_element->size.x/2);
			ui_element->pos.y = (s32)(memory->radial_menu_pos.y + current_position.y)-(ui_element->size.y/2);
		}

	}
	
	
	// highlight selected buttons

	if(ui_hot_element >= 0){
		ui_elements[ui_hot_element].color = colors_product(ui_elements[ui_hot_element].color, {0.7f,0.7f,1.0f,1.0f});
	}
	if(memory->ui_pressed_uid >= 0){
		ui_elements[memory->ui_pressed_uid].color = {1,1, 0.5f, 1};
	}
	



	//
	// END OF UI_CODE
	//

	
	// GAME PAUSED SO SKIP UPDATING
	if(input->debug_increase_spawn_charges == 1)
	{
		memory->team_spawn_charges[0]++;
	}

	if(input->pause == 1) memory->is_paused = !memory->is_paused;

	if(input->debug_up) memory->teams_resources[0]++;

	// if(input->debug_left == 1 && exists_selected_entity) selected_entity->action_range /= 2;
	// if(input->debug_right == 1 && exists_selected_entity) selected_entity->action_range *= 2;

	// UPDATE 1 FRAME IF THE KEY IS TAPPED
	if(skip_updating(memory)) return;

	memory->time_s += memory->delta_time;

	// ADD RESOURCES OVER TIME
	
	UNTIL(i, 2)
	{
		memory->teams_resources[i] += world_delta_time*memory->base_resources_per_second;
	}

	// ADD SPAWN CHARGES OVER TIME
	
	if(memory->team_spawn_charges[0] < 3 && global_player_handle.is_valid(generations))
	{
		memory->add_spawn_charge_timer -= world_delta_time;
		if(memory->add_spawn_charge_timer <= 0)
		{
			memory->add_spawn_charge_timer += 3;
			memory->team_spawn_charges[0]++;
		}
	}



	// MOVE SELECTED ENTITY

	V2 input_vector = {(f32)(holding_inputs->d_right - holding_inputs->d_left),(f32)(holding_inputs->d_up - holding_inputs->d_down)};
	input_vector = v2_normalize(input_vector);
	if(exists_selected_entity && selected_entity->flags & E_CAN_MANUALLY_MOVE ){
		selected_entity->normalized_accel = v3_normalize(input_vector.x, 0, input_vector.y);
	}

	// MOVE CAMERA IN THE DIRECTION I AM LOOKING
	// V2 looking_direction = {cosf(memory->camera_rotation.y), sinf(memory->camera_rotation.y)};
	// V2 move_direction = {
	// 	input_vector.x*looking_direction.x + input_vector.y*looking_direction.y ,
	// 	-input_vector.x*looking_direction.y + input_vector.y*looking_direction.x
	// };
	// memory->camera_pos.x += move_direction.x * delta_time * camera_speed;
	// memory->camera_pos.z += move_direction.y * delta_time * camera_speed;

	// memory->camera_pos.y += (input->up - input->down) * delta_time * camera_speed;


	s32 hot_entity_uid = -1;
	V3 cursor_screen_to_world_pos = memory->camera_pos + v3_rotate_y(
		v3_rotate_x(screen_cursor_pos, memory->camera_rotation.x),memory->camera_rotation.y
	);

	V3 z_direction = v3_rotate_y(
		v3_rotate_x({0,0,1}, memory->camera_rotation.x),memory->camera_rotation.y
	);

	V3 cursor_y0_pos = line_intersect_y0(cursor_screen_to_world_pos, z_direction);

	struct DOUBLE_HANDLE{
		Entity_handle from;
		Entity_handle to;
	};
	// this is to check that both the entity being grabbed and the grabber are both still alive
	LIST(DOUBLE_HANDLE, update_grabbing_list) = {0};
	LIST(s32, entities_to_kill) = {0};
	LIST(Entity, entities_to_create) = {0};
	LIST(u32, set_jump_change_direction_list) = {0};


	//PROCESSING LEVEL STATES / BOSS ENTITY

	if(global_boss_handle.is_valid(generations) && memory->current_level)
	{
		memory->level_state.boss_timer += world_delta_time;

		if(memory->current_level == 1)
		{
			if(memory->level_state.boss_phase == 0)
			{

				switch(memory->level_state.boss_state)
				{
					case 0:
					{
						if(memory->level_state.boss_timer >= 5)
						{
							memory->level_state.boss_state++;
							memory->level_state.boss_timer -= 5;
						}
					}
					break;
					case 1:
					{
						memory->level_state.boss_state--;
						Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);

						// behavior:
							// target closest
							// target player
							// protect boss
						u64 extra_flags = 0;
						u32 dice = (u32)(rng->next(4));
						f32 speed_multiplier = 1.0f;

						switch(dice)
						{
							case 0:
							{
								extra_flags |= E_DEFEND_BOSS|E_AUTO_AIM_CLOSEST|E_HAS_SHIELD;
							}break;
							case 1:
							{
								extra_flags |= E_AUTO_AIM_BOSS|E_FOLLOW_TARGET;
								speed_multiplier = 2.0f;
							}break;
							case 2:
							case 3:
							default:
							{
								extra_flags |= E_AUTO_AIM_CLOSEST|E_FOLLOW_TARGET;	
							}
							break;
						}
						u64 flags = extra_flags | DEFAULT_UNIT_FLAGS;

						ENTITY_ELEMENT_TYPE possible_elements [] = {EET_NULL, EET_COLD, EET_ELECTRIC, EET_HEAT, EET_WATER};
						ENTITY_ELEMENT_TYPE element_type = possible_elements[(u32)(rng->next((f32)ARRAYCOUNT(possible_elements)))];


						V3 spawn_distance_vector = {-boss_entity->action_range, 0, 0};
						f32 random_angle = rng->next(PI32) - (PI32/2);
						V3 new_entity_pos = boss_entity->pos + v3_rotate_y(spawn_distance_vector, random_angle);
						

						new_entity->fill_entity(
							flags,
							element_type,
							new_entity_pos,
							player_entity->pos,
							speed_multiplier*15,
							40,
							5,
							(0.9f + rng->next(0.2f)) * 3.0f,
							global_boss_handle,
							boss_entity->team_uid,
							memory->meshes.blank_entity_mesh_uid,
							memory->textures.test_tex_uid 
						);	
						
					}
					break;
					case 2:
					{

					}
					break;
					case 3:
					{
					}
					break;
					default:
						ASSERT(false);
					break;
				}
				
				if(boss_entity->health / boss_entity->max_health < 0.25f)
				{
					memory->level_state.boss_phase = 1;
					memory->level_state.boss_state = 0;
					memory->level_state.boss_timer = 0;
				}
			}
			else if(memory->level_state.boss_phase == 1)
			{
				switch(memory->level_state.boss_state)
				{
					case 0:
					{
						memory->level_state.boss_state++;

						UNTIL(i, 3)
						{
							Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);
							
							V3 spawn_distance_vector = {-boss_entity->action_range, 0, 0};
							f32 random_angle = rng->next(PI32) - (PI32/2);
							V3 new_entity_pos = boss_entity->pos + v3_rotate_y(spawn_distance_vector, random_angle);

							u64 extra_flags = E_AUTO_AIM_BOSS|E_FOLLOW_TARGET;

							new_entity->fill_entity(
								DEFAULT_UNIT_FLAGS|extra_flags,
								EET_HEAL,
								new_entity_pos,
								player_entity->pos,
								20.0f,
								30.0f,
								10.0f,
								0.5f,
								global_boss_handle,
								boss_entity->team_uid,
								memory->meshes.blank_entity_mesh_uid,
								memory->textures.test_tex_uid
							);
						}

						UNTIL(i, 3)
						{
							Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);
							
							V3 spawn_distance_vector = {-boss_entity->action_range, 0, 0};
							f32 random_angle = rng->next(PI32) - (PI32/2);
							V3 new_entity_pos = boss_entity->pos + v3_rotate_y(spawn_distance_vector, random_angle);

							u64 extra_flags = E_AUTO_AIM_CLOSEST|E_DEFEND_BOSS;

							new_entity->fill_entity(
								DEFAULT_UNIT_FLAGS|extra_flags,
								EET_NULL,
								new_entity_pos,
								player_entity->pos,
								20.0f,
								30.0f,
								5.0f,
								0.5f,
								global_boss_handle,
								boss_entity->team_uid,
								memory->meshes.blank_entity_mesh_uid,
								memory->textures.test_tex_uid
							);
						}
						
					}
					break;
					case 1:
					{
						if(2.0f < memory->level_state.boss_timer )
						{
							memory->level_state.boss_state++;
							memory->level_state.boss_timer = 0;
						}
					}
					break;
					case 2:
					{
						memory->level_state.boss_state--;

						Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);
						
						V3 spawn_distance_vector = {-boss_entity->action_range, 0, 0};
						f32 random_angle = rng->next(PI32) - (PI32/2);
						V3 new_entity_pos = boss_entity->pos + v3_rotate_y(spawn_distance_vector, random_angle);

						u64 extra_flags = E_AUTO_AIM_BOSS|E_FOLLOW_TARGET;

						new_entity->fill_entity(
							DEFAULT_UNIT_FLAGS|extra_flags,
							EET_NULL,
							new_entity_pos,
							player_entity->pos,
							50.0f,
							5.0f,
							10.0f,
							0.5f,
							global_boss_handle,
							boss_entity->team_uid,
							memory->meshes.blank_entity_mesh_uid,
							memory->textures.test_tex_uid
						);

					}
					break;
					default:
						ASSERT(false);
					break;
				}
			}

		}
		else if(memory->current_level == 2)
		{
			if(memory->level_state.boss_timer >= 3.0f)
			{
				memory->level_state.boss_timer -= 3.0f;
				

				Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);
				
				V3 spawn_distance_vector = {-boss_entity->action_range, 0, 0};
				f32 random_angle = rng->next(PI32) - (PI32/2);
				V3 new_entity_pos = boss_entity->pos + v3_rotate_y(spawn_distance_vector, random_angle);

				u64 extra_flags = E_AUTO_AIM_BOSS|E_FOLLOW_TARGET;

				new_entity->fill_entity(
					DEFAULT_UNIT_FLAGS|extra_flags,
					EET_NULL,
					new_entity_pos,
					player_entity->pos,
					50.0f,
					5.0f,
					10.0f,
					0.5f,
					global_boss_handle,
					boss_entity->team_uid,
					memory->meshes.blank_entity_mesh_uid,
					memory->textures.test_tex_uid
				);

			}
		}else{

		}
	}


	// MAIN ENTITY UPDATE LOOP

	memory->debug_active_entities_count = 0;
	f32 closest_t = {0}; // this is used for raycasting
	b32 first_intersection = false;
	if(memory->closest_entity_to_grab_h.index == memory->selected_entity_h.index){
		memory->closest_entity_to_grab_h = {0};
	}
	Entity_handle last_frame_closest_entity_to_grab = memory->closest_entity_to_grab_h;
	memory->is_valid_grab = true;
	UNTIL(i, MAX_ENTITIES)
	{
		
		if(!entities[i].flags) continue;

		memory->debug_active_entities_count++;


		// CREATION TIME
		// i don't like that this must be done even with entities that skip_updating but whatevs

		//TODO: the player is not being set at 1.0f but at 1.088888888f
		if(entities[i].creation_size < 1.0f)
		{
			if(entities[i].creation_delay <= 0){
				entities[i].creation_size = 1.0f;
			}else{
				entities[i].creation_size += (1-entities[i].creation_size) / (entities[i].creation_delay/world_delta_time);
				entities[i].creation_delay -= world_delta_time;
			}
		}


		if(entities[i].flags & E_SKIP_UPDATING) continue;
		Entity* entity = &entities[i];

		f32 entity_dt = world_delta_time;

		if(entity->elemental_effects[EET_COLD])
		{
			entity_dt = world_delta_time * 0.65f;
		}
		if(entity->elemental_effects[EET_HEAT])
		{
			f32 heat_damage = 2.0f*world_delta_time;
			entity->health = MAX(world_delta_time, entity->health - heat_damage);
		}

		{
			Color target_color;

			// ENEMY_COLOR
			if(entity->team_uid != player_entity->team_uid){
				target_color = Color {.3f, .0f, .0f,1};
			}else{
				target_color = Color {.5f, .99f, .5f, 1};
			}
			Color delta_color = target_color - entity->color;
			delta_color = 3.0f*world_delta_time*delta_color;
			entity->color = entity->color + (delta_color);		
		}


		//SHRINK WITH LIFETIME

		if(entity->flags & E_SHRINK_WITH_LIFETIME)
		{
			f32 next_scale = (1-(0.2f*entity_dt/entity->lifetime));
			if(next_scale > 0){
				entity->scale = next_scale*entity->scale;
			}
		}

		// LIFETIME

		if(entity->lifetime)
		{
			entity->lifetime -= world_delta_time;
			if(entity->lifetime <= 0 )
			{
				s32* entity_index; PUSH_BACK(entities_to_kill, memory->temp_arena, entity_index);
				*entity_index = i;
				entity->flags &= (~E_SELECTABLE);
			}
		}
		

		// TOXIC LIFETIME

		if(entity->toxic_time_left)
		{
			entity->toxic_time_left = MAX(0, entity->toxic_time_left-entity_dt);
		}
		

		// APPLYING TOXIC DAMAGE 

		// TODO: i think this should be done when assigning the flag
		// but my current ui system is kinda tedious
		if(entity->flags & E_TOXIC_EMITTER){
			entity->toxic_time_left = 0;
		}else if(entity->toxic_time_left){
			if(!(entity->flags & E_TOXIC_DAMAGE_INMUNE))
			{
				entity->health -= 5*world_delta_time;
				if(entity->health <= 0){
					s32* index_to_kill; PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
					*index_to_kill = i;
				}
			}
		}

		
		// FREEZE TIME

		if(entity->freezing_time_left){
			entity->freezing_time_left = MAX(0, entity->freezing_time_left - entity_dt);
		}

		// GRAVITY FIELD TIME 

		if(entity->gravity_field_time_left){
			entity->gravity_field_time_left = MAX(0, entity->gravity_field_time_left - entity_dt);
			entity->gravity_field_radius = entity->gravity_field_radius * (1-(0.2f*entity_dt/entity->gravity_field_time_left));
		}

		// SHIELD COOLDOWN

		if(entity->flags & E_HAS_SHIELD)
		{
			if(!entity->shield_active)
			{
				entity->shield_cd = MAX(0, entity->shield_cd-entity_dt);

				if(entity->shield_cd <= 0)
				{
					entity->shield_active = true;
					Entity* new_shield; PUSH_BACK(entities_to_create, memory->temp_arena, new_shield);

					new_shield->flags = 
						E_VISIBLE|E_NOT_TARGETABLE|E_RECEIVES_DAMAGE|P_SHIELD|E_IGNORE_ALLIES|
						E_TOXIC_DAMAGE_INMUNE|E_STICK_TO_ENTITY|E_SKIP_DYNAMICS
					;
					
					new_shield->max_health = 10.0f;
					new_shield->health = new_shield->max_health;

					//speed could be by the one who shoots
					new_shield->speed = 40.0f;
					new_shield->friction = 4.0f;
					new_shield->weight = 1.0f;
					// this is obsolete but useful for debug
					new_shield->mesh_uid = memory->meshes.shield_mesh_uid;
					new_shield->texinfo_uid = memory->textures.white_tex_uid;
					new_shield->color = {0.0f,0.6f,0.6f,0.5};
					new_shield->scale = {2.0f,2.0f,2.0f};

					new_shield->creation_delay = 0.5f;

					new_shield->aura_radius = 2.0f;

					Entity* parent = entity;
					new_shield->parent_handle.index = i;
					new_shield->parent_handle.generation = generations[i];
					new_shield->grabbed_entity = new_shield->parent_handle;
					new_shield->team_uid = parent->team_uid;
					new_shield->pos = parent->pos;
					// TODO: go in the direction that parent is looking (the parent's rotation);
				}

			}
			
		}

		// FOG DEBUFF TIME

		if(entity->fog_debuff_time_left)
		{
			entity->fog_debuff_time_left = MAX(0, entity->fog_debuff_time_left - world_delta_time);
			if(entity->fog_debuff_time_left)
			{
				entity->color = 0.3f*entity->color;
			}
		}

		// ELEMENTAL EFFECTS DURATIONS

		UNTIL(element_index, 4)
		{
			
			if(entity->elemental_effect_cooldowns[element_index])
			{
				entity->elemental_effect_cooldowns[element_index] = 
					MAX(0, entity->elemental_effect_cooldowns[element_index]-world_delta_time);
			}else{
				entity->elemental_effects[element_index] -= world_delta_time;
				if(entity->elemental_effects[element_index] <= 0)
				{
					entity->elemental_effects[element_index] = 0;
					entity->triggered_elements_flag &= ~element_to_element_flag(element_index);
				}
			}
		}

		// GENERATE RESOURCES OVER TIME

		if(entity->flags & E_GENERATE_RESOURCE)
		{
			memory->teams_resources[entity->team_uid] += entity_dt;
		}


		// PROJECTILE IGNORE SPHERE

		if(entity->ignore_sphere_radius){
			entity->ignore_sphere_radius = (entity->ignore_sphere_radius-world_delta_time)*0.89f;
			if(entity->ignore_sphere_radius <= 0){
				entity->ignore_sphere_radius = 0;
			}else{
				entity->ignore_sphere_pos = entity->ignore_sphere_pos + (0.1f*(entity->ignore_sphere_target_pos - entity->ignore_sphere_pos));
			}

		}

		// CURSOR RAYCASTING

		if(!is_cursor_in_ui){
			if((entity->flags & E_SELECTABLE) &&
				entity->team_uid == player_entity->team_uid 
			){	
				f32 intersected_t = 0;
				if(line_vs_sphere(cursor_screen_to_world_pos, z_direction, 
					entity->pos, entity->scale.x, 
					&intersected_t)
				){
					if(!first_intersection){
						first_intersection = true;
						closest_t = intersected_t;
						hot_entity_uid = i;
					}
					if(intersected_t < closest_t){
						closest_t = intersected_t;
						hot_entity_uid = i;
					}
				}
			}
		}


		// MOVE TOWARDS TARGET
		
		V3 entity_target_direction = entity->target_pos-entity->pos;

		if(entity->flags & E_FOLLOW_TARGET && !entity->freezing_time_left)
		{
			if(!(entity->flags & E_CAN_MANUALLY_MOVE) || (!input_vector.x && !input_vector.y) )
			{
				f32 target_magnitude = v3_magnitude(entity_target_direction);
				if(target_magnitude)
				{
					f32 range_magnitude = calculate_total_range(entity)*2;
					f32 new_target_magnitude = target_magnitude - range_magnitude;

					if(ABS(new_target_magnitude) > 2.0f)
					{
						entity->normalized_accel = ((new_target_magnitude/ABS(new_target_magnitude))/target_magnitude)*entity_target_direction;
					}
				}
				else
				{
					entity->normalized_accel = {0,0,0};
				}
			}
		}


		// MOVEMENT / DYNAMICS

		if(!(entity->flags & (E_SKIP_DYNAMICS)))
		{
			f32 paralysis_multiplier = (entity->elemental_effects[EET_ELECTRIC]) ? 0.1f : 1.0f;
			V3 result_acceleration = calculate_delta_velocity(
				entity->velocity, 
				paralysis_multiplier*entity->speed*(!entity->freezing_time_left)*entity->normalized_accel, 
				entity->friction);

			entity->velocity = entity->velocity + (entity_dt*result_acceleration);
			f32 min_threshold = 0.1f;

			// UPDATING LOOKING DIRECTION

			if( // it is not moving
				(entity->normalized_accel.x < min_threshold && entity->normalized_accel.x > -min_threshold) &&
				(entity->normalized_accel.z < min_threshold && entity->normalized_accel.z > -min_threshold)
			){
				// look at the target
				//TODO: handle case when entity->target_direction == 0
				V3 delta_looking_direction = (10*entity_dt*(entity_target_direction - entity->looking_direction));
				entity->looking_direction = entity->looking_direction + delta_looking_direction;
				//slowing_down
			}else{
				if(entity->flags & E_LOOK_IN_THE_MOVING_DIRECTION)
				{
					// look in the moving direction
					//TODO: handle case when entity->velocity == 0
					V3 delta_looking_direction = (10*entity_dt*(entity->velocity - entity->looking_direction));
					entity->looking_direction = entity->looking_direction + delta_looking_direction;
				}
				else // DEFAULT CASE
				{ 
					// look at the target
					//TODO: handle case when entity->target_direction == 0
					V3 delta_looking_direction = (10*entity_dt*(entity_target_direction - entity->looking_direction));
					entity->looking_direction = entity->looking_direction + delta_looking_direction;
				}
			}
		}

		entity->normalized_accel = {0,0,0};

		// TURN TOWARDS PARENT TARGET POS

		if(entity->flags & E_TURN_TOWARDS_PARENT_TARGET_POS &&
			entity->parent_handle.is_valid(generations))
		{
			Entity* parent = &entities[entity->parent_handle.index];

			
			f32 velocity_angle = v2_angle({entity->velocity.x, entity->velocity.z});
			f32 target_angle = v2_angle({
				parent->target_pos.x - entity->pos.x - entity->velocity.x,
				parent->target_pos.z - entity->pos.z - entity->velocity.z,
			});
			
			f32 angle_difference = get_shortest_angle_difference(target_angle, velocity_angle);
			
			if(COMPARE_FLOATS(angle_difference, PI32))
			{
				f32 dice = rng->next(2);
				if(dice < 1){
					angle_difference = -PI32;
				}
			}
			f32 delta_angle = world_delta_time*(angle_difference);
			//inverted delta angle cuz of the inverted y axis angle
			entity->velocity = v3_rotate_y(entity->velocity, -delta_angle);
		}

		
		// SIZE = VELOCITY

		if(entity->flags & E_SHRINK_WITH_VELOCITY)
		{
			// f32 speed_left = v3_magnitude(entity->velocity) / entity->speed;
			// f32 new_scale = SQRT(speed_left);
			f32 minimum_speed = 1.0f;
			f32 speed_left = v3_magnitude(entity->velocity);
			f32 scale_multiplier = (1-(20.0f*entity_dt/(speed_left-minimum_speed)));
			if(speed_left <= minimum_speed || 1 < scale_multiplier){
				s32* index_to_kill;PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
				*index_to_kill = i;
			}else{
				entity->creation_size = entity->creation_size*scale_multiplier;
			}
		}
		
		
		// ROTATION / LOOKING DIRECTION
		if(!(entity->flags & E_SKIP_ROTATION) && !entity->freezing_time_left){
			// this is negative cuz +y is up while we are looking down
			entity->rotation.y = - (v2_angle({entity->looking_direction.x, entity->looking_direction.z}) + PI32/2); 
			entity->rotation.x = entity->velocity.z/10;
			entity->rotation.z = -entity->velocity.x/10;
		}
		

		// SUB ITERATION

		// TODO:initialize distances to 0 and instead of checking if uid < 0 check for distance
		s32 closest_enemy_uid = 0;
		f32 closest_enemy_distance = 0;

		s32 closest_ally_uid = 0;
		f32 closest_ally_distance = 0;

		s32 closest_jump_target_uid = 0;
		f32 closest_jump_distance = 0;


		//TODO: check if the entity i am processing here is the same that i will check when F == -1
		// cuz here i may be checking last frame's closest entity
		
		b32 test_if_this_entity_can_be_grabbed = false;
		V3 grab_pos = {0};
		if(last_frame_closest_entity_to_grab.index == i)
		{
			if(
				exists_selected_entity && input->F && 
				last_frame_closest_entity_to_grab.is_valid(generations) &&
				!(selected_entity->flags & E_STICK_TO_ENTITY)
			){
				test_if_this_entity_can_be_grabbed = true;
				V3 distance_vector = entity->pos - selected_entity->pos;
				V3 final_relative_pos = (0.5f + selected_entity->scale.x + entity->scale.x) * v3_normalize(distance_vector);
				grab_pos = selected_entity->pos + final_relative_pos;

			}else{
				memory->is_valid_grab = false;
			}
		}

		f32 closest_entity_to_grab_distance;
		if(memory->selected_entity_h.index == i && last_frame_closest_entity_to_grab.is_valid(generations))
		{
			closest_entity_to_grab_distance = v3_magnitude(entities[last_frame_closest_entity_to_grab.index].pos - entity->pos);
		}else{
			closest_entity_to_grab_distance = MAX_FLOAT;	
		}
		
		// ALMOST ALL ENTITIES DO SOME OF THESE SO I DON'T KNOW HOW MUCH THIS OPTIMIZES ANYTHING
		// if(entity->flags & (E_RECEIVES_DAMAGE|E_AUTO_AIM_CLOSEST|E_DETECT_COLLISIONS))
		UNTIL(j, MAX_ENTITIES)
		{
			if(!entities[j].flags)continue;
			if(j == i) continue;
			//TODO: walls/obstacles should not be skipping this 
			// cuz they need to be detected by the thing that collided with them
			if(entities[j].flags & E_SKIP_UPDATING) continue;
			
			Entity* entity2 = &entities[j];
			V3 distance_vector = entity2->pos - entity->pos;
			f32 distance = v3_magnitude(distance_vector);

			
			if(entity->team_uid != entity2->team_uid)
			{
				//AUTOAIM 


				if(entity->flags & E_AUTO_AIM_CLOSEST && !(entity2->flags & E_NOT_TARGETABLE))
				{
					if(!closest_enemy_distance){
						closest_enemy_uid = j;
						closest_enemy_distance = distance;
					}else if(distance < closest_enemy_distance){
						closest_enemy_uid = j;
						closest_enemy_distance = distance;
					}
				}
				
				// POSSIBLE JUMPING TARGETS

				if(entity->flags & P_JUMP_BETWEEN_TARGETS && !(entity2->flags & E_NOT_TARGETABLE)){
					if(v3_magnitude(entity2->pos - entity->ignore_sphere_pos) > entity->ignore_sphere_radius){
						if(!closest_jump_distance){
							closest_jump_target_uid = j;
							closest_jump_distance = distance;
						}else if(distance < closest_jump_distance){
							closest_jump_target_uid = j;
							closest_jump_distance = distance;
						}
					}
				}
			}
			else
			{
				if((entity->element == EET_HEAL) && 
					entity->flags & E_AUTO_AIM_CLOSEST && 
					!(entity2->flags & E_NOT_TARGETABLE))
				{
					if(!closest_ally_distance){
						closest_ally_uid = j;
						closest_ally_distance = distance;
					}else if(distance < closest_ally_distance){
						closest_ally_uid = j;
						closest_ally_distance = distance;
					}
				}
				
				// CLOSEST GRAB ENTITY 
				if(
					i == memory->selected_entity_h.index && 
					!(entity2->flags & (E_STICK_TO_ENTITY|E_DOES_DAMAGE)) &&
					!entity2->is_grabbing && 
					j != global_player_handle.index
				)
				{
					if(distance < closest_entity_to_grab_distance)
					{
						memory->closest_entity_to_grab_h = {j, generations[j]};
						closest_entity_to_grab_distance = distance;
					}
				}

			}
			

			// CHECKING IF THE AREA WHERE THE ENTITY WILL BE PLACED AFTER GRABBING IT IS CLEARED

			if(test_if_this_entity_can_be_grabbed && 
				entity2->flags & E_HAS_COLLIDER &&
				entity2->flags & E_STICK_TO_ENTITY && 
				entity2->grabbed_entity == memory->selected_entity_h)
			{
				V3 owner_target_vector = selected_entity->target_pos - selected_entity->pos;
				f32 offset_angle = v2_angle({owner_target_vector.x, owner_target_vector.z});
				f32 relative_distance = 0.5f + entity2->scale.x + selected_entity->scale.x;
				V3 grabbed_entity_relative_pos = 
					v3_rotate_y(
						{relative_distance, 0, 0}, 
						-(entity2->relative_angle + offset_angle)
					);
				V3 grabbed_entity_world_pos = grabbed_entity_relative_pos + selected_entity->pos;
				if(
					0 <= sphere_vs_sphere(
						grab_pos, entity->creation_size*entity->scale.x, 
						grabbed_entity_world_pos, entity2->scale.x
					)
				){
					memory->is_valid_grab = false;
				}
			}


			// HITBOX / DAMAGE 

			if(entity->flags & E_RECEIVES_DAMAGE)
			{
				if(
					entity2->flags & E_DOES_DAMAGE &&
					// entity->parent_handle != entity2->parent_handle &&
					(
						!(entity2->flags & E_SKIP_PARENT_COLLISION) ||
						entity2->parent_handle.index != i ||
						entity2->parent_handle.generation != generations[i]
					) && (
						entity->team_uid != entity2->team_uid ||
						(
							!(entity->flags & E_IGNORE_ALLIES) &&
							!(entity2->flags & E_IGNORE_ALLIES)
						)
					) &&(
						!entity2->ignore_sphere_radius ||
						v3_magnitude(entity->pos - entity2->ignore_sphere_pos) > entity2->ignore_sphere_radius
					)
				){
					b32 they_collide = false;

					if(entity->collider_type == COLLIDER_TYPE_SPHERE){
						if(entity2->collider_type == COLLIDER_TYPE_SPHERE){ // BOTH SPHERES
							f32 r1 = entity->scale.x*entity->creation_size;
							f32 r2 = entity2->scale.x*entity2->creation_size;
							f32 overlapping =   (r1 + r2) - distance;

							if(overlapping > 0){
								they_collide = true;									
							}
						}else{ // E1 SPHERE E2 CUBE
							//TODO: handle sphere vs cube case
							ASSERT(false);
						}
					}else{
						if(entity2->collider_type == COLLIDER_TYPE_SPHERE){ // E1 CUBE E2 SPHERE
							//TODO: handle cube vs sphere case
							ASSERT(false);
						}else{ // BOTH CUBES
							//TODO: handle cube vs cube case
							ASSERT(false);
						}
					}
					
					if(they_collide) // DAMAGING ENTITY
					{
						if(entity->flags & P_SHIELD)
						{
							entity->health -= entity2->total_power;
							if(entity->health <= 0){
								s32* index_to_kill;
								PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
								*index_to_kill = i;

								Entity* parent = entity_from_handle(entity->parent_handle, entities, generations);
								parent->shield_active = false;
								parent->shield_cd = 8.0f;
							}
							entity2->flags = 0;
							s32* index_to_kill;
							PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
							*index_to_kill = j;
						}
						else // DEFAULT CASE
						{
							f32 damage_dealt;
							if(entity2->element == EET_HEAL){
								damage_dealt = -1;
								entity->color = {0,2,0.5f,1};
							}else{
								entity->color = {1,0,0,1};
								damage_dealt = MIN(entity->health, entity2->total_power);
							}
							entity->health = CLAMP(0, entity->health - damage_dealt, entity->max_health);
							if(entity->health <= 0)
							{
								s32* index_to_kill;
								PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
								*index_to_kill = i;
							}else{ // still alive
								// OLD WAY OF FREEZING
								if(entity2->flags & P_FREEZING){
									entity->freezing_time_left = 5.0f;
								}
								if(entity2->flags & P_SPAWN_GRAVITY_FIELD){
									entity->gravity_field_time_left = 10.0f;
								}
							}
							// CHEKING IF A ELEMENTAL REACTION OCURRED

							calculate_elemental_reaction(entity, entity2, memory, entities_to_create);

							u32* index_to_set;
							PUSH_BACK(set_jump_change_direction_list, memory->temp_arena, index_to_set);
							*index_to_set = j;

							//TODO: not very multithready friendly

							//TODO: push this code to a list to process this outside the main loop
							if(!entity2->ignore_sphere_radius){ // FIRST_COLLISION
								entity2->ignore_sphere_pos = entity->pos;
								entity2->ignore_sphere_radius = 0.9f;
							}else{
								V3 difference = entity->pos - entity2->ignore_sphere_pos;
								entity2->ignore_sphere_pos = entity2->ignore_sphere_pos + (difference/2);
								entity2->ignore_sphere_radius = MAX(0.9f,entity2->ignore_sphere_radius + v3_magnitude(difference)/2);
							}
							entity2->ignore_sphere_target_pos = entity->pos;

							entity2->health--;
							if(entity2->health <= 0)
							{
								s32* index_to_kill;
								PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
								*index_to_kill = j;
							}
							if(entity2->flags & A_STEAL_LIFE)
							{
								Entity* parent = entity_from_handle(entity2->parent_handle, entities, generations);
								f32 life_steal_value = MAX(damage_dealt, 0);
								parent->health = CLAMP(0, parent->health+life_steal_value, parent->max_health);
							}
							
							Particle_emitter particle_emitter = {0};
							particle_emitter.particle_flags = PARTICLE_ACTIVE;
							particle_emitter.particles_count = 1;
							particle_emitter.emit_cooldown = memory->delta_time*2;

							particle_emitter.velocity_yrotation_rng = TAU32;
							particle_emitter.friction = 10.0f;
							
							particle_emitter.acceleration = {0,10.0f, 0};
							
							particle_emitter.color_rng = {0.2f, 0.2f, 0.2f};
							particle_emitter.target_color = {1.0f,1.0f,1.0f,0};
							particle_emitter.color_delta_multiplier = 0.5f;

							particle_emitter.particle_lifetime = 0.3f;

							particle_emitter.initial_angle_rng = PI32;
							particle_emitter.angle_initial_speed_rng = 20.0f;
							particle_emitter.angle_friction = 4.0f;
							
							particle_emitter.initial_scale = 0.2f;
							particle_emitter.target_scale = 0.0f;
							particle_emitter.scale_delta_multiplier = 0.5f;

							Color particles_color;
							if(damage_dealt < 0)
							{
								particles_color = {.2f, 1, .2f, 1};
							}else{
								particles_color = {1,1,1,1};
							}

							UNTIL(particle_index, 10)
							{
								Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
								particle_emitter.emit_particle(new_particle, entity2->pos, {50,0,0}, particles_color, rng);
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
								0.5f,
								0,
								.0f,
								1.0f
							);
							particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
								entity2->pos,{0}, {1,1,1,1}, &memory->rng);
						}
					}
				}

				
				// TOXIC ENTITIES INFECTING

				#define DEFAULT_TOXIC_RADIUS 4.5f
				if((entity2->flags & E_TOXIC_EMITTER) && 
					!(entity->flags & E_TOXIC_EMITTER) &&
					!(entity->flags & P_SHIELD)
				){
					if(distance < entity->aura_radius){
						entity->toxic_time_left = 5.0f;
					}
				}else if(entity->toxic_time_left < entity2->toxic_time_left && !(entity->flags & E_TOXIC_EMITTER))
				{
					if(distance < DEFAULT_TOXIC_RADIUS){
						entity->toxic_time_left = MAX(0, entity2->toxic_time_left-entity_dt);
					}
				}


				// HEALING

				if(entity2->element == EET_HEAL 
					&& !(entity->flags & P_SHIELD) 
					&& !(entity->element == EET_HEAL)
				){
					if(distance < entity->aura_radius)
					{	
						if(entity->health < entity->max_health){
							f32 healing_value = entity_dt * calculate_power(entity2);
							entity->health = MIN(entity->health+healing_value, entity->max_health);

							if(rng->time_dice(7.5f, world_delta_time))
							{
								Particle_emitter particle_emitter;
								particle_emitter.fill_data(
									PARTICLE_ACTIVE,
									1,
									0,
									0,
									{0},
									{0},
									TAU32,
									1.0f,
									10.0f,
									{0,-10.0f,0},
									{0},
									{0,1,0,0.1f},
									1.0f,
									0.5f,
									0,0,0,0,0,
									0.1f,
									0,
									0.3f,
									1.0f
								);
								UNTIL(particle, particle_emitter.particles_count)
								{
									particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
										entity->pos, {40.0f, 50.0f,0}, {0, 0.5f, 0, 1.0f}, rng);
								}
							}
						}

					}
				}	


				// EXPLOSION

				if(entity2->flags & E_EXPLOSION && 
					!(entity->flags & P_SHIELD)
				){
					f32 explosion_radius = entity2->scale.x*entity2->creation_size;
					if(distance < explosion_radius)
					{
						f32 closeness_to_center = (1 - distance/explosion_radius);
						f32 result_damage = 5 + (10*closeness_to_center);
						entity->health = CLAMP(0, entity->health - (s32)result_damage, entity->max_health);
						// f32 outwards_force = 8*(explosion_radius-distance);
						f32 outwards_force = 1 + (15*closeness_to_center);
						entity->velocity = entity->velocity - (outwards_force*distance_vector);

						if(entity->health <= 0)
						{
							s32* index_to_kill;PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
							*index_to_kill = i;
						}
					}
				}
			}


			// BEING AFFECTED BY THE ENTITY'S GRAVITY FIELD
			//TODO: maybe this condition is redundant cuz gravity_field_radius is the important
			if(entity2->gravity_field_time_left)
			{
				if(distance < entity2->gravity_field_radius && entity->weight)
				{
					if(entity->flags & E_DOES_DAMAGE)
					{

						f32 velocity_angle = v2_angle({entity->velocity.x, entity->velocity.z});
						f32 distance_angle = v2_angle({distance_vector.x, distance_vector.z});
						
						f32 angle_difference = get_shortest_angle_difference(distance_angle, velocity_angle);
						
						if(COMPARE_FLOATS(angle_difference, PI32))
						{
							f32 dice = rng->next(2);
							if(dice < 1){
								angle_difference = -PI32;
							}
						}
						f32 delta_angle = 5.0f*world_delta_time* (angle_difference);
						//inverted delta angle cuz of the inverted y axis angle
						entity->velocity = v3_rotate_y(entity->velocity, -delta_angle);
					}else{
						entity->velocity = entity->velocity + (world_delta_time*distance_vector);
					}
					// V3 init_velocity = entity->velocity;
					// f32 entity_speed = MAX(v3_magnitude(entity->velocity), 0.5F);
					// //TODO: this is independent of entity_dt 
					// entity->velocity = entity_speed * v3_normalize(entity->velocity + (distance_vector/(20*entity->weight)));
					
					// if(init_velocity == entity->velocity)
					// {
					// 	f32 rotation_angle = rng->next(TAU32)-0.1f;
					// 	entity->velocity = v3_rotate_y(entity->velocity, rotation_angle);
					// }
					
				}
			}

			// BEING AFFECTED BY THE SMOKE SCREEN
			if(entity2->flags & E_SMOKE_SCREEN && 
				(entity->flags & E_RECEIVES_DAMAGE) &&
				!(entity->flags & E_DOES_DAMAGE) 
			){
				if(distance < (entity2->scale.x * entity2->creation_size))
				{
					entity->fog_debuff_time_left = 2*world_delta_time;
					calculate_elemental_reaction(entity, entity2, memory, entities_to_create);
				}
			}


			// COLLISIONS

			
			if(entity->flags & E_DETECT_COLLISIONS &&
				entity2->flags & E_HAS_COLLIDER && (
					!(entity->flags & E_SKIP_PARENT_COLLISION) ||
					entity->parent_handle.index != j ||
					entity->parent_handle.generation != generations[j]
				) && ( // this is so that piercing projectiles don't die
					!(entity->flags & E_DOES_DAMAGE) ||
					!(entity2->flags & E_RECEIVES_DAMAGE)
				)
			){
				//TODO: collision code
				b32 they_collide = false;

				if(entity->collider_type == COLLIDER_TYPE_SPHERE)
				{
					if(entity2->collider_type == COLLIDER_TYPE_SPHERE) // BOTH SPHERES
					{ 
						f32 r1 = entity->scale.x*entity->creation_size;
						f32 r2 = entity2->scale.x*entity2->creation_size;
						f32 overlapping =   (r1 + r2) - distance;

						if(overlapping > 0){
							they_collide = true;

							//TODO: maybe do this if creating entities inside another is too savage
							// overlapping =  MIN(MIN(entity->current_scale, entity2->current_scale),overlapping);
							V3 collision_direction = {0,0,1.0f};
							if(distance){
								collision_direction = (overlapping/(1.0f*distance))*distance_vector;
								// collision_direction = centers_distance / centers_distance_magnitude;
							}

							
							//TODO: this will need a rework cuz it is not multithread friendly
							// i should not modify other entities while processing another one
							// f32 momentum_i = MAX(v3_magnitude(entity->velocity), entity_dt);
							// f32 momentum_j = MAX(v3_magnitude(entity2->velocity), entity_dt);
							entity->velocity = entity->velocity - (collision_direction);
							// entity2->velocity = entity2->velocity + (collision_direction);

							ASSERT(!isnan(entity->velocity.x));
							ASSERT(isfinite(entity->velocity.x));
							ASSERT(isfinite(entity2->velocity.x));
							ASSERT(!isnan(entity2->velocity.x));

							ASSERT(!isnan(entity->velocity.z));
							ASSERT(isfinite(entity->velocity.z));
							ASSERT(isfinite(entity2->velocity.z));
							ASSERT(!isnan(entity2->velocity.z));
						}
					}else{ // E1 SPHERE E2 CUBE
						//TODO: handle sphere vs cube case
						ASSERT(false);
					}
				}else{
					if(entity2->collider_type == COLLIDER_TYPE_SPHERE){ // E1 CUBE E2 SPHERE
						//TODO: handle cube vs sphere case
						ASSERT(false);
					}else{ // BOTH CUBES
						//TODO: handle cube vs cube case
						ASSERT(false);
					}
				}


				if(they_collide)
				{
					if(entity->flags & E_DIE_ON_COLLISION){
						s32* index_to_kill; PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
						*index_to_kill = i;
					}
				} 
			}

			
		}	// END OF SUB ITERATION


		// PROJECTILE JUMPING

		if(entity->jump_change_direction && entity->flags & P_JUMP_BETWEEN_TARGETS){
			entity->jump_change_direction = false;
			if(closest_jump_distance)
			{
				entity->target_pos = entities[closest_jump_target_uid].pos;
				entity->velocity = v3_magnitude(entity->velocity) * v3_normalize(entity->target_pos - entity->pos);
			}else{
				s32* index_to_kill; 
				PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);

				*index_to_kill = i;
			}
		}


		// COOLDOWN ACTIONS 

		// if it is not 0
		if(!entity->freezing_time_left && entity->action_cd_total_time){
			entity->action_cd_time_passed += entity_dt;
			if(entity->action_cd_time_passed >= entity->action_cd_total_time){
				entity->action_cd_time_passed -= entity->action_cd_total_time;

				ASSERT(entity->action_angle >= 0 && entity->action_angle < TAU32);
				f32 angle_step;
				u32 repetitions = MAX(entity->action_count, 1);
				V3* target_directions = ARENA_PUSH_STRUCTS(memory->temp_arena, V3, repetitions);
				f32 looking_direction_length = v3_magnitude(entity->looking_direction);
				V3 normalized_looking_direction;
				if(looking_direction_length){
					normalized_looking_direction = entity->looking_direction / looking_direction_length;
				}else{
					normalized_looking_direction = {1,0,0};
				} 

				if(repetitions > 1){ //TODO: if angle = 360 then two actions will happen in the same spot behind 
					V3 current_target_direction = v3_rotate_y(normalized_looking_direction, -entity->action_angle/2);
					angle_step = entity->action_angle / (repetitions-1);
					UNTIL(current_angle_i, repetitions)
					{
						target_directions[current_angle_i] = current_target_direction;
						current_target_direction = v3_rotate_y(current_target_direction, angle_step);
					}
				}else{
					target_directions[0] = normalized_looking_direction;
					angle_step = 0;
				}

				if((entity->flags & E_SHOOT))
				{
					UNTIL(current_action_i, repetitions)
					{
						Entity* new_bullet; PUSH_BACK(entities_to_create, memory->temp_arena, new_bullet);

						new_bullet->flags = 
							E_VISIBLE|E_DETECT_COLLISIONS|E_DIE_ON_COLLISION|E_NOT_TARGETABLE
							|E_DOES_DAMAGE|E_UNCLAMP_XZ|E_SKIP_PARENT_COLLISION
							|E_TOXIC_DAMAGE_INMUNE|E_SHRINK_WITH_VELOCITY|E_EMIT_PARTICLES
							|E_TURN_TOWARDS_PARENT_TARGET_POS
						;

						if(!(entity->element == EET_HEAL))
						{
							new_bullet->flags |= E_IGNORE_ALLIES;
						}

						if(entity->flags & E_PROJECTILE_EXPLODE){
							new_bullet->flags |= P_PROJECTILE_EXPLODE;
						}
						if(entity->flags & E_HOMING_PROJECTILES){
							new_bullet->flags |= E_AUTO_AIM_CLOSEST;
						}
						if(entity->flags & E_FREEZING_ACTIONS){
							new_bullet->flags |= P_FREEZING;
						}
						if(entity->flags & E_LIFE_STEAL){
							new_bullet->flags |= A_STEAL_LIFE;
						}

						new_bullet->element = entity->element;
						new_bullet->elemental_damage_duration = 1.5f;
						
						new_bullet->max_health = 1;
						if(entity->flags & E_PROJECTILE_JUMPS_BETWEEN_TARGETS){
							new_bullet->flags |= P_JUMP_BETWEEN_TARGETS;
							new_bullet->max_health += 1;
						}
						// health is how many hits can it get before dying
						if(entity->flags & E_PROJECTILE_PIERCE_TARGETS){
							new_bullet->max_health += 4;
						}
						if(entity->flags & E_HIT_SPAWN_GRAVITY_FIELD){
							new_bullet->flags |= P_SPAWN_GRAVITY_FIELD;
						}
						new_bullet->health = new_bullet->max_health;

						//speed could be by the one who shoots
						new_bullet->speed = 80;
						new_bullet->friction = 30.0f / calculate_total_range(entity);

						new_bullet->lifetime = 5.0f;
						new_bullet->weight = 0.1f;

						new_bullet->mesh_uid = memory->meshes.ball_mesh_uid;
						new_bullet->texinfo_uid = memory->textures.white_tex_uid;
						new_bullet->color = {0.6f,0.6f,0.6f,1};
						new_bullet->scale = {0.4f,0.4f,0.4f};


						new_bullet->total_power = calculate_power(entity);

						new_bullet->aura_radius = 2.0f;

						Entity* parent = entity;
						new_bullet->parent_handle.index = i;
						new_bullet->parent_handle.generation = generations[i];
						new_bullet->team_uid = parent->team_uid;
						new_bullet->pos = parent->pos;
						// TODO: go in the direction that parent is looking (the parent's rotation);
						new_bullet->target_pos = target_directions[current_action_i] + new_bullet->pos;

						new_bullet->velocity =  (new_bullet->speed) * target_directions[current_action_i];
					}
				}
				if((entity->flags & E_MELEE_ATTACK))
				{	// spawn a pseudo entity that spawns hitboxes each 0.1 second depending on how much health it has
					UNTIL(current_action_i, repetitions)
					{
						Entity* hitbox; PUSH_BACK(entities_to_create, memory->temp_arena, hitbox);
						
						hitbox->flags = E_MELEE_HITBOX|E_VISIBLE|E_STICK_TO_ENTITY|
							E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_SKIP_PARENT_COLLISION;
						if(entity->flags & E_FREEZING_ACTIONS){
							hitbox->flags |= E_FREEZING_ACTIONS;
						}
						if(entity->flags & E_LIFE_STEAL){
							hitbox->flags |= E_LIFE_STEAL;
						}

						hitbox->max_health = 1;
						if(entity->flags & E_PROJECTILE_PIERCE_TARGETS){
							hitbox->max_health += 4;
						}
						hitbox->health = hitbox->max_health;

						hitbox->color = {0, 0, 0, 0.3f};
						//TODO: the size of the hitbox will change depending on the entity stats
						hitbox->scale = {1,1,1};
						hitbox->scale = 2*entity->action_range * hitbox->scale;
						hitbox->creation_size = 1.0f;

						hitbox->action_cd_total_time = 0.1f;

						hitbox->parent_handle.index = i;
						hitbox->parent_handle.generation = generations[i];

						hitbox->grabbed_entity = hitbox->parent_handle;
						hitbox->team_uid = entity->team_uid;

						hitbox->total_power = calculate_power(entity);
						hitbox->relative_angle = -(entity->action_angle/2) + (current_action_i*angle_step);

						f32 relative_distance = 0.5f + entity->scale.x + hitbox->scale.x;

						hitbox->pos = entity->pos + relative_distance*target_directions[current_action_i];

						// hitbox->mesh_uid = memory->meshes.centered_plane_mesh_uid;
						hitbox->mesh_uid = memory->meshes.icosphere_mesh_uid;
						hitbox->texinfo_uid = memory->textures.white_tex_uid;
					}
				}
				if(entity->flags & E_MELEE_HITBOX)
				{
					Entity* hitbox; PUSH_BACK(entities_to_create, memory->temp_arena, hitbox);
					
					hitbox->flags = E_VISIBLE|E_DOES_DAMAGE|
						E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_SKIP_PARENT_COLLISION;
					if(entity->flags & E_FREEZING_ACTIONS){
						hitbox->flags |= P_FREEZING;
					}
					if(entity->flags & E_LIFE_STEAL){
						hitbox->flags |= A_STEAL_LIFE;
					}

					hitbox->color = {1, 1, 1, 0.3f};
					//TODO: the size of the hitbox will change depending on the entity stats
					hitbox->scale = entity->scale;
					hitbox->creation_size = 1.0f;

					hitbox->lifetime = world_delta_time;

					hitbox->parent_handle = entity->parent_handle;

					hitbox->team_uid = entity->team_uid;

					hitbox->total_power = entity->total_power;

					hitbox->pos = entity->pos;

					// hitbox->mesh_uid = memory->meshes.centered_plane_mesh_uid;
					hitbox->mesh_uid = memory->meshes.icosphere_mesh_uid;
					hitbox->texinfo_uid = memory->textures.white_tex_uid;
					
					entity->health -= 1;
					if(entity->health <= 0){
						s32* index_to_kill;PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
						*index_to_kill = i;
					}
				}
			}
		}


		// UPDATE POSITION 

		if(entity->flags & E_STICK_TO_ENTITY)
		{
			if(entity->grabbed_entity.is_valid(generations) && (
					entities[entity->grabbed_entity.index].is_grabbing ||
					entity->flags & E_MELEE_HITBOX ||
					entity->flags & P_SHIELD
				)
			){
				Entity* sticked_entity = &entities[entity->grabbed_entity.index];
				{
					DOUBLE_HANDLE* handle; 
					PUSH_BACK(update_grabbing_list, memory->temp_arena, handle);
					*handle = {{i,generations[i]}, entity->grabbed_entity};
				}
				V3 owner_target_vector = sticked_entity->target_pos - sticked_entity->pos;
				f32 offset_angle = v2_angle({owner_target_vector.x, owner_target_vector.z});


				f32 relative_distance;
				if(entity->flags & P_SHIELD)
				{
					f32 owner_target_distance = v3_magnitude(owner_target_vector);
					relative_distance = CLAMP(0.1f, owner_target_distance/2, entity->scale.x + sticked_entity->scale.x);
				}else{
					relative_distance = 0.5f + entity->scale.x + sticked_entity->scale.x;
				}
				
				V3 pos_difference = entity->pos - sticked_entity->pos;

				f32 current_angle = v2_angle({pos_difference.x, pos_difference.z});
				f32 angle_difference = entity->relative_angle + offset_angle - current_angle;

				if(angle_difference < (-PI32)){
					angle_difference += TAU32;
				}else if(PI32 < angle_difference){
					angle_difference -= TAU32;
				}

				V3 final_relative_pos = 
					v3_rotate_y(
						{relative_distance, 0, 0}, 
						-(current_angle + (10*world_delta_time*angle_difference))
					);
				// entity->pos = entity->pos + (15*world_delta_time * (sticked_entity->pos + final_relative_pos - entity->pos));
				entity->pos = sticked_entity->pos + final_relative_pos;
				
				//TODO: i probably need to change this if i want grabbed entities to autoaim
				if(!(entity->flags & E_AUTO_AIM_CLOSEST))
				{
					entity->target_pos = sticked_entity->target_pos;
					entity->looking_direction = entity->target_pos - entity->pos;
				}
			}else{
				entity->flags &= (~E_STICK_TO_ENTITY);
				if(entity->flags & P_SHIELD)
				{
					s32* index_to_kill; PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
					*index_to_kill = i;
				}
			}
		}
		else
		{ // DEFAULT CASE APPLY VELOCITY
			if(entity->flags & E_DEFEND_BOSS)
			{
				V3 entity_boss_pos;
				if(entity->team_uid == player_entity->team_uid)
				{
					entity_boss_pos = player_entity->pos;
				}else{
					entity_boss_pos = boss_entity->pos;
				}
				V3 boss_to_target_vector = entity->target_pos - entity_boss_pos;
				f32 boss_to_closest_distance = v3_magnitude(boss_to_target_vector);
				#define MAX_DEFEND_DISTANCE_FROM_BOSS 5.0f
				f32 final_distance_from_boss = CLAMP(0, boss_to_closest_distance/2 ,MAX_DEFEND_DISTANCE_FROM_BOSS);
				V3 target_pos = entity_boss_pos + ((final_distance_from_boss/boss_to_closest_distance)*boss_to_target_vector);
				entity->velocity = entity->velocity + (target_pos - entity->pos)/2;
			}
			if(!(entity->flags & E_NOT_MOVE))
			{
				entity->pos = entity->pos + (entity_dt * entity->velocity);
			}
		}


		// CLAMPING ENTITY POSITION

		if(!(entity->flags & E_UNCLAMP_XZ))
		{
			f32 new_x = CLAMP(-27, entity->pos.x, 27);
			f32 new_z = CLAMP(-21, entity->pos.z, 21);
			if(entity->flags & E_STICK_TO_ENTITY)
			{
				Entity* owner_entity = entity_from_handle(entity->grabbed_entity, entities, generations);
				owner_entity->velocity.x += 10*(new_x-entity->pos.x);
				owner_entity->velocity.z += 10*(new_z-entity->pos.z);
			}
			entity->pos.x = new_x;
			entity->pos.z = new_z;
		}
		if(!(entity->flags & E_UNCLAMP_Y)){
			entity->pos.y = 0;
		}

		#define DEFAULT_AUTOAIM_RANGE 10.0f 
		// BOTH AUTOAIM FLAGS ARE INCOMPATIBLE WITH MANUAL AIMING
		if( entity->flags & E_AUTO_AIM_BOSS )
		{ 

			// if an entity is closer than the  detection range and the entity has the autoaimclosest flag
			b64 autoaims_closest = entity->flags & E_AUTO_AIM_CLOSEST;
			b32 is_healer = entity->element == EET_HEAL;

			if(autoaims_closest && 
				is_healer &&
				closest_ally_distance && closest_ally_distance < DEFAULT_AUTOAIM_RANGE
			){
				entity->target_pos = entities[closest_ally_uid].pos;
			}
			else if(
				autoaims_closest &&
				!is_healer && 
				closest_enemy_distance && closest_enemy_distance < DEFAULT_AUTOAIM_RANGE)
			{
				entity->target_pos = entities[closest_enemy_uid].pos;
			}
			else
			{
				// friendly
				if(entity->team_uid == player_entity->team_uid || 
					(entity->team_uid != player_entity->team_uid && 
					is_healer)){
					if(global_boss_handle.is_valid(generations)){
						entity->target_pos = entities[global_boss_handle.index].pos;
					}else{
						entity->target_pos = entity->pos;
					}
				}else{ // enemy
					if(global_player_handle.is_valid(generations)){
						entity->target_pos = entities[global_player_handle.index].pos;
					}else{
						entity->target_pos = entity->pos;
					}
				}
			}
		}else{
			if((entity->flags & E_AUTO_AIM_CLOSEST))
			{
				if((entity->element == EET_HEAL) && closest_ally_distance)
				{
					entity->target_pos = entities[closest_ally_uid].pos;
				}else if(closest_enemy_distance){
					entity->target_pos = entities[closest_enemy_uid].pos;
				}
			}
		}

		if(isnan(entity->pos.x) || isnan(entity->pos.y) || isnan(entity->pos.z))
		{
			ASSERT(false);
		}

		if(entity->flags & E_SMOKE_SCREEN){
			entity->color = {.1f,.3f,.5f, 1};

			if(rng->time_dice(60, world_delta_time))
			{
					
				Particle_emitter	particle_emitter;
				
				f32 entity_radius = entity->scale.x * entity->creation_size;
				V3 initial_pos_rng = {entity_radius, 0, entity_radius};

				particle_emitter.fill_data(
					PARTICLE_ACTIVE,
					1,
					0,
					0,
					{0},
					initial_pos_rng,
					TAU32,
					0,
					0,
					{0, 1.0f, 0},
					{0},
					{0.0f,0.2f,0.4f, 0.01f},
					1.0f,
					0.5f,
					0,0,0,0,0,
					.01f,
					0,
					2.0f,
					10.0f
				);

				particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
				entity->pos, {1.0f,1.0f,0}, {.1f, .4f, .8f, .7f}, rng);
			}
		}
		if(entity->freezing_time_left)
		{
			if(rng->time_dice(4, world_delta_time))
			{
				Color init_color;
				Color target_color;
				f32 color_delta;
				f32 init_scale;
				f32 target_scale;
				f32 scale_delta;
				if(rng->next(2) < 1)
				{
					init_color = {.5f, .6f, 1.0f, 0.0f};
					target_color = {.8f, .9f, 1.0f, 1.0f};
					color_delta = 3.0f;
					init_scale = 0.3f;
					target_scale = 0.0f;
					scale_delta = 0.8f;
				}else{
					init_color = {.8f, .9f, 1.0f, 1.0f};
					target_color = {.5f, .6f, 1.0f, 0.0f};
					color_delta = 0.2f;
					init_scale = 0.0f;
					target_scale = 0.3f;
					scale_delta = 5.0f;
				}

				Particle_emitter particle_emitter;
				particle_emitter.fill_data(
					PARTICLE_ACTIVE,
					1,
					0,
					memory->textures.ice_tex_uid,
					{0},
					{3.0f,3.0f,3.0f},
					TAU32,
					0,
					10.0f,
					{0},
					{0},
					target_color,
					color_delta,
					1.0f,
					0,0,0,0,0,
					init_scale,
					0,
					target_scale,
					scale_delta
				);
				Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
				particle_emitter.emit_particle(new_particle, entity->pos, {0}, init_color, rng);

				new_particle->target_entity_h = {i, generations[i]};
			}
		}
		if(entity->toxic_time_left)
		{
			if(rng->time_dice(25,world_delta_time))
			{
				Particle_emitter particle_emitter;
				particle_emitter.fill_data(
					PARTICLE_ACTIVE,
					1,
					0,
					0,
					{0},
					{DEFAULT_TOXIC_RADIUS, 0, DEFAULT_TOXIC_RADIUS},
					0,
					0,
					0,
					{0,2.0f,0},
					{0},
					{1,0,1,0},
					1.0f,
					1.0f,
					0,0,0,0,0,
					.0f,
					0,
					.5f,
					1.0f
				);
				particle_emitter.emit_particle(get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index),
					entity->pos, {0,10.0f, 0},{0.5f,0, 1,1}, rng);
			}
		}
		if(entity->gravity_field_time_left)
		{
			if(rng->time_dice(4, world_delta_time))
			{
				
				f32 pos_offset_angle = rng->next(TAU32);
				V3 pos_offset = v3_rotate_y({entity->gravity_field_radius,0,0}, pos_offset_angle);

				Particle_emitter particle_emitter;
				particle_emitter.fill_data(
					PARTICLE_ACTIVE|PARTICLE_ACCEL_TOWARDS_TARGET_ENTITY,
					1,
					0,
					0,
					pos_offset,
					{0},
					0,
					0,
					0,
					{0,0,0},
					{0},
					{0,0,0,1},
					1.0f,
					1.5f,
					0,0,0,0,0,
					0.4f,
					0,
					0.01f,
					1.0f
				);

				Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
				particle_emitter.emit_particle(new_particle,
					entity->pos, {0}, {.2f,.2f,.2f,1}, rng
				);

				new_particle->target_entity_h = {i, generations[i]};
			}
		}

		// EMITTING PARTICLES
		#if 0
		u16 current_element = entity->element ? entity->element : entity->element_effect;

		if(entity->flags & E_EMIT_PARTICLES)
		{
			if(rng->time_dice(60, world_delta_time))
			{
				Particle_emitter particle_emitter = {0};
				particle_emitter.particle_flags = PARTICLE_ACTIVE;
				particle_emitter.particles_count = 1;
				particle_emitter.emit_cooldown = memory->delta_time*2;

				particle_emitter.velocity_yrotation_rng = TAU32;
				particle_emitter.friction = 10.0f;
				
				particle_emitter.acceleration = {0,10.0f, 0};
				
				particle_emitter.color_rng = {0.2f, 0.2f, 0.2f};
				particle_emitter.target_color = {0.5f,0.5f,0.5f,1};
				particle_emitter.color_delta_multiplier = 0.5f;

				particle_emitter.particle_lifetime = 0.3f;

				particle_emitter.initial_angle_rng = PI32;
				particle_emitter.angle_initial_speed_rng = 20.0f;
				particle_emitter.angle_friction = 4.0f;
				
				particle_emitter.initial_scale = 0.2f;
				particle_emitter.target_scale = 0.0f;
				particle_emitter.scale_delta_multiplier = 1.0f;

				entity->particle_timer += world_delta_time;

				if(entity->particle_timer >= particle_emitter.emit_cooldown)
				{
					entity->particle_timer -= particle_emitter.emit_cooldown;
					f32 particle_initial_speed = 20.0f;
					if(entity->flags & E_DOES_DAMAGE)
					{
						particle_initial_speed = 10.0f;
					}

					Color particles_color = element_color(current_element);
					if(!current_element)
					{
						particle_initial_speed = 5.0f;
					}

					Particle* new_particle = get_new_particle(memory->particles, memory->particles_max, &memory->last_used_particle_index);
					particle_emitter.emit_particle(new_particle, entity->pos, {particle_initial_speed,0,0}, particles_color, &memory->rng);
				}
			}
		}

		#endif

		//
		// END OF UPDATE LOOP
		//
	
	}

	// KILLING ENTITIES

	// TODO: maybe use entity handles for this instead of u32 ???
	FOREACH(u32, e_index, set_jump_change_direction_list)
	{
		entities[*e_index].jump_change_direction = true;
	}

	FOREACH(s32, e_index, entities_to_kill){
		Entity* kill_entity = &entities[*e_index];
		if(kill_entity->flags & P_PROJECTILE_EXPLODE)
		{
			Entity* explosion_hitbox; PUSH_BACK(entities_to_create, memory->temp_arena, explosion_hitbox);

			explosion_hitbox->flags = E_VISIBLE|E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_EXPLOSION;

			explosion_hitbox->color = {1.0f, 0, 0, 0.3f};
			explosion_hitbox->scale = {5,5,5};
			explosion_hitbox->creation_size = 1.0f;

			explosion_hitbox->lifetime = world_delta_time;

			// explosion_hitbox->parent_handle.index = i;
			// explosion_hitbox->parent_handle.generation = generations[i];
			// explosion_hitbox->team_uid = entity->team_uid;
			explosion_hitbox->total_power = calculate_power(kill_entity);

			explosion_hitbox->pos = kill_entity->pos;

			// explosion_hitbox->mesh_uid = memory->meshes.centered_plane_mesh_uid;
			explosion_hitbox->mesh_uid = memory->meshes.icosphere_mesh_uid;
			explosion_hitbox->texinfo_uid = memory->textures.white_tex_uid;
			
		}
		if(kill_entity->flags & E_GIVE_LOOT)
		{
			memory->teams_resources[kill_entity->team_uid] += 5 + kill_entity->total_upgrades_cost_value/2;
		}
		if(kill_entity->grabbed_entity.is_valid(generations))
		{
			entities[kill_entity->grabbed_entity.index].is_grabbing = false;
		}
		*kill_entity = {0};
		generations[*e_index]++;
	}
	// selected entity might be invalid at this point
	FOREACH(DOUBLE_HANDLE, e_handle, update_grabbing_list)
	{
		if(e_handle->from.is_valid(generations) && e_handle->to.is_valid(generations))
		{
			entities[e_handle->to.index].is_grabbing = true;
		}
	}

	// CREATING ENTITIES

	FOREACH(Entity, entity_properties, entities_to_create){
		Entity* created_entity = get_new_entity(entities, &memory->last_used_entity_index);
		*created_entity = *entity_properties;
	}


	// HANDLING INPUT
	if(hot_entity_uid >= 0)
	{
		entities[hot_entity_uid].color = {1,1,1,1};	
	}

	// DECIDE WHOS THE SELECTED ENTITY 

	if(!is_cursor_in_ui && !input->R){
		if(input->cursor_primary == 1){
			memory->clicked_entity_h = {(u32)hot_entity_uid, generations[hot_entity_uid]};
			memory->selected_entity_h = {0};
		}else if( input->cursor_primary == -1 ){
			if(memory->clicked_entity_h.is_valid(generations)){
				if((u32)hot_entity_uid == memory->clicked_entity_h.index){
					memory->selected_entity_h = memory->clicked_entity_h;
					
					Audio_playback* new_playback = find_next_available_playback(playback_list);
					new_playback->initial_sample_t = sample_t;
					new_playback->sound_uid = memory->sounds.pa_uid;
				}else{
					memory->selected_entity_h = {0};
				}
				memory->clicked_entity_h = {0};
			}
		}
	}
	
	exists_selected_entity = memory->selected_entity_h.is_valid(generations);
	selected_entity = &entities[memory->selected_entity_h.index];

	
	if(exists_selected_entity)
	{

		// GRAB CLOSEST ENTITY BY THE SELECTED

		//TODO: highlight the closest entity
		if(input->F == -1 && memory->closest_entity_to_grab_h.is_valid(generations))
		{
			Entity* entity_to_grab = &entities[memory->closest_entity_to_grab_h.index];
			if(
				memory->is_valid_grab &&
				memory->closest_entity_to_grab_h != global_player_handle &&
				entity_to_grab->team_uid == player_entity->team_uid
			){
				memory->closest_entity_to_grab_h = {0};
				// selected_entity->entity_to_grab = selected_entity->closest_entity_handle;
				entity_to_grab->flags |= E_STICK_TO_ENTITY;
				entity_to_grab->grabbed_entity = memory->selected_entity_h;
				
				selected_entity->is_grabbing = true;
				f32 current_relative_angle = v2_angle({
						entity_to_grab->pos.x - selected_entity->pos.x,
						entity_to_grab->pos.z - selected_entity->pos.z
					});
				f32 angle_offset = v2_angle(
					selected_entity->target_pos.x - selected_entity->pos.x, 
					selected_entity->target_pos.z - selected_entity->pos.z);
				entity_to_grab->relative_angle = current_relative_angle - angle_offset;
			}
		}
		
		// COLOR AND LOOKING DIRECTION OF SELECTED ENTITY
		selected_entity->color = selected_entity->color + (world_delta_time*(color_difference({1,1,1,1}, selected_entity->color)));

		if( input->cursor_secondary > 0)
		{
			#define E_CANNOT_MANUALLY_AIM (E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST|E_CANNOT_AIM)
			if(!(selected_entity->flags & E_CANNOT_MANUALLY_AIM))
			{
				selected_entity->target_pos = {cursor_y0_pos.x, 0, cursor_y0_pos.z};
			}
		}
	}
	else
	{
		if(input->cursor_secondary == 1)
		{
			memory->creating_entity = 1;
		}

		if(memory->creating_entity)
		{
			if( input->cursor_secondary > 0 )
			{
				//TODO: preview new entity to spawn
				V3 relative_cursor_pos = cursor_y0_pos - player_entity->pos;
				f32 relative_cursor_pos_magnitude = v3_magnitude(relative_cursor_pos);
				if(relative_cursor_pos_magnitude > player_entity->action_range){
					f32 temp_multiplier = player_entity->action_range / relative_cursor_pos_magnitude;
					memory->new_entity_pos = player_entity->pos + (temp_multiplier * relative_cursor_pos);
				}else{
					memory->new_entity_pos = cursor_y0_pos;
				}
			}
			else if( input->cursor_secondary == -1)
			{
				memory->creating_entity = 0;
				if(memory->team_spawn_charges[0])
				{
					memory->team_spawn_charges[0]--;
					
					Entity* new_entity = get_new_entity(entities, &memory->last_used_entity_index);
					memory->selected_entity_h = {memory->last_used_entity_index, generations[memory->last_used_entity_index]};

					u64 new_entity_flags = E_CAN_MANUALLY_MOVE|E_LOOK_IN_THE_MOVING_DIRECTION
						|E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|E_GIVE_LOOT
						|E_EMIT_PARTICLES
						;
					new_entity->fill_entity(
						new_entity_flags,
						EET_NULL,
						memory->new_entity_pos,
						boss_entity->pos,
						40,
						40,
						10,
						0.3f,
						global_player_handle,
						player_entity->team_uid,
						memory->meshes.blank_entity_mesh_uid,
						memory->textures.test_tex_uid
					);
				}

			}	
		}
	}
	
}


void render(App_memory* memory, LIST(Renderer_request,render_list), Int2 screen_size)
{
	Renderer_request* request = 0;
	PUSH_BACK(render_list, memory->temp_arena,request);
	request->type_flags = REQUEST_FLAG_SET_PS|REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_DEPTH_WRITING|
		REQUEST_FLAG_SET_BLEND_STATE|REQUEST_FLAG_SET_DEPTH_STENCIL|REQUEST_FLAG_SET_TIME;
	request->vshader_uid = memory->vshaders.default_vshader_uid;
	request->pshader_uid = memory->pshaders.default_pshader_uid;
	request->blend_state_uid = memory->blend_states.default_blend_state_uid;
	request->depth_stencil_uid = memory->depth_stencils.default_depth_stencil_uid;
	request->render_target_view_uid = memory->render_target_views.post_processing_rtv;
	request->depth_writing = 1.0f;
	request->new_time = memory->time_s;

	LIST(Renderer_request, delayed_render_list) = {0};
	LIST(Renderer_request, delayed_render_list2) = {0};

	UNTIL(i, MAX_ENTITIES)
	{
		if(!(memory->entities[i].flags & E_VISIBLE)) continue;
		
		if(memory->entities[i].flags & E_SMOKE_SCREEN)
		{
			PUSH_BACK(delayed_render_list2, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->object3d = memory->entities[i].object3d;
			request->mesh_uid = memory->meshes.centered_plane_mesh_uid;
			request->rotation = {PI32/2,0,0};


			f32 scale_multiplier = MAX(memory->delta_time, memory->entities[i].creation_size);
			request->object3d.scale = scale_multiplier * request->object3d.scale;
		}
		else
		{
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->object3d = memory->entities[i].object3d;

			f32 scale_multiplier = MAX(memory->delta_time, memory->entities[i].creation_size);

			request->object3d.scale = scale_multiplier * request->object3d.scale;
			#if 0
			if(memory->entities[i].reaction_cooldown)
			{
				PUSH_BACK(delayed_render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d = memory->entities[i].object3d;

				request->object3d.scale = 1.0f*scale_multiplier*request->object3d.scale;
				request->mesh_uid = memory->meshes.icosphere_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
			#endif
			if(memory->entities[i].flags & E_TOXIC_EMITTER || memory->entities[i].toxic_time_left)
			{
				PUSH_BACK(delayed_render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				// V3 yoffset = {0,-1,0};
				request->object3d.pos = memory->entities[i].pos;// + yoffset;
				f32 toxic_radius;
				if(memory->entities[i].flags & E_TOXIC_EMITTER){ 
					toxic_radius = memory->entities[i].aura_radius;
				}else{
					toxic_radius = DEFAULT_TOXIC_RADIUS;
				}
				request->object3d.scale = {toxic_radius,toxic_radius,toxic_radius};
				request->object3d.color = {0.2f, 0.0f , 0.4f, 0.3f};
				request->object3d.rotation = {PI32/2,0,0};
				
				request->object3d.mesh_uid = memory->meshes.centered_plane_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;

			}
			if(memory->entities[i].element == EET_HEAL)
			{
				PUSH_BACK(delayed_render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				// V3 yoffset = {0, -1, 0};
				request->object3d.pos = memory->entities[i].pos;// + yoffset;
				request->object3d.scale = {memory->entities[i].aura_radius,memory->entities[i].aura_radius,1};
				request->object3d.color = {0.0f, 0.4f, 0.1f, 0.3f};
				request->object3d.rotation = {PI32/2, 0, 0};

				// request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
				request->mesh_uid = memory->meshes.centered_plane_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
			if(memory->entities[i].freezing_time_left)
			{				
				PUSH_BACK(delayed_render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.color = {0.6f, 0.7f, 1.0f, 1.0f};
				request->object3d.pos = memory->entities[i].pos;
				request->object3d.scale = {1.5f,1.5f,1.5f};
				request->object3d.rotation = {PI32/4, PI32/4, PI32/4};

				request->object3d.mesh_uid = memory->meshes.centered_cube_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
			if(memory->entities[i].gravity_field_time_left)
			{
				PUSH_BACK(delayed_render_list2,  memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_SET_TIME;
				request->new_time = -1.5f*memory->time_s;

				PUSH_BACK(delayed_render_list2, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.color = {0.1f, 0.1f, 0.1f, 0.3f};
				request->object3d.pos = memory->entities[i].pos;
				f32 gf_radius = memory->entities[i].gravity_field_radius;
				request->object3d.scale = {gf_radius,gf_radius,gf_radius};
				request->object3d.rotation = {PI32/2, 0, 0};

				request->object3d.mesh_uid = memory->meshes.centered_plane_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
				
				PUSH_BACK(delayed_render_list2,  memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_SET_TIME;
				request->new_time = memory->time_s;
			}

			// THIS IS JUST FOR DEBUG
			if(memory->entities[i].ignore_sphere_radius){
				PUSH_BACK(delayed_render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.color = {0.0f, 0.5f, 0.1f, 0.3f};
				V3 yoffset = {0, -1, 0};
				request->object3d.pos = memory->entities[i].ignore_sphere_pos;
				request->object3d.scale = v3_multiply(memory->entities[i].ignore_sphere_radius, {1,1,1});

				request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
		}
	}

	// SHOW FUTURE POSITION OF THE ENTITY THAT IS BEING GRABBED 
	b32 exists_selected_entity = memory->selected_entity_h.is_valid(memory->entity_generations);
	Entity* selected_entity =  &memory->entities[memory->selected_entity_h.index];

	if(exists_selected_entity && memory->input->F > 0 && memory->closest_entity_to_grab_h.is_valid(memory->entity_generations))
	{

		Entity* possible_e_to_grab = &memory->entities[memory->closest_entity_to_grab_h.index];
		V3 distance_vector = possible_e_to_grab->pos - selected_entity->pos;
		V3 final_relative_pos = (0.5f + possible_e_to_grab->scale.x + selected_entity->scale.x) * v3_normalize(distance_vector);

		PUSH_BACK(render_list, memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
		//TODO: make this color be green or blue depending if it can grab or not the entity
		if(memory->is_valid_grab){
			request->object3d.color = {0,1,0,0.3f};
		}else{
			request->object3d.color = {1,0,0,0.3f};
		}
		
		request->object3d.scale = {1,1,1};
		request->object3d.pos = selected_entity->pos + final_relative_pos;

		request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
		request->object3d.texinfo_uid = memory->textures.white_tex_uid;		
	}

	if(exists_selected_entity)
	{
		
		PUSH_BACK(delayed_render_list,  memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_SET_TIME;
		request->new_time = 0.24f + SINF(4*memory->time_s)/4;

		PUSH_BACK(delayed_render_list, memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
		request->object3d = selected_entity->object3d;
		request->mesh_uid = memory->meshes.centered_plane_mesh_uid;
		request->texinfo_uid = memory->textures.white_tex_uid;
		request->scale = {1.0f,1.0f,1.0f};
		request->pos.y += 1.0f;
		request->rotation = {PI32/2, 0,0};
		request->color = {1,1,1,1};

		// SHOW SELECTED ENTITY WITH A ICON OVER THE ENTITY
		// PUSH_BACK(render_list, memory->temp_arena, request);
		// request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
		// request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
		// request->object3d.texinfo_uid = memory->textures.white_tex_uid;
		// request->object3d.scale = {0.5f,0.5f,0.5f};
		// request->object3d.color = {1,1,1,0.5f};
		// request->object3d.pos = selected_entity->pos;
		// request->object3d.pos.y += selected_entity->scale.y;
		
		{
			// SHOW TARGET POSITION
			PUSH_BACK(delayed_render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->scale = {1.5f, 1.5f, 1.5f};
			request->rotation = {PI32/2, 0,0};
			request->pos = selected_entity->target_pos;
			request->mesh_uid = memory->meshes.centered_plane_mesh_uid;
			request->texinfo_uid = memory->textures.white_tex_uid;
			request->color = {1,0,0,0.15f};
		}
		
		PUSH_BACK(delayed_render_list,  memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_SET_TIME;
		request->new_time = memory->time_s;
	}

	// SHOW ENTITY THAT IS ABOUT TO BE CREATED

	if(memory->creating_entity)
	{
		PUSH_BACK(render_list, memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
		request->scale = {1,1,1};
		if(memory->team_spawn_charges[0]){
			request->color = {0,1,0,0.5f};
		}else{
			request->color = {1,0,0,0.5f};
		}
		request->pos = memory->new_entity_pos;
		request->mesh_uid = memory->meshes.blank_entity_mesh_uid;
		request->texinfo_uid = memory->textures.white_tex_uid;
		request->rotation.y = -PI32/2;
	}

	// UPDATING AND RENDERING PARTICLES
	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_PS
		|REQUEST_FLAG_SET_DEPTH_WRITING
		;
	request->depth_writing = 0.0f;
	request->pshader_uid = memory->pshaders.circle_pshader_uid;
	request->depth_writing = 0.0f;

	UNTIL(i, memory->particles_max)
	{
		if(!memory->particles[i].flags) continue;
		Particle* particle = &memory->particles[i];

		//UPDATING
		if(!skip_updating(memory))
		{
			V3 accel = particle->acceleration;
			
			if(particle->flags & PARTICLE_ACCEL_TOWARDS_TARGET_ENTITY)
			{
				if(particle->target_entity_h.is_valid(memory->entity_generations))
				{
					accel = accel + (memory->entities[particle->target_entity_h.index].pos - particle->position);
				}
			}
			particle->velocity = particle->velocity + memory->delta_time * calculate_delta_velocity(particle->velocity, accel, particle->friction);
			particle->position = particle->position + (memory->delta_time*particle->velocity);

			particle->scale = particle->scale + (particle->scale_delta_multiplier * memory->delta_time/particle->lifetime)*(particle->target_scale - particle->scale);
			
			particle->angle_speed = particle->angle_speed + memory->delta_time*((particle->angle_accel) - (particle->angle_friction*particle->angle_speed));
			particle->angle = particle->angle + particle->angle_speed;

			particle->color = color_addition(particle->color, (particle->color_delta_multiplier*memory->delta_time/particle->lifetime)*(color_difference(particle->target_color, particle->color)));
			particle->color.a = MAX(0.01f, particle->color.a);


			particle->lifetime -= memory->delta_time;
		}
		if(particle->lifetime <= 0 || particle->scale <= 0 )
		{
			*particle = {0};
		}
		else
		{

			// RENDERING 

			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_PARTICLES;
			request->mesh_uid = memory->meshes.centered_plane_mesh_uid;
			request->texinfo_uid = particle->tex_uid;
			
			request->scale = {particle->scale, particle->scale, particle->scale};
			request->pos = particle->position;
			request->rotation = {0, particle->angle, 0};
			request->color = particle->color;
			
		}
	}
	// RENDER AURAS
	
	// PUSH_BACK(render_list, memory->temp_arena, request);
	// request->type_flags = REQUEST_FLAG_SET_PS|REQUEST_FLAG_SET_VS
	// 	|REQUEST_FLAG_SET_DEPTH_WRITING
	// 	;
	// request->pshader_uid = memory->pshaders.default_pshader_uid;
	// request->vshader_uid = memory->vshaders.default_vshader_uid;
	// request->depth_writing = 0.0f;

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_PS
		;
	request->pshader_uid = memory->pshaders.circle_wave_pshader_uid;
	// request->depth_writing = 0.0f;
	
	
	// DELAYED RENDER LISTS

	FOREACH(Renderer_request, current_render_request, delayed_render_list){
		//TODO: not pushback but use the already saved request in the delay list (???)
		PUSH_BACK(render_list, memory->temp_arena, request);
		*request = *current_render_request;
	}
	FOREACH(Renderer_request, current_render_request, delayed_render_list2){
		//TODO: not pushback but use the already saved request in the delay list (???)
		PUSH_BACK(render_list, memory->temp_arena, request);
		*request = *current_render_request;
	}

	// POST PROCESSING
	
	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_POSTPROCESSING;
	

	// RENDER USER INTERFACES

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_DEPTH_STENCIL|REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_PS;
	request->render_target_view_uid = memory->render_target_views.default_rtv;
	request->depth_stencil_uid = memory->depth_stencils.ui_depth_stencil_uid;
	request->vshader_uid = memory->vshaders.ui_vshader_uid;
	request->pshader_uid = memory->pshaders.ui_pshader_uid;
	

	UNTIL(i, MAX_ENTITIES)
	{
		if((memory->entities[i].flags & E_RECEIVES_DAMAGE)
		){
			f32 health_offset = memory->entities[i].health - (s32)(memory->entities[i].health);
			String health_string = number_to_string((s32)memory->entities[i].health+(!!health_offset), memory->temp_arena);
			printo_world(memory, screen_size, render_list,
				health_string, 
				memory->entities[i].pos,
				{0,1,0,1}
			);
		}
	}


	printo_screen(memory, screen_size, render_list,
		string("here i will show the fps (probably): 666 fps"), {-1,1}, {1,1,0,1});

	String entities_count = number_to_string(memory->debug_active_entities_count, memory->temp_arena);
	printo_screen(memory, screen_size, render_list,
		concat_strings(string("active entities: "), entities_count, memory->temp_arena), {-1, .95f}, {1,1,0,1});

	String resources_string = number_to_string((s32)memory->teams_resources[memory->entities[memory->player_uid].team_uid], memory->temp_arena);
	printo_screen(memory, screen_size, render_list,
		concat_strings(string("resources: "), resources_string, memory->temp_arena), {-1,.9f}, {1,1,0,1}
	);

	String spawn_charges_string = number_to_string(memory->team_spawn_charges[memory->entities[memory->player_uid].team_uid], 
		memory->temp_arena);
	printo_screen(memory, screen_size, render_list,
		concat_strings(string("spawn_charges: "), spawn_charges_string, memory->temp_arena), {-1, .85f}, {1,1,0,1}
	);

	String current_level_string = number_to_string(memory->current_level, memory->temp_arena);
	printo_screen(memory, screen_size, render_list,
		concat_strings(string("current_level: "), current_level_string, memory->temp_arena),
		{-1, .8f}, {1,1,0,1}
	);

	if(exists_selected_entity)
	{
		String current_entity_value = number_to_string(selected_entity->total_upgrades_cost_value, memory->temp_arena);
		printo_screen(memory, screen_size, render_list,
			concat_strings(string("selected_entity_value: "), current_entity_value, memory->temp_arena), {-1, .75f}, {1,1,0,1}
		);
	}


	// RENDERING UI ELEMENTS


	// FIRST PASS TO DRAW RECTANGLES

	UNTIL(i, MAX_UI)
	{
		if(!memory->ui_elements[i].flags) continue;
		{
			Ui_element* current = &memory->ui_elements[i];

			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;

			request->scale = {2.0f*current->size.x/screen_size.x, 2.0f*current->size.y/screen_size.y, 1};
			request->pos = {2.0f*current->pos.x/screen_size.x -1, -(2.0f*current->pos.y/screen_size.y -1)};
			request->rotation.z = current->rotation;

			request->color = 0.8f*current->color;
			request->texinfo_uid = memory->textures.gradient_tex_uid;
			request->mesh_uid = memory->meshes.plane_mesh_uid;

			s32 cost_value = memory->ui_costs[i];
			if(cost_value)
			{
				printo_screen(memory, screen_size, render_list,
				number_to_string(cost_value, memory->temp_arena), request->pos.v2, {1,1,0,1});
			}
		}
	}

	// LINE DRAWING

	User_input* input = memory->input;
	if(
		(input->R) && (
			input->cursor_pixels_pos.x != memory->radial_menu_pos.x ||
			input->cursor_pixels_pos.y != memory->radial_menu_pos.y
		)
	){
		PUSH_BACK(render_list, memory->temp_arena, request);
		request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;

		V2 screen_radial_menu_pos = {
			2.0f*memory->radial_menu_pos.x/screen_size.x -1, 
			-2.0f*memory->radial_menu_pos.y/screen_size.y +1
		};

		V2 m_distance = input->cursor_pos - screen_radial_menu_pos;

		f32 m_distance_magnitude = v2_magnitude(m_distance);

		request->rotation.z = v2_angle({(f32)m_distance.x, (f32)m_distance.y});

		V2 pos_offset = {0};


		f32 line_width = 2;
		request->scale = {m_distance_magnitude, line_width/screen_size.y, 1};
		request->pos = {
			screen_radial_menu_pos.x, 
			screen_radial_menu_pos.y
		};

		request->color = {1,1,1,0.5f};
		request->texinfo_uid = memory->textures.white_tex_uid;
		request->mesh_uid = memory->meshes.plane_mesh_uid;
	}


	// SECOND PASS TO DRAW TEXT

	UNTIL(i, MAX_UI)
	{
		if(!memory->ui_elements[i].flags) continue;
		Ui_element* current = &memory->ui_elements[i];

		if(!current->text.length){
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;

			request->scale = {2.0f*current->size.x/screen_size.x, 2.0f*current->size.y/screen_size.y, 1};
			request->pos = {2.0f*current->pos.x/screen_size.x -1, -(2.0f*current->pos.y/screen_size.y -1)};

			request->color = current->color;
			request->texinfo_uid = memory->textures.gradient_tex_uid;
			request->mesh_uid = memory->meshes.plane_mesh_uid;
		}
		else
		{
			s32 xpixel_pos = current->pos.x+(current->size.x/2) - (current->text.length*8/2);
			s32 ypixel_pos = current->pos.y+(current->size.y-18)/2;
			printo_screen(memory, screen_size, render_list, current->text, 
				{2.0f*xpixel_pos/screen_size.x -1, 1- 2.0f*ypixel_pos/screen_size.y},
				// {2.0f*current->pos.x/screen_size.x -1, -2.0f*current->pos.y/screen_size.y -1}, 
				current->color);
		}
	}
}

#define PUSH_ASSET_REQUEST push_asset_request(memory, init_data, &request)
void init(App_memory* memory, Init_data* init_data){	
	memory->current_level = 1;
	memory->levels_count = 3;
	// TODO: when this happens, lift the assert and implement bigger bitwise flags
	ASSERT(E_LAST_FLAG < (0xfffffffffffffff)); // asserting there are not more than 60 flags
	// ASSERT(E_LAST_FLAG < 0Xffffffff); // asserting there are not more than 32 flags


	memory->entities = ARENA_PUSH_STRUCTS(memory->permanent_arena, Entity, MAX_ENTITIES);
	memory->entity_generations = ARENA_PUSH_STRUCTS(memory->permanent_arena, u32, MAX_ENTITIES);

	memory->ui_elements = ARENA_PUSH_STRUCTS(memory->permanent_arena, Ui_element, MAX_UI);

	Asset_request request = {0};
	{
		// TODO: THIS COULD BE AUTOMATED BY LOOKING INTO THE hlsl file
		u32 ie_count = 3;
		String* ie_names = ARENA_PUSH_STRUCTS(memory->temp_arena, String, ie_count);
		
		ie_names[0] = string("POSITION");
		ie_names[1] = string("TEXCOORD");
		ie_names[2] = string("NORMAL");

		u32* ie_sizes = ARENA_PUSH_STRUCTS(memory->temp_arena, u32, ie_count);
		ie_sizes[0] = sizeof(float)*3;
		ie_sizes[1] = sizeof(float)*2;
		ie_sizes[2] = sizeof(float)*3;

		request.type = VERTEX_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->vshaders.default_vshader_uid;
		request.filename = string("shaders/3d_vs.cso");
		request.ied = {ie_count, ie_names, ie_sizes};
		PUSH_ASSET_REQUEST;

		request.type = PIXEL_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->pshaders.default_pshader_uid; 
		request.filename = string("shaders/3d_ps.cso");
		PUSH_ASSET_REQUEST;

		request.type = PIXEL_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->pshaders.circle_pshader_uid;
		request.filename = string("shaders/circle_ps.cso");
		PUSH_ASSET_REQUEST;

		request.type = PIXEL_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->pshaders.circle_wave_pshader_uid;
		request.filename = string("shaders/circle_wave_ps.cso");
		PUSH_ASSET_REQUEST;
		
		request.type = VERTEX_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->vshaders.postprocessing_vshader_uid;
		request.filename = string("shaders/pp_vs.cso");
		request.ied = {ie_count, ie_names, ie_sizes};
		PUSH_ASSET_REQUEST;
		
		request.type = PIXEL_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->pshaders.postprocessing_pshader_uid;
		request.filename = string("shaders/pp_ps.cso");
		PUSH_ASSET_REQUEST;
		

		request.type = CREATE_BLEND_STATE_REQUEST;
		request.p_uid = &memory->blend_states.default_blend_state_uid;
		request.enable_alpha_blending = true;
		PUSH_ASSET_REQUEST;

		request.type = CREATE_DEPTH_STENCIL_REQUEST;
		request.p_uid = &memory->depth_stencils.default_depth_stencil_uid;
		request.enable_depth = true;
		PUSH_ASSET_REQUEST;
	}

	{
		request.type = VERTEX_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->vshaders.ui_vshader_uid;
		request.filename = string("shaders/ui_vs.cso");

		u32 ie_count = 2;

		String* ie_names = ARENA_PUSH_STRUCTS(memory->temp_arena, String, ie_count);
		ie_names[0] = string("POSITION");
		ie_names[1] = string("TEXCOORD");

		u32* ie_sizes = ARENA_PUSH_STRUCTS(memory->temp_arena, u32, ie_count);
		ie_sizes[0] = sizeof(float)*3;
		ie_sizes[1] = sizeof(float)*2;

		request.ied = {ie_count, ie_names, ie_sizes};
		
		PUSH_ASSET_REQUEST;

		request.type = PIXEL_SHADER_FROM_FILE_REQUEST;
		request.p_uid = &memory->pshaders.ui_pshader_uid;
		request.filename = string("shaders/ui_ps.cso");
		PUSH_ASSET_REQUEST;

		request.type = CREATE_DEPTH_STENCIL_REQUEST;
		request.p_uid = &memory->depth_stencils.ui_depth_stencil_uid;
		request.enable_depth = false;
		PUSH_ASSET_REQUEST;

		memory->render_target_views.default_rtv = 2;
		memory->render_target_views.post_processing_rtv = 0;
		memory->render_target_views.depth_rtv = 1;
		
	}

	memory->debug_active_entities_count = 0;
	memory->particles_max = 1000;
	memory->particles = ARENA_PUSH_STRUCTS(memory->permanent_arena, Particle, memory->particles_max);


	Boss_action Actions_pool [] =
	{
		boss_action_move({0,0,0}),
		boss_action_wait(0.5f)
	};

/*
	// CREATING CONSTANT_BUFFER
	D3D_constant_buffer object_buffer = {0};
	dx11_create_and_bind_constant_buffer(
		dx, &object_buffer, sizeof(XMMATRIX), OBJECT_BUFFER_REGISTER_INDEX, 0
	);
	// WORLD_VIEW_BUFFER_REGISTER_INDEX
	XMMATRIX IDENTITY_MATRIX = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	D3D_constant_buffer view_buffer = {0};
	XMMATRIX view_matrix; // this will change in main loop
	dx11_create_and_bind_constant_buffer(
		dx, &view_buffer, sizeof(XMMATRIX), WORLD_VIEW_BUFFER_REGISTER_INDEX, 0
	);

	// WORLD_PROJECTION_BUFFER_REGISTER_INDEX
	D3D_constant_buffer projection_buffer = {0};
	XMMATRIX projection_matrix;
	dx11_create_and_bind_constant_buffer(
		dx, &projection_buffer, sizeof(XMMATRIX), WORLD_PROJECTION_BUFFER_REGISTER_INDEX, 0
	);

*/
	//TODO: PLEASE AUTOMATE THIS IN THE FUTURE
	//TODO: add another level of indirection using the keys
	// maybe union the memory->meshes with an array of indexes and automatically index them 
	{
		String_index_pair string_index_pairs [] = {
			{string(":default_mesh:"), &memory->meshes.default_mesh_uid},
			{string(":ball_mesh:"), &memory->meshes.ball_mesh_uid},
			{string(":centered_cube_mesh:"), &memory->meshes.centered_cube_mesh_uid},
			{string(":cube_mesh:"), &memory->meshes.cube_mesh_uid},
			{string(":plane_mesh:"), &memory->meshes.plane_mesh_uid},
			{string(":icosphere_mesh:"), &memory->meshes.icosphere_mesh_uid},
			{string(":player_mesh:"), &memory->meshes.player_mesh_uid},
			{string(":spawner_mesh:"), &memory->meshes.spawner_mesh_uid},
			{string(":boss_mesh:"), &memory->meshes.boss_mesh_uid},
			{string(":tank_mesh:"), &memory->meshes.tank_mesh_uid},
			{string(":shield_mesh:"), &memory->meshes.shield_mesh_uid},
			{string(":shooter_mesh:"), &memory->meshes.shooter_mesh_uid},
			{string(":melee_mesh:"), &memory->meshes.melee_mesh_uid},
			{string(":blank_entity:"), &memory->meshes.blank_entity_mesh_uid},
		};

		LIST(String, meshes_filenames) = {0};
		parse_assets_serialization_file(memory, init_data->meshes_serialization, 
			string_index_pairs, ARRAYCOUNT(string_index_pairs), meshes_filenames);

		FOREACH(String, current, meshes_filenames){
			request.type = MESH_FROM_FILE_REQUEST;
			request.filename = *current;
			PUSH_ASSET_REQUEST;
		}
	}
	{
		String_index_pair string_index_pairs [] = {
			{string(":default_tex:"), &memory->textures.default_tex_uid},
			{string(":white_tex:"), &memory->textures.white_tex_uid},
			{string(":gradient_tex:"), &memory->textures.gradient_tex_uid},
			{string(":test_texture:"), &memory->textures.test_tex_uid},
			{string(":ice_tex:"), &memory->textures.ice_tex_uid}
		};

		LIST(String, textures_filenames) = {0};
		parse_assets_serialization_file(memory, init_data->textures_serialization,
			string_index_pairs, ARRAYCOUNT(string_index_pairs), textures_filenames);

		FOREACH(String, current, textures_filenames){
			request.type = TEX_FROM_FILE_REQUEST;
			request.filename = *current;
			PUSH_ASSET_REQUEST;
		}
	}

	//TODO: PLEASE AUTOMATE THIS IN THE FUTURE
	// maybe union the memory->meshes with an array of indexes and automatically index them 
	{
		String_index_pair string_index_pairs [] = {
			{string(":wa:"), &memory->sounds.wa_uid},
			{string(":pe:"), &memory->sounds.pe_uid},
			{string(":pa:"), &memory->sounds.pa_uid},
			{string(":psss:"), &memory->sounds.psss_uid}
		};

		LIST(String, sound_filenames) = {0};
		parse_assets_serialization_file(memory, init_data->sounds_serialization, 
			string_index_pairs, ARRAYCOUNT(string_index_pairs), sound_filenames);

		FOREACH(String, current, sound_filenames){
			request.type = SOUND_FROM_FILE_REQUEST;
			request.filename = *current;
			PUSH_ASSET_REQUEST;
		}
	}

	//TODO: make it possible to load more than one font
	request.type = FONT_FROM_FILE_REQUEST;
	request.p_uid = &memory->textures.font_atlas_uid;
	request.filename = string("data/fonts/Inconsolata-Regular.ttf");
	PUSH_ASSET_REQUEST;
}