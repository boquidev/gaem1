#include "app.h"

void update(App_memory* memory){
	User_input* input = memory->input;
	User_input* holding_inputs = memory->holding_inputs;
	Entity* entities = memory->entities;

	r32 delta_time = 1;
	r32 camera_speed = 1.0f;
	r32 sensitivity = 2.0f;

	memory->camera_rotation.y += sensitivity*(r32)input->cursor_speed.x;
	memory->camera_rotation.x += -sensitivity*(r32)input->cursor_speed.y;
	memory->camera_rotation.x = CLAMP(-PI32/2, memory->camera_rotation.x, PI32/2);	

	V2 input_vector = {(r32)(holding_inputs->d_right - holding_inputs->d_left),(r32)(holding_inputs->d_up - holding_inputs->d_down)};
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

		Entity* mc = &entities[memory->player_uid];
		DEFAULT_ENTITY(mc);
		mc->active = true;
		mc->current_scale = 1.0f;

		mc->health = 10;

		mc->team_uid = 0;
		mc->speed = 10.0f;
		mc->type = ENTITY_QUEEN;
		mc->mesh_uid = memory->meshes.ball_uid;
		mc->tex_uid = memory->textures.white_tex_uid;
		// mc->pos.x += input_vector.x *  mc->speed * delta_time;
		// mc->pos.z += input_vector.y * mc->speed * delta_time;
		mc->target_move_pos = v3_addition(mc->pos, {input_vector.x*mc->speed, 0, input_vector.y*mc->speed});
	}

	//TODO: make this into a function screen to world
	V3 cursor_pos = {
		memory->aspect_ratio*memory->fov*input->cursor_pos.x,
		memory->fov*input->cursor_pos.y, 0};

	// RAYCAST WITH ALL ENTITIES
	memory->highlighted_uid = 0;
	V3 cursor_world_pos = v3_rotate_y(
		v3_rotate_x(cursor_pos, memory->camera_rotation.x),memory->camera_rotation.y
	);
	V3 z_direction = v3_rotate_y(
		v3_rotate_x({0,0,1}, memory->camera_rotation.x),memory->camera_rotation.y
	);

	r32 closest_t = {0};
	b32 first_intersection = false;
	// CURSOR RAYCASTING
	UNTIL(i, MAX_ENTITIES){
		if(entities[i].active && 
			entities[i].selectable &&
			entities[i].team_uid == entities[memory->player_uid].team_uid 
		){
			
			entities[i].color = {1,1,1,1};

			r32 intersected_t = 0;
			if(line_vs_sphere(cursor_world_pos, z_direction, 
				entities[i].object3d.pos, entities[i].scale.x, 
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
	entities[memory->highlighted_uid].color = {1,1,0,1};	
	if(input->L == 1)
		memory->creating_unit = (memory->creating_unit+2)%3;
	if(input->R == 1)
		memory->creating_unit = ((memory->creating_unit+1) % 3);
	if(memory->creating_unit == 1) { // SELECTED UNIT TO CREATE
		if(input->cursor_primary == -1){
			// CREATING UNIT
			if(memory->teams_resources[entities[memory->player_uid].team_uid] > 0){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= 1;
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				DEFAULT_ENTITY(new_unit);
				new_unit->current_scale = MIN(1.0f, memory->delta_time);
				new_unit->selectable = true;
				new_unit->type = ENTITY_UNIT;
				new_unit->unit_type = UNIT_TURRET;

				new_unit->health = 2;

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				new_unit->shooting_cooldown = 0.9f;
				new_unit->shooting_cd_time_left = new_unit->shooting_cooldown;
				new_unit->rotation.y = 0;
				new_unit->mesh_uid = memory->meshes.icosphere_uid;
				new_unit->tex_uid = memory->textures.white_tex_uid;
				new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
			}
		}
	} else if ( memory->creating_unit == 2){
		if(input->cursor_primary == -1){
			// CREATING UNIT
			if(memory->teams_resources[entities[memory->player_uid].team_uid] > 0){
				memory->teams_resources[entities[memory->player_uid].team_uid] -= 1;
				u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
				Entity* new_unit = &entities[new_entity_index];
				DEFAULT_ENTITY(new_unit);
				new_unit->current_scale = MIN(1.0f, memory->delta_time);
				new_unit->selectable = true;
				new_unit->type = ENTITY_UNIT;
				new_unit->unit_type = UNIT_SPAWNER;

				new_unit->health = 2;

				new_unit->pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = entities[memory->player_uid].team_uid;
				new_unit->shooting_cooldown = 5.0f;
				new_unit->shooting_cd_time_left = new_unit->shooting_cooldown;
				new_unit->rotation.y = 0;
				new_unit->mesh_uid = memory->meshes.test_orientation_uid;
				new_unit->tex_uid = memory->textures.white_tex_uid;
				new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
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
		selected_entity->object3d.color = {0,1,0,1};

		if( memory->selected_uid != memory->player_uid ){
			if( input->cursor_secondary > 0)
				selected_entity->target_pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
			else if( input->cursor_primary > 0)
				memory->selected_uid = memory->player_uid;
			else if( input->move > 0)
				selected_entity->target_move_pos = {cursor_world_pos.x, 0, cursor_world_pos.z};
		}
	}	
	// memory->spawn_timer -= memory->delta_time;
	if(memory->spawn_timer < 0){
		memory->spawn_timer += 5.0f;

		u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
		Entity* enemy = &entities[new_entity_index];
		DEFAULT_ENTITY(enemy);
		enemy->current_scale = MIN(1.0f, memory->delta_time);
		enemy->mesh_uid = memory->meshes.test_orientation_uid;
		enemy->tex_uid = memory->textures.white_tex_uid;
		enemy->shooting_cooldown = 1.1f;

		enemy->health = 5;
		enemy->target_pos = entities[memory->player_uid].pos;
		enemy->team_uid = 1; //TODO: put something that is not the player's team

		enemy->pos = {4,0,4};
		enemy->target_move_pos = enemy->pos;
		enemy->color = {1,0,0,1};
	}

	// UPDATING ENTITIES
	UNTIL(i, MAX_ENTITIES){
		// SHOOTING
		Entity* entity = &entities[i]; 
		if(!entity->visible) continue;

		if( !entity->active){
			entity->current_scale += memory->delta_time;
			if(entity->current_scale > 1.0f){
				entity->current_scale = 1.0f;
				entity->active = true;
			}
		} else if ( entity->type == ENTITY_UNIT ) {
			// IF IT IS A TURRET
			entity->shooting_cd_time_left -= memory->delta_time;
			if(entity->shooting_cd_time_left < 0){
				entity->shooting_cd_time_left = entity->shooting_cooldown;

				if(entity->unit_type == UNIT_TURRET){

					u32 new_entity_index = next_inactive_entity(entities,&memory->last_inactive_entity);
					Entity* new_bullet = &entities[new_entity_index];
					DEFAULT_ENTITY(new_bullet);
					new_bullet->health = 1; //TODO: for now this is just so it doesn't disappear, CHANGE IT
					new_bullet->lifetime = 5.0f;

					new_bullet->current_scale = 1.0f;

					new_bullet->type = ENTITY_PROJECTILE;
					new_bullet->speed = 50;
					new_bullet->mesh_uid = memory->meshes.ball_uid;
					new_bullet->tex_uid = memory->textures.white_tex_uid;
					new_bullet->color = {0.6f,0.6f,0.6f,1};
					Entity* parent = &entities[i];
					new_bullet->team_uid = parent->team_uid;
					new_bullet->pos = parent->object3d.pos;
					// TODO: go in the direction that parent is looking (the parent's rotation);
					new_bullet->target_pos = parent->looking_at;
					new_bullet->scale = {0.4f,0.4f,0.4f};
					V3 target_direction = v3_difference(new_bullet->target_pos, new_bullet->object3d.pos);
					new_bullet->velocity =  new_bullet->speed * v3_normalize(target_direction);
				} else if( entity->type == UNIT_SPAWNER){
					u32 new_entity_index = next_inactive_entity(entities, &memory->last_inactive_entity);
					Entity* new_unit = &entities[new_entity_index];
					DEFAULT_ENTITY(new_unit);

					new_unit->current_scale = MIN(1.0f, memory->delta_time);
					new_unit->selectable = true;
					new_unit->type = ENTITY_UNIT;
					new_unit->unit_type = UNIT_TURRET;

					new_unit->health = 2;

					new_unit->pos = entity->target_pos;
					new_unit->target_move_pos = new_unit->pos;

					new_unit->team_uid = entity->team_uid;
					new_unit->shooting_cooldown = 0.9f;
					new_unit->rotation.y = 0;
					new_unit->mesh_uid = memory->meshes.icosphere_uid;
					new_unit->tex_uid = memory->textures.white_tex_uid;
					new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
				}
			}
		}{// DYNAMICS / COLLISIONS
			entity->pos.y = 0;//TODO: clamping height position
			if(i == memory->player_uid){
				V3 move_v = (entity->target_move_pos - entity->pos);
				V3 accel = 10*(move_v - entity->velocity);
				entity->velocity = entity->velocity + (memory->delta_time * accel);
				if(entity->velocity.x || entity->velocity.z)
					entity->rotation.y = v2_angle({entity->velocity.x, entity->velocity.z}) + PI32/2;
					
				UNTIL(j, MAX_ENTITIES){
					if(i!=j)
						test_collision(&entities[i], &entities[j], memory->delta_time);
				}
			}else if(entity->type != ENTITY_PROJECTILE){
				//TODO: make it dependent on the entity's speed so that not all entities move at the same speed
				V3 move_v = (entity->target_move_pos - entity->pos);
				V3 accel = 10*(move_v - (0.4f*entity->velocity));
				entity->velocity = entity->velocity +( memory->delta_time * accel );
				UNTIL(j, MAX_ENTITIES){
					if(i!=j)
						test_collision(&entities[i], &entities[j], memory->delta_time);
				}

				//TODO: when unit is moving and shooting, shoots seem to come from the body
				// and that's because it should spawn in the tip of the cannon instead of the center
				//LERPING TARGET POS
				entity->looking_at = entity->looking_at + (10*memory->delta_time * (entity->target_pos - entity->looking_at));
				V3 target_direction = entity->looking_at - entity->pos; 
				r32 target_rotation = v2_angle({target_direction.x, target_direction.z}) + PI32/2;
				entity->rotation.y = target_rotation;

				//LERPING ROTATION
				// V3 target_direction = entity->target_pos - entity->pos;
				// r32 target_rotation = v2_angle({target_direction.x, target_direction.z})+ PI32/2;

				// r32 angle_difference = target_rotation - entity->object3d.rotation.y;
				// if(angle_difference > TAU32/2)
				// 	entity->rotation.y += TAU32;
				// else if(angle_difference < -TAU32/2)
				// 	entity->rotation.y -= TAU32;
				// entity->rotation.y += 10*(target_rotation - entity->rotation.y) * memory->delta_time;
			}
		}{// PROJECTILES
			if(entity->lifetime){
				entity->lifetime -= memory->delta_time;
				if(entity->lifetime < 0){
					*entity = {0};
					memory->entity_generations[i]++;
				}
			}
			if(entity->type == ENTITY_PROJECTILE){
				UNTIL(i2, MAX_ENTITIES){
					Entity* entity2 = &entities[i2];
					if(entity2->visible && 
						entity->team_uid != entity2->team_uid
					){
						r32 intersect = sphere_vs_sphere(entity->pos, entity->scale.x, entity2->pos, entity2->scale.x);
						if(intersect > 0){
							entity2->health -= 1;
							//TODO: this check should be done always for just the entities that use the health property
							// but i don't know how will i know who destroyed that entity to give respective reward
							if(entity2->health <= 0){
								*entity2 = {0};
								memory->teams_resources[entity->team_uid] += 1;
							}
							*entity = {0}; 
							memory->entity_generations[i]++;
							break;
						}
					}
				}
			}
		}
		entity->pos = entity->pos + (memory->delta_time * entity->velocity);
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
		if(memory->entities[i].visible)
		{
			PUSH_BACK(render_list, memory->temp_arena, request);
			request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
			request->object3d = memory->entities[i].object3d;
			request->object3d.scale = memory->entities[i].current_scale * request->object3d.scale;
		}
	}

	PUSH_BACK(render_list, memory->temp_arena, request);
	request->type_flags = REQUEST_FLAG_SET_VS|REQUEST_FLAG_SET_PS|REQUEST_FLAG_SET_DEPTH_STENCIL;
	request->vshader_uid = memory->vshaders.ui_vshader_uid;
	request->pshader_uid = memory->pshaders.ui_pshader_uid;
	request->depth_stencil_uid = memory->depth_stencils.ui_depth_stencil_uid;

	printo_screen(memory, screen_size, render_list, string("here i will show the fps (probably): 69 fps"), {-1,1});

	{
		Object3d template_object;
		template_object.mesh_uid = memory->meshes.plane_mesh_uid;
		template_object.tex_uid = memory->textures.font_atlas_uid;
		template_object.scale = {5,5,5};
		template_object.color = {1,1,1,1};
		Renderer_request* request_1;
		PUSH_BACK(render_list, memory->temp_arena, request_1);
		request_1->type_flags = REQUEST_FLAG_RENDER_IMAGE;
		request_1->object3d = template_object;
		request_1->object3d.tex_uid.rect_uid = '0'-FIRST_CHAR;
		request_1->object3d.pos = {-0.2f, -0.8f, 0};

		Renderer_request* request_2;
		PUSH_BACK(render_list, memory->temp_arena, request_2);
		request_2->type_flags = REQUEST_FLAG_RENDER_IMAGE;
		request_2->object3d = template_object;
		request_2->object3d.tex_uid.rect_uid = '1'-FIRST_CHAR;
		request_2->object3d.pos = {0, -0.8f, 0};
		
		Renderer_request* request_3;
		PUSH_BACK(render_list, memory->temp_arena, request_3);
		request_3->type_flags = REQUEST_FLAG_RENDER_IMAGE;
		request_3->object3d = template_object;
		request_3->object3d.tex_uid.rect_uid = '2'-FIRST_CHAR;
		request_3->object3d.pos = {0.2f, -0.8f, 0};
	}


	// draw(render_list, memory->temp_arena, &test_plane);
}

void init(App_memory* memory, Init_data* init_data){
	memory->entities = ARENA_PUSH_STRUCTS(memory->permanent_arena, Entity, MAX_ENTITIES);
	memory->entity_generations = ARENA_PUSH_STRUCTS(memory->permanent_arena, u32, MAX_ENTITIES);

	memory->camera_rotation.x = PI32/2;
	memory->camera_pos.y = 16.0f;


	memory->entities[memory->player_uid].health = 1;
	memory->entities[memory->player_uid].team_uid = 0;
	memory->teams_resources[memory->entities[memory->player_uid].team_uid] = 200;

	{
		Vertex_shader_from_file_request vs_request = {0};
		vs_request.p_uid = &memory->vshaders.default_vshader_uid;
		vs_request.filename = string("x:/source/code/shaders/3d_shaders.hlsl");
		vs_request.ie_count = 3;
		vs_request.ie_names = ARENA_PUSH_STRUCTS(memory->temp_arena, String, vs_request.ie_count);
		
		vs_request.ie_names[0] = string("POSITION");
		vs_request.ie_names[1] = string("TEXCOORD");
		vs_request.ie_names[2] = string("NORMAL");


		vs_request.ie_sizes = ARENA_PUSH_STRUCTS(memory->temp_arena, u32, vs_request.ie_count);
		vs_request.ie_sizes[0] = sizeof(float)*3;
		vs_request.ie_sizes[1] = sizeof(float)*2;
		vs_request.ie_sizes[2] = sizeof(float)*3;
		
		push_vertex_shader_from_file_request(memory, init_data, vs_request);

		push_pixel_shader_from_file_request(memory, init_data, 
			&memory->pshaders.default_pshader_uid, vs_request.filename
		);

		push_create_blend_state_request(memory, init_data, 
			&memory->blend_states.default_blend_state_uid, true
		);

		push_create_depth_stencil_request(memory, init_data,
			&memory->depth_stencils.default_depth_stencil_uid, true
		);
	}

	{
		Vertex_shader_from_file_request vs_request = {0};
		vs_request.p_uid = &memory->vshaders.ui_vshader_uid;
		vs_request.filename = string("x:/source/code/shaders/ui_shaders.hlsl");
		vs_request.ie_count = 2;
		vs_request.ie_names = ARENA_PUSH_STRUCTS(memory->temp_arena, String, vs_request.ie_count);

		vs_request.ie_names[0] = string("POSITION");
		vs_request.ie_names[1] = string("TEXCOORD");

		vs_request.ie_sizes = ARENA_PUSH_STRUCTS(memory->temp_arena, u32, vs_request.ie_count);
		vs_request.ie_sizes[0] = sizeof(float)*3;
		vs_request.ie_sizes[1] = sizeof(float)*2;
		
		push_vertex_shader_from_file_request(memory, init_data, vs_request);

		push_pixel_shader_from_file_request(
			memory, init_data, &memory->pshaders.ui_pshader_uid, vs_request.filename
		);

		push_create_depth_stencil_request(memory, init_data,
			&memory->depth_stencils.ui_depth_stencil_uid, true
		);
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
	push_load_font_request(memory, init_data, &memory->textures.font_atlas_uid, string("x:/source/fonts/Inconsolata-Regular.ttf"));

	u32 default_tex_pixels[] = {
		0x7f000000, 0xffff0000,
		0xff00ff00, 0xff0000ff,
	};
	push_tex_from_surface_request(memory, init_data, &memory->textures.default_tex_uid, 2,2, default_tex_pixels);

	u32 white_tex_pixels[] = {0xffffffff};
	push_tex_from_surface_request(memory, init_data, &memory->textures.white_tex_uid, 1, 1, white_tex_pixels);

	push_tex_from_file_request(memory, init_data, &memory->textures.ogre_tex_uid, string("data/ogre_color.png"));

	Vertex3d triangle_vertices [3] = {
		{{0, 1, 0},{0.5, 0.0}},
		{{1, 0, 0},{1, 1}},
		{{-1, 0, 0},{0, 1}}
	};
	u16 triangle_indices[3] = {
		0,1,2
	};
	
	Mesh_primitive* triangle_primitives = save_primitives(
		memory->temp_arena, 
		triangle_vertices, sizeof(triangle_vertices[0]), ARRAYCOUNT(triangle_vertices),
		triangle_indices, ARRAYCOUNT(triangle_indices)
	);
	push_mesh_from_primitives_request(memory, init_data, &memory->meshes.triangle_mesh_uid, triangle_primitives);
	
	Vertex3d centered_plane_vertices[] =
	{
		{ { -1.0f, +1.0f, 0.0f}, { 0.0f, 0.0f }},
		{ { +1.0f, +1.0f, 0.0f}, { 1.0f, 0.0f }},
		{ { -1.0f, -1.0f, 0.0f}, { 0.0f, 1.0f }},
		{ { +1.0f, -1.0f, 0.0f}, { 1.0f, 1.0f }}
	};
	Vertex3d plane_vertices[] = 
	{
		{ {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f} },
		{ {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} },
		{ {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} },
		{ {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f} },
	};
	u16 plane_indices[] =
	{
		0,1,2,
		1,3,2
	};
	Mesh_primitive* plane_primitives = save_primitives(
		memory->temp_arena,
		plane_vertices, sizeof(plane_vertices[0]), ARRAYCOUNT(plane_vertices),
		plane_indices, ARRAYCOUNT(plane_indices)
	);
	push_mesh_from_primitives_request(memory, init_data,&memory->meshes.plane_mesh_uid,plane_primitives);

	Vertex3d test_vertices[8] =
	{
		{{-0.5, 0, 1},{},{-1,0,1}},
		{{0.5, 0, 1},{},{1,0,1}},
		{{-0.1f, 1, 1},{},{0,1,0}},
		{{2, 0.5, 0.5},{},{1,0.5f,0.5f}},
		{{0.1f, 0.5, -2},{},{0,0.1f,-1}},
		{{0,3,-3}, {}, {0,1,0}},
		{{3,-1,-3}, {}, {1,-0.1f,0}},
		{{-3,-1,-3}, {}, {-1,-0.1f,0}}
	};
	u16 test_indices[] = 
	{
		2,4,0,
		4,1,0,
		4,3,1,
		4,2,3,
		1,2,0,
		1,3,2,
		5,6,7
	};

	Mesh_primitive* test_orientation_primitives = save_primitives(
		memory->temp_arena,
		test_vertices, sizeof(test_vertices[0]), ARRAYCOUNT(test_vertices),
		test_indices, ARRAYCOUNT(test_indices)
	);
	push_mesh_from_primitives_request(memory,init_data,&memory->meshes.test_orientation2_uid, test_orientation_primitives);

	push_mesh_from_file_request(memory, init_data,&memory->meshes.ogre_mesh_uid, string("data/ogre.glb"));

	push_mesh_from_file_request(memory, init_data, &memory->meshes.female_mesh_uid, string("data/female.glb"));

	push_mesh_from_file_request(memory, init_data,&memory->meshes.turret_mesh_uid, string("data/turret.glb"));

	push_mesh_from_file_request(memory, init_data,&memory->meshes.test_orientation_uid, string("data/new_test_orientation.glb"));
	
	push_mesh_from_file_request(memory, init_data,&memory->meshes.ball_uid, string("data/ball.glb"));

	push_mesh_from_file_request(memory, init_data,&memory->meshes.icosphere_uid, string("data/icosphere.glb"));
}