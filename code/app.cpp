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

	Object3d* triangle = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
	triangle->mesh_uid = *memory->meshes.triangle_mesh_uid;

	Object3d* ogre = LIST_PUSH_BACK_STRUCT(render_list, Object3d, memory->temp_arena);
	ogre->mesh_uid = *memory->meshes.ogre_mesh_uid;
	until(y, ARRAYCOUNT(memory->tilemap))
	{
		until(x, ARRAYCOUNT(memory->tilemap[y]))
		{

			//draw_rectangle(x, y, 10,10, tilemap[y][x]);
			// or maybe
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
	
	// DEFAULT TEXTURE
	// u32 test_tex[] = {
	// 	0x00000000, 0xffff0000,
	// 	0xff00ff00, 0xff0000ff,
	// };
	u32 white_tex_pixels[] = {
		0x00000000, 0xffff0000,
		0xff00ff00, 0xff0000ff,
	};
	Surface white_texture = {2, 2, &white_tex_pixels};
	dx11_create_texture_view(dx, &white_texture, &pipeline_3d.default_texture_view);

	// TEST LOADING A MODEL

	File_data ogre_file = win_read_file(string("data/ogre.glb"), temp_arena);
	GLB glb = {0};
	glb_get_chunks(ogre_file.data, 
		&glb);
	{ // THIS IS JUST FOR READABILITY OF THE JSON FILE
		void* formated_json = arena_push_size(temp_arena,MEGABYTES(4));
		u32 new_size = format_json_more_readable(glb.json_chunk, glb.json_size, formated_json);
		win_write_file(string("data/ogre.json"), formated_json, new_size);
		arena_pop_size(temp_arena, MEGABYTES(4));
	}
	u32 meshes_count = 0;

	Gltf_mesh* meshes = gltf_get_meshes(&glb, temp_arena, &meshes_count);

	Mesh_primitive* primitives = ARENA_PUSH_STRUCTS(permanent_arena, Mesh_primitive, meshes_count);
	for(u32 m=0; m<meshes_count; m++)
	{
		u32 primitives_count = meshes[m].primitives_count;
		Gltf_primitive* Mesh_primitive = meshes[m].primitives;
		//TODO: here i am assuming this mesh has only one primitive
		primitives[m] = gltf_get_mesh_primitives(permanent_arena, &Mesh_primitive[0]);
		// for(u32 p=0; p<primitives_count; p++)
		// {	
		// }
	}
	Dx_mesh ogre_mesh = dx11_init_mesh(dx, 
	primitives[0].vertices, primitives[0].vertices_count, primitives[0].vertex_size,
	primitives[0].indices, primitives[0].indices_count,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Object3d* ogre = LIST_PUSH_BACK_STRUCT(&pipeline_3d.list, Object3d, permanent_arena);
	*ogre = object3d(&ogre_mesh);
	// Object3d* ogre2 =  LIST_PUSH_BACK_STRUCT(&pipeline_3d.list, Object3d, permanent_arena);
	// *ogre2 = object3d(&ogre_mesh);
*/

	Vertex3d triangle_vertices [3] = {
		{{0, 1, 0},{0.5, 0.0}},
		{{1, 0, 0},{1, 1}},
		{{-1, 0, 0},{0, 1}}
	};
	u16 triangle_indices[3] = {
		0,1,2
	};
	Mesh_primitive triangle_primitives = {
		triangle_vertices,
		sizeof(Vertex3d),
		ARRAYCOUNT(triangle_vertices),
		triangle_indices,
		ARRAYCOUNT(triangle_indices),
	};

	u32* triangle_mesh_uid = LIST_PUSH_BACK_STRUCT(&init_data->meshes_uid_list, u32, memory->permanent_arena);
	memory->meshes.triangle_mesh_uid = triangle_mesh_uid;

	u32* ogre_mesh_uid = LIST_PUSH_BACK_STRUCT(&init_data->meshes_uid_list,u32, memory->permanent_arena);
	memory->meshes.ogre_mesh_uid = ogre_mesh_uid;



/*
	Dx_mesh triangle_mesh = dx11_init_mesh(dx, 
		triangle_vertices, 3, sizeof(Vertex3d), 
		triangle_indices, 3, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Vertex3d test_plane_vertices[] =
	{
		{ { -1.0f, +1.0f, 0.0f}, { 0.0f, 0.0f }},
		{ { +1.0f, +1.0f, 0.0f}, { 1.0f, 0.0f }},
		{ { -1.0f, -1.0f, 0.0f}, { 0.0f, 1.0f }},
		{ { +1.0f, -1.0f, 0.0f}, { 1.0f, 1.0f }}
	};
	u16 test_plane_indices[] =
	{
		0,1,2,
		1,3,2
	};
	Dx_mesh plane_mesh = dx11_init_mesh(dx,
		test_plane_vertices, ARRAYCOUNT(test_plane_vertices), sizeof(Vertex3d),
		test_plane_indices, ARRAYCOUNT(test_plane_indices), 
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	
*/
}