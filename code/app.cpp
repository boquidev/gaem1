#include "app.h"

void update(App_memory* memory){
	User_input* input = memory->input;
	User_input* holding_inputs = memory->holding_inputs;

	r32 delta_time = 1;
	r32 camera_speed = 1.0f;
	r32 sensitivity = 1.0f;

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

		Entity* ogre = &memory->entities[memory->player_uid];
		ogre->visible = true;
		ogre->team_uid = 0;
		ogre->speed = 10.0f;
		ogre->p_mesh_uid = memory->meshes.p_ogre_mesh_uid;
		ogre->p_tex_uid = memory->textures.p_white_tex_uid;
		ogre->scale = {1.0f,1.0f,1.0f};
		ogre->color = {1,1,1,1};
		// ogre->pos.x += input_vector.x *  ogre->speed * delta_time;
		// ogre->pos.z += input_vector.y * ogre->speed * delta_time;
		ogre->target_move_pos = v3_addition(ogre->pos, {input_vector.x*ogre->speed, 0, input_vector.y*ogre->speed});
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
	until(i, MAX_ENTITIES){
		if(memory->entities[i].visible && 
			memory->entities[i].selectable &&
			memory->entities[i].team_uid == memory->entities[memory->player_uid].team_uid &&
			i != memory->player_uid 
		){
			Entity* entity = &memory->entities[i];
			entity->radius = 1.0f;
			entity->color = {1,1,1,1};

			r32 intersected_t = 0;
			if(line_vs_sphere(cursor_world_pos, z_direction, entity->object3d.pos, entity->radius, &intersected_t)){
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
	Entity* highlighted_entity = &memory->entities[memory->highlighted_uid];
	highlighted_entity->object3d.color = {1,1,0,1};	
	if(input->L == 1)
		memory->creating_unit = !memory->creating_unit;
	if(memory->creating_unit) {// SELECTED UNIT TO CREATE
		if(input->cursor_primary == 1){//TODO: do it when releasing the button
			// CREATING UNIT
			if(memory->teams_resources[memory->entities[memory->player_uid].team_uid] > 0)
			{
				memory->teams_resources[memory->entities[memory->player_uid].team_uid] -= 1;
				u32 new_entity_index = next_inactive_entity(memory->entities, &memory->last_inactive_entity);
				Entity* new_unit = &memory->entities[new_entity_index];
				new_unit->visible = true;
				new_unit->selectable = true;

				new_unit->health = 2;

				new_unit->pos = cursor_world_pos;
				new_unit->target_move_pos = new_unit->pos;

				new_unit->team_uid = memory->entities[memory->player_uid].team_uid;
				new_unit->shooting_cooldown = 0.9f;
				new_unit->scale = {1.0f, 1.0f, 1.0f};
				new_unit->rotation.y = 0;
				new_unit->color = {1,1,1,1};
				new_unit->p_mesh_uid = memory->meshes.p_turret_mesh_uid;
				new_unit->p_tex_uid = memory->textures.p_white_tex_uid;
				new_unit->target_pos = v3_addition(new_unit->pos, {0,0, 10.0f});
			}
		}
	} else {  // NO SELECTED UNIT TO CREATE
		if(input->cursor_primary == 1)
			memory->clicked_uid = memory->highlighted_uid;
		else if( !input->cursor_primary ){//TODO: do it when releasing the button
			if(memory->clicked_uid){
				if(memory->highlighted_uid == memory->clicked_uid){
					memory->selected_uid = memory->clicked_uid;
				}
				memory->clicked_uid = 0;
			}
		}

		Entity* selected_entity = &memory->entities[memory->selected_uid];
		selected_entity->object3d.color = {0,1,0,1};

		if( memory->selected_uid != memory->player_uid ){
			if( input->cursor_secondary)
				selected_entity->target_pos = cursor_world_pos;
			else if( input->cursor_primary )
				memory->selected_uid = memory->player_uid;
			else if( input->move)
				selected_entity->target_move_pos = cursor_world_pos;
		}
	}	
	memory->spawn_timer -= memory->delta_time;
	if(memory->spawn_timer < 0){
		memory->spawn_timer += 5.0f;

		u32 new_entity_index = next_inactive_entity(memory->entities, &memory->last_inactive_entity);
		Entity* enemy = &memory->entities[new_entity_index];
		enemy->visible = true;
		enemy->p_mesh_uid = memory->meshes.p_test_orientation_uid;
		enemy->p_tex_uid = memory->textures.p_white_tex_uid;
		enemy->shooting_cooldown = 1.1f;

		enemy->health = 5;
		enemy->target_pos = memory->entities[memory->player_uid].pos;
		enemy->team_uid = 1; //TODO: put something that is not the player's team

		enemy->pos = {0,0,2};
		enemy->target_move_pos = enemy->pos;
		enemy->scale = {1,1,1};
		enemy->color = {1,0,0,1};
	}

	// UPDATING ENTITIES
	until(i, MAX_ENTITIES){
		// SHOOTING
		Entity* entity = &memory->entities[i]; 
		if( entity->visible && !entity->is_projectile && i!=memory->player_uid) {
			// IF IT IS A TURRET
			entity->shooting_cd_time_left -= memory->delta_time;
			if(entity->shooting_cd_time_left < 0){
				entity->shooting_cd_time_left = entity->shooting_cooldown;

				u32 new_entity_index = next_inactive_entity(memory->entities,&memory->last_inactive_entity);
				Entity* new_bullet = &memory->entities[new_entity_index];
				new_bullet->health = 1; //TODO: for now this is just so it doesn't disappear, CHANGE IT
				new_bullet->lifetime = 10.0f;
				new_bullet->visible = 1;
				new_bullet->is_projectile = 1;
				new_bullet->speed = 50;
				new_bullet->object3d.p_mesh_uid = memory->meshes.p_ball_uid;
				new_bullet->object3d.p_tex_uid = memory->textures.p_white_tex_uid;
				new_bullet->object3d.color = {0.8f,0,0,1};
				Entity* parent = &memory->entities[i];
				new_bullet->team_uid = parent->team_uid;
				new_bullet->object3d.pos = parent->object3d.pos;
				// TODO: go in the direction that parent is looking;
				new_bullet->target_pos = parent->looking_at;
				new_bullet->object3d.scale = {0.4f,0.4f,0.4f};
				V3 target_direction = v3_difference(new_bullet->target_pos, new_bullet->object3d.pos);
				new_bullet->velocity =  new_bullet->speed * v3_normalize(target_direction);
			}
		}
		// DYNAMICS
		if(entity->visible){
			if(i == memory->player_uid)
			{
				V3 move_v = (entity->target_move_pos - entity->pos);
				V3 accel = 10*(move_v - entity->velocity);
				entity->velocity = entity->velocity + (memory->delta_time * accel);
				if(entity->velocity.x || entity->velocity.z)
					entity->rotation.y = v2_angle({entity->velocity.x, entity->velocity.z}) + PI32/2;
			}else if(!entity->is_projectile){
				V3 move_v = (entity->target_move_pos - entity->pos);
				V3 accel = 10*(move_v - (0.4f*entity->velocity));
				entity->velocity = entity->velocity +( memory->delta_time * accel );

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
			}else{
				if( entity->lifetime < 0 )
				{
					*entity = {0};
					memory->entity_generations[i]++;
				}else{
					entity->lifetime -= memory->delta_time;
					until(i2, MAX_ENTITIES){
						Entity* entity2 = &memory->entities[i2];
						if(entity2->visible && 
							entity->team_uid != entity2->team_uid
						){
							r32 intersect = sphere_vs_sphere(entity->pos, entity->scale.x, entity2->pos, entity2->scale.x);
							if(intersect > 0){
								entity2->health -= 1;
								//TODO: this check should be done always for just the entities that use the health property
								if(entity2->health <= 0){
									entity2->visible = false;
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
		}
		
		entity->object3d.pos = entity->object3d.pos + (memory->delta_time * entity->velocity);

	}
}

void render(App_memory* memory, Int2 screen_size, List* render_list){
	until(i, MAX_ENTITIES)
	{
		if(memory->entities[i].visible)
		{
			Object3d* render_object = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
			*render_object = memory->entities[i].object3d;
		}
	}
}

void init(App_memory* memory, Init_data* init_data){
	memory->camera_rotation.x = PI32/2;
	memory->camera_pos.y = 15.0f;

	
	memory->entities[memory->player_uid].health = 1;
	memory->entities[memory->player_uid].team_uid = 0;
	memory->teams_resources[memory->entities[memory->player_uid].team_uid] = 2;
	
	
/*
	// GETTING COMPILED SHADERS
	// 3D SHADERS
		// VERTEX SHADER	
	String shaders_3d_filename = string("x:/source/code/shaders/3d_shaders.hlsl");
	//TODO: remember the last write time of the file when doing runtime compiling

	File_data shaders_3d_compiled_vs = dx11_get_compiled_shader(shaders_3d_filename, temp_arena, "vs", VS_PROFILE);
	
	dx11_create_vs(dx, shaders_3d_compiled_vs, &pipeline_3d.vs);
	D3D11_INPUT_ELEMENT_DESC ied[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	};
	
	dx11_create_input_layout(dx, shaders_3d_compiled_vs, ied, ARRAYCOUNT(ied), &pipeline_3d.input_layout);
	
		// PIXEL SHADER
	File_data shaders_3d_compiled_ps = dx11_get_compiled_shader(shaders_3d_filename, temp_arena, "ps", PS_PROFILE);

	dx11_create_ps(dx, shaders_3d_compiled_ps, &pipeline_3d.ps);

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
	dx11_create_texture_view(dx, &white_texture, &pipeline_3d.default_texture_view);

*/
	u32 default_tex_pixels[] = {
		0x7f000000, 0xffff0000,
		0xff00ff00, 0xff0000ff,
	};
	memory->textures.p_default_tex_uid = push_tex_from_surface_request(memory, init_data, 2,2, default_tex_pixels);

	u32 white_tex_pixels[] = {0xffffffff};
	memory->textures.p_white_tex_uid = push_tex_from_surface_request(memory, init_data, 1, 1, white_tex_pixels);

	memory->textures.p_test_uid = push_tex_from_file_request(memory, init_data, string("data/test_atlas.png"));

	Vertex3d triangle_vertices [3] = {
		{{0, 1, 0},{0.5, 0.0}},
		{{1, 0, 0},{1, 1}},
		{{-1, 0, 0},{0, 1}}
	};
	u16 triangle_indices[3] = {
		0,1,2
	};
	
	Mesh_primitive* triangle_primitives = save_primitives(
		memory->permanent_arena, 
		triangle_vertices, sizeof(triangle_vertices[0]), ARRAYCOUNT(triangle_vertices),
		triangle_indices, ARRAYCOUNT(triangle_indices)
	);
	memory->meshes.p_triangle_mesh_uid = push_mesh_from_primitives_request(memory, init_data, triangle_primitives);
	
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
		memory->permanent_arena,
		plane_vertices, sizeof(plane_vertices[0]), ARRAYCOUNT(plane_vertices),
		plane_indices, ARRAYCOUNT(plane_indices)
	);
	memory->meshes.p_plane_mesh_uid = push_mesh_from_primitives_request(memory, init_data,plane_primitives);

	Vertex3d test_vertices[] =
	{
		{{-0.5, 0, 1}},
		{{0.5, 0, 1}},
		{{0, 1, 1}},
		{{2, 0.5, 0.5}},
		{{0, 0.5, -2}}
	};
	u16 test_indices[] = 
	{
		2,4,0,
		4,1,0,
		4,3,1,
		4,2,3,
		1,2,0,
		1,3,2
	};

	Mesh_primitive* test_orientation_primitives = save_primitives(
		memory->permanent_arena,
		test_vertices, sizeof(test_vertices[0]), ARRAYCOUNT(test_vertices),
		test_indices, ARRAYCOUNT(test_indices)
	);
	memory->meshes.p_test_orientation2_uid = push_mesh_from_primitives_request(memory,init_data, test_orientation_primitives);

	memory->meshes.p_ogre_mesh_uid = push_mesh_from_file_request(memory, init_data, string("data/ogre.glb"));

	memory->meshes.p_female_mesh_uid = push_mesh_from_file_request(memory, init_data, string("data/female.glb"));

	memory->meshes.p_turret_mesh_uid = push_mesh_from_file_request(memory, init_data, string("data/turret.glb"));

	memory->meshes.p_test_orientation_uid = push_mesh_from_file_request(memory, init_data, string("data/test_orientation.glb"));
	
	memory->meshes.p_ball_uid = push_mesh_from_file_request(memory, init_data, string("data/ball.glb"));
}