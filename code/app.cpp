#include "app.h"

#define MAX_SPAWN_DISTANCE 5.0f
#define BOSS_INDEX 1

global_variable Element_handle global_boss_handle = {0};
global_variable Element_handle global_player_handle = {0};
void update(App_memory* memory, Audio_playback* playback_list, u32 sample_t, Int2 client_size){
	if(!memory->is_initialized){
		memory->is_initialized = true;
		
		set_mem(memory->entities, MAX_ENTITIES*sizeof(Entity), 0);
		set_mem(memory->entity_generations, MAX_ENTITIES*sizeof(u32), 0);

		memory->camera_rotation.x = PI32/4;
		memory->camera_rotation.y = 0;
		memory->camera_rotation.z = 0;
		memory->camera_pos.y = 32.0f;

		memory->last_inactive_entity = 0;

		memory->entity_generations[0] = 1;
		global_player_handle.index = memory->player_uid;
		global_player_handle.generation = memory->entity_generations[memory->player_uid];
		Entity* player = &memory->entities[memory->player_uid];
		default_object3d(player);
		player->flags = E_VISIBLE|E_SELECTABLE|E_SPAWN_ENTITIES|E_AUTO_AIM_BOSS|
			E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE|E_NOT_MOVE;
		player->pos = {-25, 0, 0};
		player->max_health = 10;
		player->health = player->max_health;
		player->team_uid = 0;

		player->action_count = 1;
		player->action_angle = TAU32/4;
		player->action_cd_total_time = 3.0f;
		player->action_cd_time_passed = 2.0f;
		player->action_max_distance = 5.0f;

		player->aura_radius = 3.0f;

		memory->teams_resources[player->team_uid] = 50;
		player->creation_delay = 0.2f;

		player->team_uid = 0;
		player->speed = 5.0f;
		player->friction = 1.0f;
		player->type = ENTITY_UNIT;

		player->mesh_uid = memory->meshes.player_mesh_uid;
		player->texinfo_uid = memory->textures.white_tex_uid;
		
		global_boss_handle.index = BOSS_INDEX;
		global_boss_handle.generation = memory->entity_generations[BOSS_INDEX];

		Entity* boss = &memory->entities[BOSS_INDEX];
		default_object3d(boss);
		boss->flags = 
			E_VISIBLE|E_DETECT_COLLISIONS|E_HAS_COLLIDER|E_RECEIVES_DAMAGE|E_NOT_MOVE|
			E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST|E_LOOK_TARGET_WHILE_MOVING|E_SPAWN_ENTITIES;

		boss->speed = 60.0f;
		boss->friction = 10.0f;

		boss->max_health = 200;
		boss->health = boss->max_health;
		boss->pos = {25, 0, 0};
		boss->team_uid = 1;
		boss->type = ENTITY_BOSS;
		boss->creation_delay = 0.2f;

		boss->action_count = 1;
		boss->action_angle = TAU32/4;
		boss->action_cd_total_time = 2.0f;
		boss->action_max_distance = 5.0f;
		boss->aura_radius = 3.0f;

		boss->mesh_uid = memory->meshes.boss_mesh_uid;;
		boss->texinfo_uid = memory->textures.default_tex_uid;

		memory->unit_creation_costs[UNIT_SHOOTER] = 20; // 20
		memory->unit_creation_costs[UNIT_TANK] = 15;
		memory->unit_creation_costs[UNIT_SPAWNER] = 40;
		memory->unit_creation_costs[UNIT_MELEE] = 5;

		memory->selected_uid = -1;
		memory->ui_selected_uid = -1;
		memory->ui_clicked_uid = -1;
	}

	User_input* input = memory->input;
	User_input* holding_inputs = memory->holding_inputs;
	Entity* entities = memory->entities;
	u32* generations = memory->entity_generations;
	Ui_element* ui_elements = memory->ui_elements;
	f32 delta_time = memory->delta_time;

	//TODO: this
	if(input->reset == 1)
		memory->is_initialized = false;

	// f32 camera_speed = 1.0f;
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


	// UI

	// CALCULATE UI_ELEMENT RECTANGLES





	// FIND OUT WHOS THE ACTIVE ONE

	
	s32 hot_element_uid = -1;
	UNTIL(i, MAX_UI)
	{
		if(!ui_elements[i].flags) continue;
		
		if(ui_is_point_inside(&ui_elements[i], input->cursor_pixels_pos))
		{
			hot_element_uid = i;
		}
	}


	if(input->cursor_primary == 1)
	{
		memory->ui_selected_uid = hot_element_uid;
	}
	
	if(input->cursor_primary == -1)
	{
		if(memory->ui_selected_uid == hot_element_uid){
			memory->ui_clicked_uid = hot_element_uid;
		}else{
			memory->ui_clicked_uid = -1;
		}
		memory->ui_selected_uid = -1;
	}else{
		memory->ui_clicked_uid = -1;
	}
	b32 is_cursor_in_ui = hot_element_uid >= 0;


	// CLEANING LAST FRAME UI_ELEMENTS
	
	UNTIL(i, MAX_UI){
		ui_elements[i] = {0};
	}
	

	// CREATING RADIAL MENU

	#define MENU_RADIUS 200

	if(input->L == 1){
		// memory->radial_menu_pos = input->cursor_pixels_pos;
		memory->radial_menu_pos.x = CLAMP(MENU_RADIUS+50, input->cursor_pixels_pos.x, client_size.x -(50+MENU_RADIUS));
		memory->radial_menu_pos.y = CLAMP(MENU_RADIUS, input->cursor_pixels_pos.y, client_size.y -MENU_RADIUS);
	}

	if(input->L > 0){
		s32 ui_last = 0;

		u64 possible_flags_to_set[] = {
			E_SHOOT,
			E_MELEE_ATTACK,
			E_FOLLOW_TARGET|E_AUTO_AIM_BOSS|E_AUTO_AIM_CLOSEST,
			E_CAN_MANUALLY_MOVE,
			E_TOXIC|E_TOXIC_EMITTER|E_TOXIC_DAMAGE_INMUNE,
			E_HEALER,
			E_GENERATE_RESOURCE
		};

		char* button_text[] = {
			"shoot",
			"melee attack",
			"homing",
			"manually move",
			"toxic",
			"healer",
			"resource farm"
		};

		f32 angle_step = TAU32 / ARRAYCOUNT(button_text);
		V2 initial_position = {0,-MENU_RADIUS};
		UNTIL(i, ARRAYCOUNT(button_text)){
			if(ui_last == memory->ui_clicked_uid)
			{
				if(memory->selected_uid >= 0)
				{
					if(possible_flags_to_set[i] & E_TOXIC) // toxic emitter
					{
						if(entities[memory->selected_uid].flags & E_TOXIC_EMITTER){
							entities[memory->selected_uid].flags &= possible_flags_to_set[i] ^ 0xffffffffffffffff;
						}else{
							entities[memory->selected_uid].flags |= possible_flags_to_set[i];
						}
					}
					else if(possible_flags_to_set[i] & E_HEALER) // healer
					{
						entities[memory->selected_uid].flags ^= E_HEALER; 
						//TODO: this will be a bug if there is any other thing that turns action power 
						// into a negative number
						entities[memory->selected_uid].action_power *= -1;
					}
					else // default case
					{
						entities[memory->selected_uid].flags ^= possible_flags_to_set[i];
					}
				}
			}

			Ui_element* ui_element = &ui_elements[ui_last++];
			
			
			if(memory->selected_uid >= 0){
				if(!((entities[memory->selected_uid].flags & possible_flags_to_set[i]) ^ possible_flags_to_set[i])){
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
	
	
	
	// 
	if(hot_element_uid >= 0){
		ui_elements[hot_element_uid].color = colors_product(ui_elements[hot_element_uid].color, {0.7f,0.7f,1.0f,1.0f});
	}
	if(memory->ui_selected_uid >= 0){
		ui_elements[memory->ui_selected_uid].color = {1,1, 0.5f, 1};
	}
	



	//
	// END OF UI_CODE
	//


	// MOVE SELECTED ENTITY

	V2 input_vector = {(f32)(holding_inputs->d_right - holding_inputs->d_left),(f32)(holding_inputs->d_up - holding_inputs->d_down)};
	input_vector = v2_normalize(input_vector);
	if(memory->selected_uid >= 0){
		Entity* selected_entity = &entities[memory->selected_uid];
		if(selected_entity->flags & E_CAN_MANUALLY_MOVE)
		{
			selected_entity->normalized_accel = v3_normalize(input_vector.x, 0, input_vector.y);
		}
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

	LIST(u32, entities_to_kill) = {0};
	LIST(Entity, entities_to_create) = {0};

	
	{
		Entity* wall = 0;
		wall = &entities[5];
		default_wall(wall, memory);
		wall->scale = {1,1,45};
		wall->pos = {-29, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		wall->team_uid = 0xffff;
		
		wall = &entities[6];
		default_wall(wall, memory);
		wall->scale = {1,1,45};
		wall->pos = {28, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		wall->team_uid = 0xffff;
		
		wall = &entities[7];
		default_wall(wall, memory);
		wall->scale = {56,1,1};
		wall->pos = {-28, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		wall->team_uid = 0xffff;

		wall = &entities[8];
		default_wall(wall, memory);
		wall->scale = {56,1,1};
		wall->pos = {-28, 0, 21};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		wall->team_uid = 0xffff;
	}

// MAIN ENTITY UPDATE LOOP

	if(input->d_right == 1){
		ASSERT(true);
	}

	f32 closest_t = {0};
	b32 first_intersection = false;
	UNTIL(i, MAX_ENTITIES){
		
		if(!entities[i].flags) continue;


		// CREATION TIME
		// i don't like that this must be done even with entities that skip_updating but whatevs

		if(entities[i].creation_size < 1.0f)
		{
			if(entities[i].creation_delay <= 0){
				entities[i].creation_size = 1.0f;
			}else{
				entities[i].creation_size += (1-entities[i].creation_size) / (entities[i].creation_delay/delta_time);
				entities[i].creation_delay -= delta_time;
			}
		}


		if(entities[i].flags & E_SKIP_UPDATING) continue;
		Entity* entity = &entities[i];


		// ENEMY_COLOR

		if(entity->team_uid != entities[memory->player_uid].team_uid){
			entity->color = {0.7f, 0,0,1};
		}


		// LIFETIME

		if(entity->lifetime)
		{
			entity->lifetime -= delta_time;
			if(entity->lifetime <= 0 )
			{
				u32* entity_index; PUSH_BACK(entities_to_kill, memory->temp_arena, entity_index);
				*entity_index = i;
				entity->flags &= (0xffffffffffffffff ^ E_SELECTABLE);
			}
		}


		// HEALING COOLDOWN

		entity->healing_cd -= delta_time;
		if(entity->healing_cd <= 0){
			entity->healing_cd = 0;
		}
		


		// PROCESSING TOXIC ENTITIES

		// TODO: i think this should be done when assigning the flag
		// but my current ui system is kinda tedious
		if(entity->flags & E_TOXIC_EMITTER){
			entity->toxic_time_left = 0;
		}else if(entity->flags & E_TOXIC){
			if(!(entity->flags & E_TOXIC_DAMAGE_INMUNE)){
				entity->toxic_tick_damage_cd -= delta_time;
				if(entity->toxic_tick_damage_cd <= 0){
					entity->toxic_tick_damage_cd += 2.0f;

					entity->health -= 1;
					if(entity->health <= 0){
						u32* index_to_kill; PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
						*index_to_kill = i;
					}
				}
			}
		}

		// TOXIC LIFETIME

		if(entity->toxic_time_left)
		{

			entity->toxic_time_left -= delta_time;
			if(entity->toxic_time_left <= 0)
			{
				entity->toxic_time_left = 0;
				entity->flags ^= E_TOXIC;
			}
		}
		// TOXIC TICK DAMAGE


		

		// CURSOR RAYCASTING

		if(!is_cursor_in_ui){
			if((entity->flags & E_SELECTABLE) &&
				entity->team_uid == entities[memory->player_uid].team_uid 
			){	
				entity->color = {0.5f,0.5f,0.5f,1};

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

		if(entity->flags & E_FOLLOW_TARGET)
		{
			entity->normalized_accel = v3_normalize(entity->target_direction);
		}
		

		// MOVEMENT / DYNAMICS

		if(!(entity->flags & E_SKIP_DYNAMICS))
		{
			V3 acceleration = ((entity->speed*entity->normalized_accel)-(entity->friction*entity->velocity));
			entity->velocity = entity->velocity + (delta_time*acceleration);
			f32 min_threshold = 0.1f;
			if( // it is not moving
				(entity->normalized_accel.x < min_threshold && entity->normalized_accel.x > -min_threshold) &&
				(entity->normalized_accel.z < min_threshold && entity->normalized_accel.z > -min_threshold)
			){
				// look at the target
				//TODO: handle case when entity->target_direction == 0
				V3 delta_looking_direction = (10*delta_time*(entity->target_direction - entity->looking_direction));
				entity->looking_direction = entity->looking_direction + delta_looking_direction;
				//slowing_down
			}else{
				if(entity->flags & E_LOOK_TARGET_WHILE_MOVING)
				{
					// look at the target
					//TODO: handle case when entity->target_direction == 0
					V3 delta_looking_direction = (10*delta_time*(entity->target_direction - entity->looking_direction));
					entity->looking_direction = entity->looking_direction + delta_looking_direction;
				}else{
					// look in the moving direction
					//TODO: handle case when entity->velocity == 0
					V3 delta_looking_direction = (10*delta_time*(entity->velocity - entity->looking_direction));
					entity->looking_direction = entity->looking_direction + delta_looking_direction;
				}
			}
		}

		entity->normalized_accel = {0,0,0};
		
		
		// ROTATION / LOOKING DIRECTION
		if(!(entity->flags & E_SKIP_ROTATION)){
			// this is negative cuz +y is up while we are looking down
			entity->rotation.y = - (v2_angle({entity->looking_direction.x, entity->looking_direction.z}) + PI32/2); 
		}
		

		// SUB ITERATION

		s32 closest_entity_uid = -1;
		f32 closest_distance = 100000;
		
		// ALMOST ALL ENTITIES DO SOME OF THESE SO I DON'T KNOW HOW MUCH THIS OPTIMIZES ANYTHING
		if(entity->flags & (E_DOES_DAMAGE|E_AUTO_AIM_CLOSEST|E_DETECT_COLLISIONS))
		{
			UNTIL(j, MAX_ENTITIES)
			{
				if(!entities[j].flags)continue;
				if(j == i) continue;
				//TODO: walls/obstacles should not be skipping this 
				// cuz they need to be detected by the thing that collided with them
				if(entities[j].flags & E_SKIP_UPDATING) continue;
				//TODO:maybe not skip the whole iteration but just certain parts?
				// or then make specific flags to handle each case when necessary
				if(entity->flags & E_SKIP_PARENT_COLLISION)
				{
					if(entity->parent_handle.index == j &&
						entity->parent_handle.generation == generations[j])
					{
						continue;
					}
				}

				Entity* entity2 = &entities[j];


				//AUTOAIM 

				
				if(entity->team_uid != entity2->team_uid)
				{
					if(entity->flags & E_AUTO_AIM_CLOSEST && !(entity2->flags & E_NOT_TARGETABLE))
					{
						f32 distance = v3_magnitude(entity2->pos - entity->pos);
						if(closest_entity_uid < 0){
							closest_entity_uid = j;
							closest_distance = distance;
						}else if(distance < closest_distance){
							closest_entity_uid = j;
							closest_distance = distance;
						}
					}
				}


				// HITBOX / DAMAGE 


				if(
					entity->flags & E_DOES_DAMAGE &&
					entity2->flags & E_RECEIVES_DAMAGE &&
					entity->parent_handle != entity2->parent_handle
				){
					b32 they_collide = false;

					if(entity->collider_type == COLLIDER_TYPE_SPHERE){
						if(entity2->collider_type == COLLIDER_TYPE_SPHERE){ // BOTH SPHERES
							V3 centers_distance = entity2->pos - entity->pos;
							f32 centers_distance_magnitude = v3_magnitude(centers_distance);
							f32 r1 = entity->scale.x*entity->creation_size;
							f32 r2 = entity2->scale.x*entity2->creation_size;
							f32 overlapping =   (r1 + r2) - centers_distance_magnitude;

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
					
					if(they_collide)
					{
						entity2->health = CLAMP(0, entity2->health - entity->action_power, entity2->max_health);

						if(entity2->health <= 0)
						{
							u32* index_to_kill;
							PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
							*index_to_kill = j;
						}
						if(entity->flags & E_DIE_ON_COLLISION)
						{
							u32* index_to_kill;
							PUSH_BACK(entities_to_kill, memory->temp_arena, index_to_kill);
							*index_to_kill = i;
						}
					}
				}

				
				// TOXIC ENTITIES INFECTING


				if(entity->flags & E_TOXIC && 
					!(entity2->flags & E_TOXIC_EMITTER) &&
					(entity2->flags & E_RECEIVES_DAMAGE)
				){
					f32 distance = v3_magnitude(entity2->pos - entity->pos);
					if(distance < entity->aura_radius){
						entity2->toxic_time_left = 6.0f;
						entity2->flags |= E_TOXIC;
					}
				}


				// HEALING

				if(entity->flags & E_HEALER &&
					entity2->flags & E_RECEIVES_DAMAGE
				){
					f32 distance = v3_magnitude(entity2->pos - entity->pos);
					if(distance < entity->aura_radius){
						if(entity2->healing_cd <= 0){
							if(entity2->health < entity2->max_health){
								entity2->health = MIN(entity2->health+1, entity2->max_health);
								entity2->healing_cd += 2.0f;
							}
						}
					}
				}



				// COLLISIONS

				
				if(entity->flags & E_DETECT_COLLISIONS &&
					entity2->flags & E_HAS_COLLIDER
				){
					//TODO: collision code
					b32 they_collide = false;

					if(entity->collider_type == COLLIDER_TYPE_SPHERE)
					{
						if(entity2->collider_type == COLLIDER_TYPE_SPHERE) // BOTH SPHERES
						{ 
							V3 centers_distance = entity2->pos - entity->pos;
							f32 centers_distance_magnitude = v3_magnitude(centers_distance);
							f32 r1 = entity->scale.x*entity->creation_size;
							f32 r2 = entity2->scale.x*entity2->creation_size;
							f32 overlapping =   (r1 + r2) - centers_distance_magnitude;

							if(overlapping > 0){
								they_collide = true;

								//TODO: maybe do this if creating entities inside another is too savage
								// overlapping =  MIN(MIN(entity->current_scale, entity2->current_scale),overlapping);
								V3 collision_direction = {0,0,1.0f};
								if(centers_distance_magnitude){
									collision_direction = (overlapping/(1.0f*centers_distance_magnitude))*centers_distance;
									// collision_direction = centers_distance / centers_distance_magnitude;
								}

								
								//TODO: this will need a rework cuz it is not multithread friendly
								// i should not modify other entities while processing another one
								// f32 momentum_i = MAX(v3_magnitude(entity->velocity), delta_time);
								// f32 momentum_j = MAX(v3_magnitude(entity2->velocity), delta_time);
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
							*entity = {0};
							generations[i]++;
						}
					} 
				}


			}
		}

		#define DEFAULT_AUTOAIM_RANGE 10.0f 
		// BOTH AUTOAIM FLAGS ARE INCOMPATIBLE WITH MANUAL AIMING
		if(entity->flags & E_AUTO_AIM_BOSS){ 
			// if an entity is closer than the  detection range and the entity has the autoaimclosest flag
			if((entity->flags & E_AUTO_AIM_CLOSEST) && (closest_distance < DEFAULT_AUTOAIM_RANGE)){
				entity->target_direction = entities[closest_entity_uid].pos - entity->pos;
			}else{
				// friendly
				if(entity->team_uid == entities[memory->player_uid].team_uid){
					if(is_handle_valid(generations, global_boss_handle)){
						entity->target_direction = entities[global_boss_handle.index].pos - entity->pos;
					}else{
						entity->target_direction = {0,0,0};
					}
				}else{ // enemy
					if(is_handle_valid(generations, global_player_handle)){
						entity->target_direction = entities[global_player_handle.index].pos - entity->pos;
					}else{
						entity->target_direction = {0,0,0};
					}
				}
			}
		}else{
			if((entity->flags & E_AUTO_AIM_CLOSEST) && (closest_entity_uid >= 0) ){
				entity->target_direction = entities[closest_entity_uid].pos - entity->pos;
			}
		}


		// COOLDOWN ACTIONS 

		// if it is not 0
		if(entity->action_cd_total_time){
			entity->action_cd_time_passed += delta_time;
			if(entity->action_cd_time_passed >= entity->action_cd_total_time){
				entity->action_cd_time_passed -= entity->action_cd_total_time;

				ASSERT(entity->action_angle >= 0 && entity->action_angle <= TAU32);
				f32 angle_step;
				u32 repetitions = MAX(entity->action_count, 1);
				V3* target_directions = ARENA_PUSH_STRUCTS(memory->temp_arena, V3, repetitions);
				f32 looking_direction_length = v3_magnitude(entity->looking_direction);
				V3 normalized_looking_direction = looking_direction_length ? 
					entity->looking_direction / looking_direction_length :
					entity->looking_direction;

				if(repetitions > 1){ // if angle = 360 then two actions will happen in the same spot behind 
					V3 current_target_direction = v3_rotate_y(normalized_looking_direction,-entity->action_angle/2);
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
							E_VISIBLE|E_DETECT_COLLISIONS|E_DIE_ON_COLLISION|E_NOT_TARGETABLE|
							E_RECEIVES_DAMAGE|E_DOES_DAMAGE|E_UNCLAMP_XZ|E_SKIP_PARENT_COLLISION|
							E_TOXIC_DAMAGE_INMUNE|E_FOLLOW_TARGET
						;

						//speed could be by the one who shoots
						new_bullet->speed = 150;
						new_bullet->friction = 4.0f;
						new_bullet->lifetime = 5.0f;
						// this is obsolete but useful for debug
						new_bullet->type = ENTITY_PROJECTILE;
						new_bullet->mesh_uid = memory->meshes.ball_mesh_uid;
						new_bullet->texinfo_uid = memory->textures.white_tex_uid;
						new_bullet->color = {0.6f,0.6f,0.6f,1};
						new_bullet->scale = {0.4f,0.4f,0.4f};

						// health is how many hits can it get before dying
						new_bullet->max_health = 1;
						new_bullet->health = new_bullet->max_health;

						new_bullet->action_power = entity->action_power;

						new_bullet->aura_radius = 2.0f;

						Entity* parent = entity;
						new_bullet->parent_handle.index = i;
						new_bullet->parent_handle.generation = generations[i];
						new_bullet->team_uid = parent->team_uid;
						new_bullet->pos = parent->pos;
						// TODO: go in the direction that parent is looking (the parent's rotation);
						new_bullet->target_direction = parent->looking_direction;

						new_bullet->velocity =  (new_bullet->speed/2) * target_directions[current_action_i];
					}
				}
				if((entity->flags & E_MELEE_ATTACK))
				{
					UNTIL(current_action_i, repetitions)
					{
						Entity* hitbox; PUSH_BACK(entities_to_create, memory->temp_arena, hitbox);
						
						hitbox->flags = E_VISIBLE|E_DOES_DAMAGE|E_POS_IN_FRONT_OF_PARENT
							|E_SKIP_ROTATION|E_SKIP_DYNAMICS|E_SKIP_PARENT_COLLISION;

						hitbox->color = {0, 0, 0, 0.3f};
						hitbox->scale = {1,1,1};

						hitbox->lifetime = delta_time;

						hitbox->parent_handle.index = i;
						hitbox->parent_handle.generation = generations[i];
						hitbox->team_uid = entity->team_uid;
						hitbox->action_power = entity->action_power;
						hitbox->distance_from_parent = entity->creation_size*entity->scale.x;

						hitbox->pos = entity->pos + hitbox->distance_from_parent*target_directions[current_action_i];

						// hitbox->mesh_uid = memory->meshes.centered_plane_mesh_uid;
						hitbox->mesh_uid = memory->meshes.icosphere_mesh_uid;
						hitbox->texinfo_uid = memory->textures.white_tex_uid;
					}
				}
				if(entity->flags & E_GENERATE_RESOURCE){// RESOURCE GENERATOR
					memory->teams_resources[entity->team_uid]++;
				}
				if((entity->flags & E_SPAWN_ENTITIES))
				{
					UNTIL(current_action_i, repetitions)
					{
						Entity* new_entity; PUSH_BACK(entities_to_create, memory->temp_arena, new_entity);

						// default_melee(new_entity, memory);
						new_entity->flags = 
							E_VISIBLE|E_SELECTABLE|E_HAS_COLLIDER|E_DETECT_COLLISIONS|E_RECEIVES_DAMAGE;

						new_entity->color = {1,1,1,1};
						new_entity->scale = {1.0f,1.0f,1.0f};

						new_entity->speed = 40.0f;
						new_entity->friction = 5.0f;
						new_entity->max_health = 4;
						new_entity->health = new_entity->max_health;
						new_entity->action_cd_total_time = 1.0f;
						new_entity->aura_radius = 3.0f;

						new_entity->parent_handle.index = i;
						new_entity->parent_handle.generation = generations[i];
						new_entity->team_uid = entity->team_uid;
						new_entity->action_power = 1;

						V3 spawn_direction;
						if(looking_direction_length > entity->action_max_distance){
							f32 temp_multiplier = entity->action_max_distance / looking_direction_length;
							spawn_direction =  (temp_multiplier * entity->looking_direction);
						}else{
							spawn_direction = entity->looking_direction;
						}
						new_entity->pos = spawn_direction + entity->pos;
						
						if(entity->team_uid == entities[memory->player_uid].team_uid){//friendly
							V3 boss_pos = entity_from_handle(entities, generations, global_boss_handle)->pos;
							new_entity->target_direction = boss_pos - new_entity->pos;
						}else{//enemy
							V3 player_pos = entity_from_handle(entities, generations, global_player_handle)->pos;
							new_entity->target_direction = player_pos - new_entity->pos;
						}
						
						new_entity->mesh_uid = memory->meshes.icosphere_mesh_uid;
						new_entity->texinfo_uid = memory->textures.white_tex_uid;
					}
				}
			}
		}


		// UPDATE POSITION APPLYING VELOCITY

		if(entity->flags & E_POS_IN_FRONT_OF_PARENT){
			if(is_handle_valid(generations, entity->parent_handle)){
				Entity* parent = entity_from_handle(entities, generations, entity->parent_handle);
				entity->pos = parent->pos + entity->distance_from_parent*v3_normalize(parent->looking_direction);
			}
		}else{ // DEFAULT CASE
			if(!(entity->flags & E_NOT_MOVE)){
				entity->pos = entity->pos + (delta_time * entity->velocity);
			}
		}


		// CLAMPING ENTITY POSITION

		if(!(entity->flags & E_UNCLAMP_XZ)){
			entity->pos.x = CLAMP(-27, entity->pos.x, 27);
			entity->pos.z = CLAMP(-21, entity->pos.z, 21);
		}
		if(!(entity->flags & E_UNCLAMP_Y)){
			entity->pos.y = 0;
		}


	//
	// END OF UPDATE LOOP
	//
	
	}

	// KILLING ENTITIES

	FOREACH(u32, e_index, entities_to_kill){
		entities[*e_index] = {0};
		generations[*e_index]++;
	}


	// CREATING ENTITIES

	FOREACH(Entity, entity_properties, entities_to_create){
		u32 e_index = next_inactive_entity(entities, &memory->last_inactive_entity);
		entities[e_index] = *entity_properties;
	}


	// HANDLING INPUT
	if(hot_entity_uid >= 0)
		entities[hot_entity_uid].color = {1,1,1,1};	

	if(input->cancel == 1) memory->is_paused = !memory->is_paused;
	if(memory->is_paused) if (input->R != 1) return;

	if(input->debug_up) memory->teams_resources[0]++;

	// if(input->L == 1)
	// 	memory->creating_unit = (memory->creating_unit+(AVAILABLE_UNITS))%(1+AVAILABLE_UNITS);
	// if(input->R == 1)
	// 	memory->creating_unit = ((memory->creating_unit+1) % (1+AVAILABLE_UNITS));

	// DECIDE WHOS THE SELECTED ENTITY 

	if(!is_cursor_in_ui && !input->L){
		if(input->cursor_primary == 1){
			memory->clicked_uid = hot_entity_uid;
			memory->selected_uid = -1;
		}else if( input->cursor_primary == -1 ){
			if(memory->clicked_uid >= 0){
				if(hot_entity_uid == memory->clicked_uid){
					memory->selected_uid = memory->clicked_uid;
					
					Audio_playback* new_playback = find_next_available_playback(playback_list);
					new_playback->initial_sample_t = sample_t;
					new_playback->sound_uid = memory->sounds.pa_uid;
				}else{
					memory->selected_uid = -1;
				}
				memory->clicked_uid = 0;
			}
		}
	}

	// COLOR AND LOOKING DIRECTION OF SELECTED ENTITY
	
	if(memory->selected_uid >= 0){
		Entity* selected_entity = &entities[memory->selected_uid];
		selected_entity->color = {0,0.7f,0,1};

		if( input->cursor_secondary > 0){
			if(!(selected_entity->flags & E_CANNOT_MANUALLY_AIM))
			{
				selected_entity->target_direction = v3_difference({cursor_y0_pos.x, 0, cursor_y0_pos.z}, selected_entity->pos);
			}
		}
	}
	
}



void render(App_memory* memory, LIST(Renderer_request,render_list), Int2 screen_size)
{
	Renderer_request* request = 0;
	PUSH_BACK(render_list, memory->temp_arena,request);
	request->type_flags = REQUEST_FLAG_SET_PS|REQUEST_FLAG_SET_VS|
		REQUEST_FLAG_SET_BLEND_STATE|REQUEST_FLAG_SET_DEPTH_STENCIL;
	request->vshader_uid = memory->vshaders.default_vshader_uid;
	request->pshader_uid = memory->pshaders.default_pshader_uid;
	request->blend_state_uid = memory->blend_states.default_blend_state_uid;
	request->depth_stencil_uid = memory->depth_stencils.default_depth_stencil_uid;

	UNTIL(i, MAX_ENTITIES)
	{
		if(memory->entities[i].flags & E_VISIBLE)
		{
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->object3d = memory->entities[i].object3d;

			f32 scale_multiplier = MAX(memory->delta_time, memory->entities[i].creation_size);

			request->object3d.scale = scale_multiplier * request->object3d.scale;

			if(memory->entities[i].flags & E_TOXIC)
			{
				PUSH_BACK(render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.color = {0.2f, 0.0f , 0.4f, 1.0f};
				V3 yoffset = {0,-1,0};
				request->object3d.pos = memory->entities[i].pos + yoffset;
				request->object3d.scale = v3_multiply(memory->entities[i].aura_radius, {1,1,1});
				request->object3d.rotation = {PI32/2,0,0};
				
				request->object3d.mesh_uid = memory->meshes.centered_plane_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
			if(memory->entities[i].flags & E_HEALER)
			{
				PUSH_BACK(render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.color = {0.0f, 0.5f, 0.1f, 1.0f};
				V3 yoffset = {0, -1, 0};
				request->object3d.pos = memory->entities[i].pos + yoffset;
				request->object3d.scale = v3_multiply(memory->entities[i].aura_radius, {1,1,1});
				request->object3d.rotation = {PI32/2, 0, 0};

				request->object3d.mesh_uid = memory->meshes.centered_plane_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			}
		}
	}
	if(memory->selected_uid >= 0){
		Entity* selected_entity = &memory->entities[memory->selected_uid];
		{

			PUSH_BACK(render_list, memory->temp_arena, request);
				request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
				request->object3d.scale = {1,1,1};
				request->object3d.pos = selected_entity->looking_direction + selected_entity->pos;
				request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
				request->object3d.texinfo_uid = memory->textures.white_tex_uid;
				request->object3d.color = {1,0,0,0.1f};
		}
	}

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_DEPTH_STENCIL|REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_PS;
	request->depth_stencil_uid = memory->depth_stencils.ui_depth_stencil_uid;
	request->vshader_uid = memory->vshaders.ui_vshader_uid;
	request->pshader_uid = memory->pshaders.ui_pshader_uid;
	

	UNTIL(i, MAX_ENTITIES)
	{
		if((memory->entities[i].flags & E_VISIBLE) && 
			(memory->entities[i].type != ENTITY_PROJECTILE) && 
			(memory->entities[i].type != ENTITY_OBSTACLE)
		){
			String health_string = number_to_string(memory->entities[i].health, memory->temp_arena);
			printo_world(memory, screen_size, render_list,
				health_string, 
				memory->entities[i].pos,
				{0,1,0,1}
			);
		}
	}


	printo_screen(memory, screen_size, render_list,
		string("here i will show the fps (probably): 69 fps"), {-1,1}, {1,1,1,1});
	
	String resources_string = number_to_string(memory->teams_resources[memory->entities[memory->player_uid].team_uid], memory->temp_arena);
	printo_screen(memory, screen_size, render_list,
		concat_strings(string("resources: "), resources_string, memory->temp_arena), {-1,.9f}, {1,1,0,1});

	{
		
		Tex_info* texinfo;

		Object3d template_object = {0};
		template_object.mesh_uid = memory->meshes.plane_mesh_uid;
		f32 scale_tex = 5;

		f32 unselected_alpha = 0.4f;
		Color unselected_color = { 1.0f, 1.0f, 1.0f, unselected_alpha};
		Color insuficient_res_color = { 1.0f, 0.5f, 0.5f, unselected_alpha};



		V3 current_pos = {-0.6f, -0.8f, 0.2f};
		Renderer_request* requests [7];
		PUSH_BACK(render_list, memory->temp_arena, requests[0]);
		requests[0]->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
		requests[0]->object3d = template_object;
		requests[0]->object3d.texinfo_uid = memory->font_tex_infos_uids[CHAR_TO_INDEX('0')];

		LIST_GET(memory->tex_infos, requests[0]->texinfo_uid, texinfo);
		V2 normalized_scale;
		normalized_scale = normalize_texture_size(screen_size, {texinfo->w, texinfo->h});
		requests[0]->scale = {scale_tex*normalized_scale.x, scale_tex*normalized_scale.y, 1};

		requests[0]->object3d.pos = current_pos;
		requests[0]->object3d.color = unselected_color;

		u32 resources_value = memory->teams_resources[0];

		current_pos.x += 0.2f;
		String cost_string;
		u32 current_creation_cost = memory->unit_creation_costs[memory->possible_entities[1]];
		cost_string = number_to_string(current_creation_cost, memory->temp_arena);
		printo_screen(memory, screen_size, render_list, cost_string, {current_pos.x, current_pos.y}, {1,1,0,1});

		PUSH_BACK(render_list, memory->temp_arena, requests[1]);
		requests[1]->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
		requests[1]->object3d = template_object;
		requests[1]->object3d.texinfo_uid = memory->font_tex_infos_uids[CHAR_TO_INDEX('1')];

		LIST_GET(memory->tex_infos, requests[1]->texinfo_uid, texinfo);
		normalized_scale = normalize_texture_size(screen_size, {texinfo->w, texinfo->h});
		requests[1]->scale = {scale_tex*normalized_scale.x, scale_tex*normalized_scale.y, 1};

		requests[1]->object3d.pos = current_pos;
		if(resources_value >= current_creation_cost)
			requests[1]->object3d.color = unselected_color;
		else
			requests[1]->object3d.color = insuficient_res_color;
			



		current_pos.x += 0.2f;
		current_creation_cost = memory->unit_creation_costs[memory->possible_entities[2]];
		cost_string = number_to_string(current_creation_cost, memory->temp_arena);
		printo_screen(memory, screen_size, render_list, cost_string, {current_pos.x, current_pos.y}, {1,1,0,1});

		PUSH_BACK(render_list, memory->temp_arena, requests[2]);
		requests[2]->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
		requests[2]->object3d = template_object;
		requests[2]->object3d.texinfo_uid = memory->font_tex_infos_uids[CHAR_TO_INDEX('2')];
		
		LIST_GET(memory->tex_infos, requests[2]->texinfo_uid, texinfo);
		normalized_scale = normalize_texture_size(screen_size, {texinfo->w, texinfo->h});
		requests[2]->scale = {scale_tex*normalized_scale.x, scale_tex*normalized_scale.y, 1};

		requests[2]->object3d.pos = current_pos;
		if(resources_value >= current_creation_cost)
			requests[2]->object3d.color = unselected_color;
		else
			requests[2]->object3d.color = insuficient_res_color;



		current_pos.x += 0.2f;
		current_creation_cost = memory->unit_creation_costs[memory->possible_entities[3]];
		cost_string = number_to_string(current_creation_cost, memory->temp_arena);
		printo_screen(memory, screen_size, render_list, cost_string, {current_pos.x, current_pos.y}, {1,1,0,1});

		PUSH_BACK(render_list, memory->temp_arena, requests[3]);
		requests[3]->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
		requests[3]->object3d = template_object;
		requests[3]->object3d.texinfo_uid = memory->font_tex_infos_uids[CHAR_TO_INDEX('3')];
		
		LIST_GET(memory->tex_infos, requests[3]->texinfo_uid, texinfo);
		normalized_scale = normalize_texture_size(screen_size, {texinfo->w, texinfo->h});
		requests[3]->scale = {scale_tex*normalized_scale.x, scale_tex*normalized_scale.y, 1};

		requests[3]->object3d.pos = current_pos;
		if(resources_value >= current_creation_cost)
			requests[3]->object3d.color = unselected_color;
		else
			requests[3]->object3d.color = insuficient_res_color;



		current_pos.x += 0.2f;
		current_creation_cost = memory->unit_creation_costs[memory->possible_entities[4]];
		cost_string = number_to_string(current_creation_cost, memory->temp_arena);
		printo_screen(memory, screen_size, render_list, cost_string, {current_pos.x, current_pos.y}, {1,1,0,1});

		PUSH_BACK(render_list, memory->temp_arena, requests[4]);
		requests[4]->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
		requests[4]->object3d = template_object;
		requests[4]->object3d.texinfo_uid = memory->font_tex_infos_uids[CHAR_TO_INDEX('4')];
		
		LIST_GET(memory->tex_infos, requests[4]->texinfo_uid, texinfo);
		normalized_scale = normalize_texture_size(screen_size, {texinfo->w, texinfo->h});
		requests[4]->scale = {scale_tex*normalized_scale.x, scale_tex*normalized_scale.y, 1};

		requests[4]->object3d.pos = current_pos;
		if(resources_value >= current_creation_cost)
			requests[4]->object3d.color = unselected_color;
		else
			requests[4]->object3d.color = insuficient_res_color;


		current_pos.x += 0.2f;
		Renderer_request* selected = requests[memory->creating_unit];
		selected->object3d.scale = 1.3f*selected->scale;
		selected->object3d.color.a = 1.0f;
	}


	// RENDERING UI ELEMENTS


	// FIRST PASS TO DRAW RECTANGLES

	UNTIL(i, MAX_UI){
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
		}
	}

	// LINE DRAWING

	User_input* input = memory->input;
	if(
		input->L && (
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

	UNTIL(i, MAX_UI){
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
	// TODO: when this happens, lift the assert and implement bigger bitwise flags
	ASSERT(E_LAST_FLAG < (0xfffffffffffffff)); // asserting there are not more than 60 flags
	// ASSERT(E_LAST_FLAG < 0Xffffffff); // asserting there are not more than 32 flags


	memory->entities = ARENA_PUSH_STRUCTS(memory->permanent_arena, Entity, MAX_ENTITIES);
	memory->entity_generations = ARENA_PUSH_STRUCTS(memory->permanent_arena, u32, MAX_ENTITIES);

	memory->ui_elements = ARENA_PUSH_STRUCTS(memory->permanent_arena, Ui_element, MAX_UI);
	memory->ui_generations = ARENA_PUSH_STRUCTS(memory->permanent_arena, u32, MAX_UI);

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
	}



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
			{string(":centered_plane_mesh:"), &memory->meshes.centered_plane_mesh_uid},
			{string(":plane_mesh:"), &memory->meshes.plane_mesh_uid},
			{string(":icosphere_mesh:"), &memory->meshes.icosphere_mesh_uid},
			{string(":player_mesh:"), &memory->meshes.player_mesh_uid},
			{string(":spawner_mesh:"), &memory->meshes.spawner_mesh_uid},
			{string(":boss_mesh:"), &memory->meshes.boss_mesh_uid},
			{string(":tank_mesh:"), &memory->meshes.tank_mesh_uid},
			{string(":shield_mesh:"), &memory->meshes.shield_mesh_uid},
			{string(":shooter_mesh:"), &memory->meshes.shooter_mesh_uid},
			{string(":melee_mesh:"), &memory->meshes.melee_mesh_uid},
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
			{string(":gradient_tex:"), &memory->textures.gradient_tex_uid}
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