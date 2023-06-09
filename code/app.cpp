#include "app.h"

void update(App_memory* memory)
{
	u8 red = 0;
	u8 green = 0;
	u8 color_step = 255/ARRAYCOUNT(memory->tilemap);
	until(y, ARRAYCOUNT(memory->tilemap))
	{
		until(x, ARRAYCOUNT(memory->tilemap[y]))
		{
			memory->tilemap[y][x] = {red, green, 128, 255};
			red += color_step;
		}
		green += color_step;
	}
}

void render(App_memory* memory, Int2 screen_size, List* render_list)
{
	// Object3d* triangle = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
	// *triangle = {*memory->meshes.p_triangle_mesh_uid, {1,1,1}, {0,0,1}};

	// Object3d* ogre = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
	// *ogre = {*memory->meshes.p_ogre_mesh_uid,{1,1,1}, {0,0,1}};

	// Object3d* female = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
	// *female = {*memory->meshes.p_female_mesh_uid, {1,1,1}, {0,0,1}};


	until(y, ARRAYCOUNT(memory->tilemap))
	{
		until(x, ARRAYCOUNT(memory->tilemap[y]))
		{

			Object3d* plane = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
			*plane = {*memory->meshes.p_plane_mesh_uid, *memory->textures.p_test_uid,
			{(r32)ARRAYCOUNT(memory->tilemap[y])/screen_size.x, (r32)ARRAYCOUNT(memory->tilemap)/screen_size.y, 1}, 
			{(r32)x*ARRAYCOUNT(memory->tilemap[y])/screen_size.x,(r32)y*ARRAYCOUNT(memory->tilemap)/screen_size.y,  0.01f}, 
			{0,0,0}};
			//draw(rectangle_id, etc...); 
		}
	}
	Int2 rect_pos = memory->input->cursor_pos;
	// draw_rectangle(rect_pos.x, rect_pos.y, 15,15);
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
	
	// DEFAULT TEXTURE
	// u32 test_tex[] = {
	// 	0x00000000, 0xffff0000,
	// 	0xff00ff00, 0xff0000ff,
	// };
	u32 white_tex_pixels[] = {
		0x7f000000, 0xffff0000,
		0xff00ff00, 0xff0000ff,
	};
	Surface white_texture = {2, 2, &white_tex_pixels};
	memory->textures.p_default_tex_uid = push_tex_from_surface_request(memory, init_data, 2,2, white_tex_pixels);

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


	memory->meshes.p_ogre_mesh_uid = push_mesh_from_file_request(memory, init_data, string("data/ogre.glb"));

	memory->meshes.p_female_mesh_uid = push_mesh_from_file_request(memory, init_data, string("data/female.glb"));


	

}