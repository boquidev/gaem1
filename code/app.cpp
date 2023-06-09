#include "app.h"

void update(App_memory* memory)
{
	User_input* input = memory->input;

	r32 delta_time = 1;
	r32 camera_speed = 0.01f;
	r32 movement_speed = 0.01f;
	r32 sensitivity = 1.0f;

	memory->camera_rotation.y += sensitivity*(r32)input->cursor_speed.x / 500;
	memory->camera_rotation.x += sensitivity*(r32)input->cursor_speed.y / 500;
	memory->camera_rotation.x = CLAMP(-PI32/2, memory->camera_rotation.x, PI32/2);
	// memory->camera_rotation.x = PI32/2;

	V2 input_vector = {(r32)(input->right - input->left),(r32)(input->forward - input->backward)};
	input_vector = normalize(input_vector);
	{
		// V2 looking_direction = {cosf(memory->camera_rotation.y), sinf(memory->camera_rotation.y)};
		// V2 move_direction = {
		// 	input_vector.x*looking_direction.x + input_vector.y*looking_direction.y ,
		// 	-input_vector.x*looking_direction.y + input_vector.y*looking_direction.x
		// };
		// memory->camera_pos.x += move_direction.x * delta_time * camera_speed;
		// memory->camera_pos.z += move_direction.y * delta_time * camera_speed;

		// memory->camera_pos.y += (input->up - input->down) * delta_time * camera_speed;
		Object3d* player = &memory->entities[memory->player_uid];
		player->pos.x += input_vector.x *  movement_speed * delta_time;
		player->pos.z += input_vector.y * movement_speed * delta_time;
		if(input_vector.x || input_vector.y)
			player->rotation.y = v2_angle(input_vector) -PI32/2;
	}

	Object3d* turret = &memory->entities[1];
	turret->visible = true;
	turret->scale = {0.3f,0.3f,0.3f};
	turret->rotation.y = 0;
	turret->color = {1,1,1,1};
	turret->p_mesh_uid = memory->meshes.p_turret_mesh_uid;
	turret->p_tex_uid = memory->textures.p_white_tex_uid;
	turret->pos = {0,-1,0};

	Object3d* test_orientation = &memory->entities[2];
	test_orientation->visible = true;
	test_orientation->scale = {0.3f, 0.3f, 0.3f};
	test_orientation->color = {0.5,0.5,0.5,1};
	test_orientation->p_mesh_uid = memory->meshes.p_test_orientation2_uid;
	test_orientation->p_tex_uid = memory->textures.p_white_tex_uid;
	test_orientation->pos = {-1, -1, 1};
}

void render(App_memory* memory, Int2 screen_size, List* render_list)
{

	until(i, MAX_ENTITIES)
	{
		if(memory->entities[i].visible)
		{
			Object3d* render_object = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
			*render_object = memory->entities[i];
		}

	}
	
}

void init(App_memory* memory, Init_data* init_data)
{
	
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


	memory->player_uid = 0;
	memory->entities[memory->player_uid] = 
		{memory->meshes.p_ogre_mesh_uid,memory->textures.p_white_tex_uid, 
		{0.4f,0.4f,0.4f}, {}, {}, {1,1,1,1}, true};


}