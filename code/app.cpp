#include "app.h"

#define MAX_SPAWN_DISTANCE 5.0f
#define BOSS_INDEX 1
void update(App_memory* memory, Audio_playback* playback_list, u32 sample_t){
	if(!memory->is_initialized){
		memory->is_initialized = true;
		
		set_mem(memory->entities, MAX_ENTITIES*sizeof(Entity), 0);
		set_mem(memory->entity_generations, MAX_ENTITIES*sizeof(u32), 0);

		memory->camera_rotation.x = PI32/4;
		memory->camera_rotation.y = 0;
		memory->camera_rotation.z = 0;
		memory->camera_pos.y = 32.0f;

		memory->last_inactive_entity = 0;


		Entity* player = &memory->entities[memory->player_uid];
		default_object3d(player);
		player->pos = {-25, 0, 0};
		player->max_health = 10;
		player->health = player->max_health;
		player->team_uid = 0;
		memory->teams_resources[player->team_uid] = 30;
		player->flags = VISIBLE|ACTIVE;
		player->current_scale = 1.0f;

		player->team_uid = 0;
		player->speed = 5.0f;
		player->type = ENTITY_UNIT;

		player->mesh_uid = memory->meshes.player_mesh_uid;
		player->texinfo_uid = memory->textures.white_tex_uid;
		
		Entity* boss = &memory->entities[BOSS_INDEX];
		default_object3d(boss);
		boss->speed = 40.0f;
		boss->shooting_cooldown = 2.0f;
		boss->shooting_cd_time_left = boss->shooting_cooldown;

		boss->flags = VISIBLE;
		boss->max_health = 100;
		boss->health = boss->max_health;
		boss->pos = {25, 0, 0};
		boss->target_move_pos = boss->pos;
		boss->team_uid = 1;
		boss->current_scale = 1.0f;
		boss->type = ENTITY_BOSS;

		boss->mesh_uid = memory->meshes.boss_mesh_uid;;
		boss->texinfo_uid = memory->textures.default_tex_uid;

		memory->unit_creation_costs[UNIT_SHOOTER] = 20; // 20
		memory->unit_creation_costs[UNIT_TANK] = 15;
		memory->unit_creation_costs[UNIT_SPAWNER] = 40;
		memory->unit_creation_costs[UNIT_MELEE] = 5;
	}

	User_input* input = memory->input;
	User_input* holding_inputs = memory->holding_inputs;
	Entity* entities = memory->entities;
	u32* generations = memory->entity_generations;
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

	V2 input_vector = {(f32)(holding_inputs->d_right - holding_inputs->d_left),(f32)(holding_inputs->d_up - holding_inputs->d_down)};
	input_vector = normalize(input_vector);
	{
		// MOVE CAMERA IN THE DIRECTION I AM LOOKING
		// V2 looking_direction = {cosf(memory->camera_rotation.y), sinf(memory->camera_rotation.y)};
		// V2 move_direction = {
		// 	input_vector.x*looking_direction.x + input_vector.y*looking_direction.y ,
		// 	-input_vector.x*looking_direction.y + input_vector.y*looking_direction.x
		// };
		// memory->camera_pos.x += move_direction.x * delta_time * camera_speed;
		// memory->camera_pos.z += move_direction.y * delta_time * camera_speed;

		// memory->camera_pos.y += (input->up - input->down) * delta_time * camera_speed;


		Entity* wall = 0;
		wall = &entities[5];
		wall->flags = VISIBLE | ACTIVE;
		wall->current_scale = 1.0f;
		wall->type = ENTITY_OBSTACLE;
		wall->mesh_uid = memory->meshes.cube_mesh_uid;
		wall->texinfo_uid = memory->textures.white_tex_uid;
		wall->scale = {1,1,45};
		wall->pos = {-29, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		
		wall = &entities[6];
		wall->flags = VISIBLE | ACTIVE;
		wall->current_scale = 1.0f;
		wall->type = ENTITY_OBSTACLE;
		wall->mesh_uid = memory->meshes.cube_mesh_uid;
		wall->texinfo_uid = memory->textures.white_tex_uid;
		wall->scale = {1,1,45};
		wall->pos = {28, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
		
		wall = &entities[7];
		wall->flags = VISIBLE | ACTIVE;
		wall->current_scale = 1.0f;
		wall->type = ENTITY_OBSTACLE;
		wall->mesh_uid = memory->meshes.cube_mesh_uid;
		wall->texinfo_uid = memory->textures.white_tex_uid;
		wall->scale = {56,1,1};
		wall->pos = {-28, 0, -23};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};

		wall = &entities[8];
		wall->flags = VISIBLE | ACTIVE;
		wall->current_scale = 1.0f;
		wall->type = ENTITY_OBSTACLE;
		wall->mesh_uid = memory->meshes.cube_mesh_uid;
		wall->texinfo_uid = memory->textures.white_tex_uid;
		wall->scale = {56,1,1};
		wall->pos = {-28, 0, 21};
		wall->color = {0.3f,0.3f,0.3f,1};
		wall->rotation = {0,0,0};
	}

	V3 cursor_pos = {
		memory->aspect_ratio*memory->fov*input->cursor_pos.x,
		memory->fov*input->cursor_pos.y, 0
	};

	// RAYCAST WITH ALL ENTITIES
	memory->highlighted_uid = 0;
	V3 cursor_screen_to_world_pos = memory->camera_pos + v3_rotate_y(
		v3_rotate_x(cursor_pos, memory->camera_rotation.x),memory->camera_rotation.y
	);

	V3 z_direction = v3_rotate_y(
		v3_rotate_x({0,0,1}, memory->camera_rotation.x),memory->camera_rotation.y
	);

	V3 cursor_world_pos = line_intersect_y0(cursor_screen_to_world_pos, z_direction);

	f32 closest_t = {0};
	b32 first_intersection = false;
	// CURSOR RAYCASTING
	UNTIL(i, MAX_ENTITIES){
		if((entities[i].flags & VISIBLE) && 
			(entities[i].flags & SELECTABLE) &&
			entities[i].team_uid == entities[memory->player_uid].team_uid 
		){
			
			entities[i].color = {0.5f,0.5f,0.5f,1};

			f32 intersected_t = 0;
			if(line_vs_sphere(cursor_world_pos, z_direction, 
				entities[i].pos, entities[i].scale.x, 
				&intersected_t)
			){
				if(!first_intersection){
					first_intersection = true;
					closest_t = intersected_t;
					memory->highlighted_uid = i;
				}
				if(intersected_t < closest_t){
					closest_t = intersected_t;
					memory->highlighted_uid = i;
				}
			}
		}
	}
	// HANDLING INPUT
	entities[memory->highlighted_uid].color = {1,1,1,1};	
	if(input->cancel == 1) memory->is_paused = !memory->is_paused;
	if(memory->is_paused) if (input->R != 1) return;

	// if(input->L == 1)
	// 	memory->creating_unit = (memory->creating_unit+(AVAILABLE_UNITS))%(1+AVAILABLE_UNITS);
	// if(input->R == 1)
	// 	memory->creating_unit = ((memory->creating_unit+1) % (1+AVAILABLE_UNITS));
	if(input->L)
		memory->creating_unit = 0;
	else if(input->k1 == 1)
		memory->creating_unit = 1;
	else if(input->k2 == 1)
		memory->creating_unit = 2;
	else if(input->k3 == 1)
		memory->creating_unit = 3;

	memory->possible_entities[0] = UNIT_NOT_A_UNIT;
	memory->possible_entities[1] = UNIT_SHOOTER;
	memory->possible_entities[2] = UNIT_TANK;
	memory->possible_entities[3] = UNIT_MELEE;
		
	UNIT_TYPE new_unit_type = memory->possible_entities[memory->creating_unit];
	if(new_unit_type == UNIT_SHOOTER) { // SELECTED UNIT TO CREATE
		if(input->cursor_primary == -1){
			// CREATING SHOOTER
			if(memory->teams_resources[entities[memory->player_uid].team_uid] > memory->unit_creation_costs[new_unit_type]){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= memory->unit_creation_costs[new_unit_type];
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				default_shooter(new_unit, memory);

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				// new_unit->rotation.y = 0;
				new_unit->target_pos = entities[BOSS_INDEX].pos;
			}
		}
	} else if ( new_unit_type == UNIT_SPAWNER){
		if(input->cursor_primary == -1){
			// CREATING SPAWNER
			if(memory->teams_resources[entities[memory->player_uid].team_uid] > memory->unit_creation_costs[new_unit_type]){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= memory->unit_creation_costs[new_unit_type];
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				default_spawner(new_unit, memory);

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				// new_unit->rotation.y = 0;
				new_unit->target_pos = entities[BOSS_INDEX].pos;
			}
		}
	} else if ( new_unit_type == UNIT_TANK){
		if(input->cursor_primary == -1){
			// CREATING TANK
			if(memory->teams_resources[entities[memory->player_uid].team_uid] >= memory->unit_creation_costs[new_unit_type]){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= memory->unit_creation_costs[new_unit_type];
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				default_tank(new_unit, memory);

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				// new_unit->rotation.y = 0;
				new_unit->target_pos = entities[BOSS_INDEX].pos;
				new_unit->looking_at = v3_normalize(new_unit->target_pos - new_unit->pos) + new_unit->pos;
				
				Audio_playback* new_playback = find_next_available_playback(playback_list);
				new_playback->initial_sample_t = sample_t;
				new_playback->sound_uid = memory->sounds.wa_uid;
			}
		}
	} else if( new_unit_type == UNIT_MELEE) {
		if(input->cursor_primary == -1){
			if(memory->teams_resources[entities[memory->player_uid].team_uid] >= memory->unit_creation_costs[new_unit_type]){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= memory->unit_creation_costs[new_unit_type];
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				
				default_melee(new_unit, memory);

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				new_unit->target_pos = entities[BOSS_INDEX].pos;

				Audio_playback* new_playback = find_next_available_playback(playback_list);
				new_playback->initial_sample_t = sample_t;
				new_playback->sound_uid = memory->sounds.wa_uid;

			}
		}
	} else {  // NO SELECTED UNIT TO CREATE
		if(input->cursor_primary == 1)
			memory->clicked_uid = memory->highlighted_uid;
		else if( input->cursor_primary == -1 ){
			if(memory->clicked_uid){
				if(memory->highlighted_uid == memory->clicked_uid){
					memory->selected_uid = memory->clicked_uid;
				}
				memory->clicked_uid = 0;
			}
		}

		Entity* selected_entity = &entities[memory->selected_uid];
		selected_entity->color = {0,0.7f,0,1};

		if( memory->selected_uid != memory->player_uid ){
			if( input->cursor_secondary > 0)
				selected_entity->target_pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
			else if( input->cursor_primary > 0)
				memory->selected_uid = memory->player_uid;
			if( input->move > 0)
				selected_entity->target_move_pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
		}
	}	

	// UPDATING ENTITIES
	UNTIL(i, MAX_ENTITIES){
		Entity* entity = &entities[i]; 
		if(!(entity->flags & VISIBLE)) continue;
		if(entity->type == ENTITY_OBSTACLE) continue;

		if(entity->team_uid != 0)
			entity->color = {0.7f,0,0,1};
			
		if( !(entity->flags & ACTIVE)){
			if(entity->current_scale < 1.0f){
				entity->current_scale += delta_time;
				continue;
			}else{
				entity->current_scale = 1.0f;
				entity->flags |= ACTIVE;
				if(entity->unit_type == UNIT_TANK){
					//TODO: create shield
					u32 new_entity_index = last_inactive_entity(entities);
					Entity* new_shield= &entities[new_entity_index];
					default_shield(new_shield, memory);
					Entity* parent = entity;
					new_shield->parent_handle.index = i;
					new_shield->parent_handle.generation = generations[i];
					new_shield->team_uid = parent->team_uid;
					new_shield->pos = parent->looking_at;
					new_shield->target_move_pos = parent->looking_at;
					// TODO: go in the direction that parent is looking (the parent's rotation);
					new_shield->target_pos = new_shield->pos + (parent->looking_at - parent->pos);
					new_shield->looking_at = new_shield->target_pos;
					V3 looking_direction = (new_shield->looking_at - new_shield->pos);
					new_shield->rotation.y = v2_angle({looking_direction.x, looking_direction.z}) + PI32/2;
				}
			}

		}else if(entity->type == ENTITY_BOSS){ // BEGINNING OF BOSS CODE 
			entity->target_pos = entities[memory->player_uid].pos;
			entity->color = {0.7f,0,0,1};

			entity->shooting_cd_time_left -= delta_time;
			if(entity->shooting_cd_time_left < 0){
				if(entity->health > 75){
					switch(entity->state){
						case 0:
						case 6:{
							entity->state = 1;
							entity->shooting_cd_time_left += 3.0f;
							// entity->target_move_pos = {20, 0, 10};
						}break; 
						case 1:
						case 4:{
							entity->state++;
							entity->shooting_cd_time_left += 1.0f;
					
							V3 target_direction = v3_normalize(entity->target_pos - entity->pos);
							u32 shoots_count = 24;
							UNTIL(shot, shoots_count)
							{
								u32 new_entity_index = next_inactive_entity(entities,&memory->last_inactive_entity);
								Entity* new_bullet = &entities[new_entity_index];
								default_projectile(new_bullet, memory);

								new_bullet->health = 1; 
								new_bullet->speed = 50;

								Entity* parent = entity;
								new_bullet->parent_handle.index = i;
								new_bullet->parent_handle.generation = generations[i];
								new_bullet->team_uid = parent->team_uid;
								new_bullet->pos = parent->pos;
								
								new_bullet->target_pos = parent->looking_at;

								new_bullet->velocity =  new_bullet->speed * target_direction;

								target_direction = v3_rotate_y(target_direction, TAU32/shoots_count);
							}

						}break; 
						case 2:
						case 5:{
							entity->state++;
							entity->shooting_cd_time_left += 2.0f;
							// spawn entity
							u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
							V3 target_distance = entity->target_pos - entity->pos;
							f32 target_distance_magnitude = v3_magnitude(target_distance);
							V3 spawn_pos;
							if(target_distance_magnitude > MAX_SPAWN_DISTANCE)
								spawn_pos = entity->pos + (MAX_SPAWN_DISTANCE * v3_normalize(target_distance));
							else
								spawn_pos = entity->target_pos;

							Entity* new_unit = &entities[new_entity_index];
							default_shooter(new_unit, memory);

							new_unit->pos = spawn_pos;
							new_unit->target_move_pos = new_unit->pos;

							new_unit->team_uid = entity->team_uid;
							// new_unit->rotation.y = 0;
							new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});

						}break;
						case 3:{
							entity->state++;
							entity->shooting_cd_time_left += 4.0f;
							// entity->target_move_pos = {-20, 0, 10};
						}break;
						default:
							ASSERT(false);
						break;
					}

				} else if (entity->health > 40){
					if(entity->state < 10) entity->state = 10; 

					switch(entity->state){
						case 10:
						case 24:{
							entity->state = 11;
							entity->shooting_cd_time_left += 1.0f;
							// entity->target_move_pos = {20, 0, 11};

						}break;
						case 11:
						case 18:{
							entity->state++;
							entity->shooting_cd_time_left = delta_time;

							// spawn 2 entities
							V3 target_distance = entity->target_pos - entity->pos;
							f32 target_distance_magnitude = v3_magnitude(target_distance);
							V3 target_direction = v3_normalize(target_distance);

							V3 spawn_direction = MIN(MAX_SPAWN_DISTANCE, target_distance_magnitude) * (target_direction);
							spawn_direction = v3_rotate_y(spawn_direction, -TAU32/4);
							UNTIL(e, 2){
								u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);

								Entity* new_unit = &entities[new_entity_index];
								default_shooter(new_unit, memory);

								new_unit->pos = entity->pos + spawn_direction;
								new_unit->target_move_pos = new_unit->pos;

								new_unit->team_uid = entity->team_uid;
								// new_unit->rotation.y = 0;
								new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
								
								spawn_direction = v3_rotate_y(spawn_direction, TAU32/2);
							}
							
						}break;
						case 12:
						case 19:{
							entity->state++;
							entity->shooting_cd_time_left += 1;
							// entity->target_move_pos = {0, 0, 9};

						}break;
						case 13:
						case 14:
						case 15:
						case 20:
						case 21:
						case 22:{
							entity->state++;
							entity->shooting_cd_time_left += 0.5f;

							V3 target_direction = v3_normalize(entity->looking_at - entity->pos);
							u32 shoots_count = 48;
							UNTIL(shot, shoots_count){
								u32 new_entity_index = next_inactive_entity(entities,&memory->last_inactive_entity);
								Entity* new_bullet = &entities[new_entity_index];
								default_projectile(new_bullet, memory);
								//TODO: for now this is just so it doesn't disappear
								// actually it could be a feature :)
								new_bullet->health = 1; 
								new_bullet->speed = 50;

								Entity* parent = entity;
								new_bullet->parent_handle.index = i;
								new_bullet->parent_handle.generation = generations[i];
								new_bullet->team_uid = parent->team_uid;
								new_bullet->pos = parent->pos;
								// TODO: go in the direction that parent is looking (the parent's rotation);
								new_bullet->target_pos = parent->looking_at;

								new_bullet->velocity =  new_bullet->speed * target_direction;

								target_direction = v3_rotate_y(target_direction, TAU32/shoots_count);
							}

						}break;
						case 16:
						case 23:{
							entity->state++;
							entity->shooting_cd_time_left += 7;
						}break;
						case 17:{
							entity->state++;
							entity->shooting_cd_time_left += 1.0f;
							// entity->target_move_pos = {-20, 0, 11};
						}break;
						default:
							ASSERT(false);
						break;
					}

				} else if (entity->health > 20){

				} else {

				}
			}
		// END OF BOSS CODE
		}
		else if (entity->type == ENTITY_SHIELD)
		{
			Entity* parent = entity_from_handle(entities, generations, entity->parent_handle);
			if(parent->flags & ACTIVE){
				entity->target_move_pos = (0.1f*parent->velocity)+parent->pos + v3_normalize((parent->target_pos)- parent->pos);
				entity->target_pos = entity->pos + (entity->pos - parent->pos);
				entity->looking_at = entity->target_pos;
			}else{
				*entity = {0};
			}

		}
		else if ( entity->type == ENTITY_UNIT ) 
		{
			if(entity->team_uid != 0){
				entity->color = {0.7f,0,0,1};
				entity->target_pos = entities[memory->player_uid].pos;
			}

			entity->shooting_cd_time_left -= delta_time;
			if(entity->shooting_cd_time_left < 0){
				entity->shooting_cd_time_left = entity->shooting_cooldown;
			
				if( entity->unit_type == UNIT_SHOOTER ){
					V3 target_direction = v3_rotate_y(v3_normalize( entity->looking_at - entity->pos ), -TAU32/20);
					UNTIL(iterator, 3){
						u32 new_entity_index = next_inactive_entity(entities,&memory->last_inactive_entity);
						Entity* new_bullet = &entities[new_entity_index];
						default_projectile(new_bullet, memory);
						//TODO: for now this is just so it doesn't disappear
						// actually it could be a feature :)
						new_bullet->health = 1; 
						new_bullet->speed = 50;

						Entity* parent = entity;
						new_bullet->parent_handle.index = i;
						new_bullet->parent_handle.generation = generations[i];
						new_bullet->team_uid = parent->team_uid;
						new_bullet->pos = parent->pos;
						// TODO: go in the direction that parent is looking (the parent's rotation);
						new_bullet->target_pos = parent->looking_at;

						new_bullet->velocity =  new_bullet->speed * target_direction;
						target_direction = v3_rotate_y(target_direction, TAU32/20);
					}

				}else if( entity->unit_type == UNIT_TANK ){
					
					V3 target_direction = v3_normalize( entity->looking_at - entity->pos );
					u32 new_entity_index = next_inactive_entity(entities,&memory->last_inactive_entity);
					Entity* new_bullet = &entities[new_entity_index];
					default_projectile(new_bullet, memory);
					//TODO: for now this is just so it doesn't disappear
					// actually it could be a feature :)
					new_bullet->health = 1; 
					new_bullet->speed = 50;

					Entity* parent = entity;
					new_bullet->parent_handle.index = i;
					new_bullet->parent_handle.generation = generations[i];
					new_bullet->team_uid = parent->team_uid;
					new_bullet->pos = parent->pos;
					// TODO: go in the direction that parent is looking (the parent's rotation);
					new_bullet->target_pos = parent->looking_at;

					new_bullet->velocity =  new_bullet->speed * target_direction;

				} else if( entity->unit_type == UNIT_SPAWNER){
					u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
					V3 target_distance = entity->target_pos - entity->pos;
					f32 target_distance_magnitude = v3_magnitude(target_distance);
					V3 spawn_pos;
					if(target_distance_magnitude > MAX_SPAWN_DISTANCE)
						spawn_pos = entity->pos + (MAX_SPAWN_DISTANCE * v3_normalize(target_distance));
					else
						spawn_pos = entity->target_pos;

					Entity* new_unit = &entities[new_entity_index];
					default_shooter(new_unit, memory);

					new_unit->pos = spawn_pos;
					new_unit->target_move_pos = new_unit->pos;

					new_unit->team_uid = entity->team_uid;
					// new_unit->rotation.y = 0;
					new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
				}
			}
		}
		
		{// DYNAMICS
			entity->pos.y = 0;//TODO: clamping height position
			if(i == memory->player_uid){
				entity->target_move_pos = v3_addition(entity->pos, {input_vector.x*entity->speed, 0, input_vector.y*entity->speed});
				V3 move_v = (entity->target_move_pos - entity->pos);
				V3 accel = 10*(move_v - entity->velocity);
				entity->velocity = entity->velocity + (delta_time * accel);
				if(entity->velocity.x || entity->velocity.z)
					entity->rotation.y = v2_angle({entity->velocity.x, entity->velocity.z}) + PI32/2;
				entity->pos.x = CLAMP(-27, entity->pos.x, 27);
				entity->pos.z = CLAMP(-21, entity->pos.z, 21);	
					
			}else if(entity->type != ENTITY_PROJECTILE){
				entity->pos.x = CLAMP(-27, entity->pos.x, 27);
				entity->pos.z = CLAMP(-21, entity->pos.z, 21);

				V3 move_distance = (entity->target_move_pos - entity->pos);
				// V3 accel = entity->speed*(move_v - (0.4f*entity->velocity));

				// f32 temp_4log10_speed = 0.025f;
				// f32 temp_4log10_speed = 0.1f;
				// V3 accel = ((entity->speed)*move_distance) - ((entity->speed*temp_4log10_speed)*entity->velocity);
				// f32 vel_magnitude = v3_magnitude(entity->velocity);
				// f32 new_magnitude = ((1-(vel_magnitude/500)));
				// V3 v_n = {
				// 	entity->velocity.x*(1-(entity->velocity.x/10000)),
				// 	entity->velocity.y*(1-(entity->velocity.y/10000)),
				// 	entity->velocity.z*(1-(entity->velocity.z/10000))
				// };
				// TODO: get an ecuation that solves acceleration depending on the distance left to the target
				V3 accel = entity->speed*( move_distance - (0.1f*entity->velocity) ) - (10*(entity->velocity));
				entity->velocity = entity->velocity +(delta_time*( accel ));
				// V3 accel = entity->speed*delta_time*(move_distance);
				// entity->velocity = entity->velocity +(delta_time*( accel ));
				// V3 accel = ((move_distance)-entity->velocity);
				// entity->velocity = entity->velocity + (10*delta_time*( accel ));

				//TODO: when unit is moving and shooting, shoots seem to come from the body
				// and that's because it should spawn in the tip of the cannon instead of the center
				//LERPING TARGET POS
				entity->looking_at = entity->looking_at + (10*delta_time * (entity->target_pos - entity->looking_at));
				V3 target_direction = entity->looking_at - entity->pos;
				f32 target_rotation = v2_angle({target_direction.x, target_direction.z}) + PI32/2;
				entity->rotation.y = target_rotation;

				//LERPING ROTATION
				// V3 target_direction = entity->target_pos - entity->pos;
				// f32 target_rotation = v2_angle({target_direction.x, target_direction.z})+ PI32/2;

				// f32 angle_difference = target_rotation - entity->rotation.y;
				// if(angle_difference > TAU32/2)
				// 	entity->rotation.y += TAU32;
				// else if(angle_difference < -TAU32/2)
				// 	entity->rotation.y -= TAU32;
				// entity->rotation.y += 10*(target_rotation - entity->rotation.y) * delta_time;
			}
			if(entity->lifetime){
				entity->lifetime -= delta_time;
				if(entity->lifetime < 0){
					*entity = {0};
					memory->entity_generations[i]++;
				}
			}
			// COLLISIONS
				// PROJECTILE
			if(entity->type == ENTITY_PROJECTILE){
				UNTIL(j, MAX_ENTITIES){
					if(!(entities[j].flags & VISIBLE)) continue;
					Entity* entity2 = &entities[j];
					if(entity2->type == ENTITY_OBSTACLE){
						V3 distance = sphere_vs_box(entity->pos, entity2->pos, entity2->pos+entity2->scale);
						f32 distance_value = v3_magnitude(distance);
						if(distance_value < entity->current_scale){
							*entity = {0};
							memory->entity_generations[i]++;
							break;
						}
					} else if (entity->team_uid != entity2->team_uid){
						f32 intersect = sphere_vs_sphere(entity->pos, entity->scale.x, entity2->pos, entity2->scale.x);
						if(intersect > 0){
							entity2->health -= 1;
							if(entity2->type == ENTITY_SHIELD)
								memory->teams_resources[entity2->team_uid] += 2;
							if(entity2->health <= 0){
								if(entity2->type == ENTITY_UNIT){
									s32 reward_value = memory->unit_creation_costs[entity2->unit_type]/2;
									memory->teams_resources[entity->team_uid] += reward_value;
									memory->teams_resources[entity2->team_uid] += reward_value;
								}
								*entity2 = {0};
								memory->entity_generations[j]++;
							}
							*entity = {0}; 
							memory->entity_generations[i]++;
							break;
						}
					}
				}
				// UNIT
			}else if (entity->type == ENTITY_UNIT){
				UNTIL(j, MAX_ENTITIES){
					if( i!=j && (entities[j].flags & VISIBLE) ){
						Entity* entity2 = &entities[j];
						if(entity2->type == ENTITY_UNIT){
							V3 pos_difference = entity2->pos-entity->pos;
							f32 collision_magnitude = v3_magnitude(pos_difference);
							//sphere vs sphere simplified
							f32 overlapping = ((entity->scale.x * entity->current_scale)+(entity2->scale.x * entity2->current_scale)) - collision_magnitude;
							if(overlapping > 0){
								V3 collision_direction = v3_normalize(pos_difference);
								if(!collision_magnitude)
									collision_direction = {1.0f,0,0};
								overlapping =  MIN(MIN(entity->current_scale, entity2->current_scale),overlapping);
								//TODO: maybe get rid of all divisions of delta time
								entity->velocity = entity->velocity - (((overlapping/delta_time)/2) * collision_direction);
								entity2->velocity = (entity2->velocity + (((overlapping/delta_time)/2) * collision_direction));
							}
						}
						else if(entity2->type == ENTITY_OBSTACLE){
							f32 sphere_radius = entity->current_scale;
							V3 distance = sphere_vs_box(entity->pos, entity2->pos, entity2->pos+entity2->scale);
							f32 distance_value = v3_magnitude(distance);
							// checking if distance is less than the sphere radius
							if(distance_value < sphere_radius){
								// entity->pos = entity->pos + ((sphere_radius-distance_value)/delta_time)*v3_normalize(distance);
								f32 vel_magnitude = v3_magnitude(entity->velocity);
								entity->velocity = entity->velocity + (vel_magnitude * v3_normalize_with_magnitude(distance,distance_value));
							}
						}
					}
				}
			}	
		}
		entity->pos = entity->pos + (delta_time * entity->velocity);
	}
}



void render(App_memory* memory, LIST(Renderer_request,render_list), Int2 screen_size){
	Renderer_request* request = 0;
	PUSH_BACK(render_list, memory->temp_arena,request);
	request->type_flags = REQUEST_FLAG_SET_PS|REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_BLEND_STATE|REQUEST_FLAG_SET_DEPTH_STENCIL;
	request->vshader_uid = memory->vshaders.default_vshader_uid;
	request->pshader_uid = memory->pshaders.default_pshader_uid;
	request->blend_state_uid = memory->blend_states.default_blend_state_uid;
	request->depth_stencil_uid = memory->depth_stencils.default_depth_stencil_uid;

	UNTIL(i, MAX_ENTITIES)
	{
		if(memory->entities[i].flags & VISIBLE)
		{
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->object3d = memory->entities[i].object3d;
			request->object3d.scale = memory->entities[i].current_scale * request->object3d.scale;
		}
	}
	if(memory->player_uid != memory->selected_uid)
	{
		PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			Entity* selected_entity = &memory->entities[memory->selected_uid];
			request->object3d.scale = {1,1,1};
			request->object3d.pos = selected_entity->looking_at;
			request->object3d.mesh_uid = memory->meshes.icosphere_mesh_uid;
			request->object3d.texinfo_uid = memory->textures.white_tex_uid;
			request->object3d.color = {1,0,0,0.1f};
	}

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_DEPTH_STENCIL|REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_PS;
	request->depth_stencil_uid = memory->depth_stencils.ui_depth_stencil_uid;
	request->vshader_uid = memory->vshaders.ui_vshader_uid;
	request->pshader_uid = memory->pshaders.ui_pshader_uid;
	

	UNTIL(i, MAX_ENTITIES)
	{
		if((memory->entities[i].flags & VISIBLE) && 
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
		Renderer_request* requests [4];
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
		Renderer_request* selected = requests[memory->creating_unit];
		selected->object3d.scale = 1.3f*selected->scale;
		selected->object3d.color.a = 1.0f;
	}

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN;
	request->scale = {2, 0.1f,1};
	request->color = {1,1,1,1};
	request->texinfo_uid = memory->textures.gradient_tex_uid;
	request->mesh_uid = memory->meshes.plane_mesh_uid;
	request->pos = { -1, -0.9f, 0.01f};

	// draw(render_list, memory->temp_arena, &test_plane);
}

#define PUSH_ASSET_REQUEST push_asset_request(memory, init_data, &request)
void init(App_memory* memory, Init_data* init_data){	

	memory->entities = ARENA_PUSH_STRUCTS(memory->permanent_arena, Entity, MAX_ENTITIES);
	memory->entity_generations = ARENA_PUSH_STRUCTS(memory->permanent_arena, u32, MAX_ENTITIES);

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
		request.enable_depth = true;
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