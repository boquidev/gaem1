#include "pch.h"

#include "app.h"

#include "win_functions.h"

#include "d3d11_layer.h"

// STB LIBRARIES
//TODO: in the future use this just to convert image formats
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "libraries/stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "libraries/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "libraries/stb_truetype.h"
// STB END


HWND global_main_window = 0;
b32 global_running = 0;

struct App_dll
{
	b32 is_valid;

	update_type(update);
	render_type(render);
	init_type(init);
};


LRESULT CALLBACK
win_main_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch(message)
	{
		case WM_DESTROY:
			ASSERT(false);
		case WM_CLOSE:
			global_running = 0;
		break;
		case WM_ACTIVATE:
			// Check if the window is being activated or deactivated
			
			// OutputDebugString(bool_to_string((wparam != WA_INACTIVE)).text);
		break;
		case WM_MOUSEWHEEL:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
			//error unprocessed input messages
			ASSERT(false);
		break;
		
		default:
			result = DefWindowProc(window, message, wparam, lparam);
	}

	return result;
}

int WINAPI 
wWinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, PWSTR cmd_line, int cmd_show)
{
	Int2 win_size = {1000, 1000};

	// WINDOW CREATION
	WNDCLASSA window_class = {0};
	window_class.style = CS_VREDRAW|CS_HREDRAW;
	window_class.lpfnWndProc = win_main_window_proc;
	window_class.hInstance = h_instance;
	window_class.lpszClassName = "classname";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassA(&window_class);

	DWORD exstyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	
	global_main_window = CreateWindowExA(
		0,
		window_class.lpszClassName,
		"THE window",
		exstyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		win_size.x,
		win_size.y,
		0,
		0,
		window_class.hInstance,
		0
	);

	ASSERT(global_main_window);
	if(!global_main_window) return 1;

	// MAIN MEMORY BLOCKS
	Memory_arena arena1 = {0};
	arena1.size = MEGABYTES(256);
	arena1.data = (u8*)VirtualAlloc(0, arena1.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory_arena* permanent_arena = &arena1;
	
	Memory_arena arena2 = {0};
	arena2.size = MEGABYTES(256);
	arena2.data = (u8*)VirtualAlloc(0, arena2.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Memory_arena* temp_arena = &arena2;

	App_memory memory = {0};
	memory.temp_arena = temp_arena;
	memory.permanent_arena = permanent_arena;
	
	// GETTING CLIENT SIZES
	Int2 client_size = win_get_client_sizes(global_main_window);
	if(client_size.y) memory.aspect_ratio = (r32)client_size.x / (r32)client_size.y;





	// DIRECTX11
	// INITIALIZE DIRECT3D

	D3D d3d = {0};
	D3D* dx = &d3d;
	HRESULT hr;
	// CREATE DEVICE AND SWAPCHAIN
	{
		D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};

		D3D_FEATURE_LEVEL result_feature_level;
		u32 create_device_flags = DX11_CREATE_DEVICE_DEBUG_FLAG;

		hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flags, feature_levels, 
			ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &dx->device, &result_feature_level, &dx->context);
		ASSERTHR(hr);

	#if DEBUGMODE
		//for debug builds enable debug break on API errors
		ID3D11InfoQueue* info;
		dx->device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		info->Release();

		// hresult return values should not be checked anymore
		// cuz debugger will break on errors but i will check them anyway
	#endif

		IDXGIDevice* dxgi_device;
		hr = dx->device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
		ASSERTHR(hr);
		
		IDXGIAdapter* dxgi_adapter;
		hr = dxgi_device->GetAdapter(&dxgi_adapter);
		ASSERTHR(hr);

		IDXGIFactory2* factory;
		hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&factory));
		ASSERTHR(hr);
		
		DXGI_SWAP_CHAIN_DESC1 scd = {0};
		// default 0 value for width & height means to get it from window automatically
		//.Width = 0,
		//.Height = 0,
		scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scd.SampleDesc = {1, 0};
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.Scaling = DXGI_SCALING_NONE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		hr = factory->CreateSwapChainForHwnd(dx->device, global_main_window, &scd, 0, 0, &dx->swap_chain);
		ASSERTHR(hr);

		// disable Alt+Enter changing monitor resolution to match window size
		factory->MakeWindowAssociation(global_main_window, DXGI_MWA_NO_ALT_ENTER);

		dxgi_device->Release();
		dxgi_adapter->Release();
		factory->Release();
	}
	
	// LOADING APP DLL
	App_dll app = {0};
	{
		String dll_name = string("app.dll");
		HMODULE dll = LoadLibraryA(dll_name.text);
		ASSERT(dll);
		app.update = (update_type())GetProcAddress(dll, "update");
		app.render = (render_type())GetProcAddress(dll, "render");
		app.init = (init_type())GetProcAddress(dll, "init");
		ASSERT(app.update);
		ASSERT(app.render);
		ASSERT(app.init);
	}

	// APP INIT
	Init_data init_data = {0};
	app.init(&memory, &init_data);
	
	// CREATING TEXTURES
	LIST(Dx11_texture, textures_list) = {0};
	// fuck this is too long
	Tex_from_surface_request* tex_from_surface_request_node = init_data.tex_from_surface_requests[0];
	UNTIL(i, LIST_SIZE(init_data.tex_from_surface_requests))
	{
		Tex_from_surface_request* request = tex_from_surface_request_node;
		NEXT_ELEM(tex_from_surface_request_node);

		request->p_uid->uid = LIST_SIZE(textures_list);
		Dx11_texture* texture; PUSH_BACK(textures_list, memory.permanent_arena, texture);
		dx11_create_texture_view(dx, &request->surface, &texture->texview);
	}

	Tex_from_file_request* tex_from_file_request_node = init_data.tex_from_file_requests[0];
	UNTIL(i, LIST_SIZE(init_data.tex_from_file_requests)){
		Tex_from_file_request* request = tex_from_file_request_node;
		NEXT_ELEM(tex_from_file_request_node);

		int comp;
		Surface tex_surface = {0};
		tex_surface.data = stbi_load(request->filename.text, (int*)&tex_surface.width, (int*)&tex_surface.height, &comp, STBI_rgb_alpha);
		ASSERT(tex_surface.data);
		request->p_uid->uid = LIST_SIZE(textures_list);
		Dx11_texture* texture; PUSH_BACK(textures_list, permanent_arena, texture);
		dx11_create_texture_view(dx, &tex_surface, &texture->texview);
	}

	//  LOADING FONT
	r32 lines_height = 18; 

	Tex_from_file_request* load_font_request_node = init_data.load_font_requests[0];
	UNTIL(r, LIST_SIZE(init_data.load_font_requests)){
		Tex_from_file_request* request = load_font_request_node;
		NEXT_ELEM(load_font_request_node);

		// TESTING FONTS 
		File_data font_file = win_read_file(request->filename, temp_arena);
		//currently just supporting ANSI characters
		//TODO: learn how to do UNICODE

		// GETTING BITMAPS AND INFO
		request->p_uid->uid = LIST_SIZE(textures_list);
		request->p_uid->is_atlas = true;
		Dx11_texture* atlas_texture; PUSH_BACK(textures_list, permanent_arena, atlas_texture);
		atlas_texture->is_atlas = true;
		atlas_texture->texcount = CHARS_COUNT;

		stbtt_fontinfo font;
		Packed_tex_info* charinfo = memory.font_charinfo; //TODO: do i need this on the app layer? 
		Color32* charbitmaps [CHARS_COUNT];
		stbtt_InitFont(&font, (u8*)font_file.data,stbtt_GetFontOffsetForIndex((u8*)font_file.data, 0));
		UNTIL(c, LAST_CHAR-FIRST_CHAR){
			u32 codepoint = c+FIRST_CHAR;
			u8* monobitmap = stbtt_GetCodepointBitmap(
				&font, 0, stbtt_ScaleForPixelHeight(&font, lines_height), codepoint, 
				&charinfo[c].w, &charinfo[c].h, 
				&charinfo[c].xoffset, &charinfo[c].yoffset
			);
			u32 bitmap_size = charinfo[c].w * charinfo[c].h;
			charbitmaps[c] = ARENA_PUSH_STRUCTS(temp_arena, Color32, bitmap_size);
			Color32* bitmap =  charbitmaps[c];
			UNTIL(p, bitmap_size){
				bitmap[p].r = 255;
				bitmap[p].g = 255;
				bitmap[p].b = 255;
				bitmap[p].a = monobitmap[p];
			}
			stbtt_FreeBitmap(monobitmap,0);
		}
		// PACKING BITMAP RECTS

		//total_atlas_size = atlas_side*atlas_side
		s32 atlas_side = find_bigger_exponent_of_2((u32)(lines_height*(lines_height/2)*CHARS_COUNT));

		stbrp_context pack_context = {0};
		stbrp_node* pack_nodes = ARENA_PUSH_STRUCTS(temp_arena, stbrp_node, atlas_side);
		stbrp_init_target(&pack_context, atlas_side, atlas_side, pack_nodes, atlas_side);

		stbrp_rect* rects = ARENA_PUSH_STRUCTS(temp_arena, stbrp_rect, CHARS_COUNT);
		UNTIL(i, CHARS_COUNT){
			rects[i].w = charinfo[i].w;
			rects[i].h = charinfo[i].h;
		}
		stbrp_pack_rects(&pack_context, rects, CHARS_COUNT);

		// CREATE TEXTURE ATLAS AND COPY EACH CHARACTER BITMAP INTO IT USING THE POSITIONS OBTAINED FROM PACK
		Color32* atlas_pixels = ARENA_PUSH_STRUCTS(temp_arena, Color32, atlas_side*atlas_side);

		Int2 atlas_size = {atlas_side, atlas_side};

		atlas_texture->texrects = ARENA_PUSH_STRUCTS(permanent_arena, Rect, atlas_texture->texcount);
		atlas_texture->texoffsets = ARENA_PUSH_STRUCTS(permanent_arena, Int2, atlas_texture->texcount);

		UNTIL(i, CHARS_COUNT){
			ASSERT(rects[i].was_packed);
			if(rects[i].was_packed){
				atlas_texture->texrects[i].xf = (r32)rects[i].x / atlas_size.x;
				atlas_texture->texrects[i].yf = (r32)rects[i].y / atlas_size.y;
				atlas_texture->texrects[i].wf = (r32)charinfo[i].w / atlas_size.x;
				atlas_texture->texrects[i].hf = (r32)charinfo[i].h / atlas_size.y;
				atlas_texture->texoffsets[i].x = charinfo[i].xoffset;
				atlas_texture->texoffsets[i].y = charinfo[i].yoffset;

				// width and height had already been set before

				u32 first_char_pixel = (rects[i].y*atlas_side) + rects[i].x;
				Color32* charpixels = charbitmaps[i];
				UNTIL(y, (u32)rects[i].h){
					UNTIL(x, (u32)rects[i].w){
						u32 current_pixel = first_char_pixel + (y*atlas_side) + x;
						atlas_pixels[current_pixel] = charpixels[(y*rects[i].w) + x];
					}
				}
			}
		}
		Surface atlas_surface = {(u32)atlas_size.x, (u32)atlas_size.y, atlas_pixels};
		dx11_create_texture_view(dx, &atlas_surface, &atlas_texture->texview);
	}

	// SHADERS
	//TODO: remember the last write_time of the file when doing runtime compiling
	
	LIST(Vertex_shader, vertex_shaders_list) = {0};
	LIST(Dx11_pixel_shader*, pixel_shaders_list) = {0};
	{
		// VERTEX SHADERS	
		Vertex_shader_from_file_request* vs_ff_request_node = init_data.vs_ff_requests[0];
		UNTIL(i, LIST_SIZE(init_data.vs_ff_requests)){
			Vertex_shader_from_file_request* request = vs_ff_request_node;
			NEXT_ELEM(vs_ff_request_node);

			*request->p_uid = LIST_SIZE(vertex_shaders_list);
			Vertex_shader* vs; PUSH_BACK(vertex_shaders_list, permanent_arena, vs);
			File_data compiled_vs = dx11_get_compiled_shader(request->filename, temp_arena, "vs", VS_PROFILE);
			dx11_create_vs(dx, compiled_vs, &vs->shader);
			
			D3D11_INPUT_ELEMENT_DESC* ied = ARENA_PUSH_STRUCTS(temp_arena, D3D11_INPUT_ELEMENT_DESC, request->ie_count);
			u32 current_aligned_byte_offset = 0;
			UNTIL(j, request->ie_count){
				ied[j].SemanticName = request->ie_names[j].text;
				// ied[j].SemanticIndex // this is in case the element is bigger than a float4 (a matrix for example)
				ied[j].Format = input_element_format_from_size(request->ie_sizes[j]);
				// ied[j].InputSlot // this is for using secondary buffers (like an instance buffer)
				ied[j].AlignedByteOffset = current_aligned_byte_offset;
				current_aligned_byte_offset += request->ie_sizes[j]; // reset this value if using instance buffer
				// ied[j].InputSlotClass; // 0 PER_VERTEX_DATA vs 1 PER_INSTANCE_DATA
				// ied[j].InstanceDataStepRate; // the amount of instances to draw using the PER_INSTANCE data
			}
			dx11_create_input_layout(dx, compiled_vs, ied, request->ie_count, &vs->input_layout);
		}
		// PIXEL SHADERS
		From_file_request* ps_ff_request_node = init_data.ps_ff_requests[0];
		UNTIL(i, LIST_SIZE(init_data.ps_ff_requests)){
			From_file_request* request = ps_ff_request_node;
			NEXT_ELEM(ps_ff_request_node);

			*request->p_uid = LIST_SIZE(pixel_shaders_list);

			File_data compiled_ps = dx11_get_compiled_shader(request->filename, temp_arena, "ps", PS_PROFILE);
			Dx11_pixel_shader** ps; PUSH_BACK(pixel_shaders_list, permanent_arena, ps);
			dx11_create_ps(dx, compiled_ps, ps);
		}
			
	}
	
	// CREATING CONSTANT BUFFER
	// OBJECT TRANSFORM CONSTANT BUFFER
	D3D_constant_buffer object_buffer = {0};
	dx11_create_and_bind_constant_buffer(
		dx, &object_buffer, sizeof(XMMATRIX), OBJECT_BUFFER_REGISTER_INDEX, 0
	);
	// WORLD VIEW BUFFER
	XMMATRIX IDENTITY_MATRIX = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};
	D3D_constant_buffer view_buffer = {0};
	XMMATRIX view_matrix; // this will be updated in main loop
	dx11_create_and_bind_constant_buffer(
		dx, &view_buffer, sizeof(XMMATRIX), WORLD_VIEW_BUFFER_REGISTER_INDEX, 0
	);

	// WORLD PROJECTION BUFFER
	D3D_constant_buffer projection_buffer = {0};
	XMMATRIX projection_matrix; // this will be updated in main loop
	dx11_create_and_bind_constant_buffer(
		dx, &projection_buffer, sizeof(XMMATRIX), WORLD_PROJECTION_BUFFER_REGISTER_INDEX, 0
	);

	D3D_constant_buffer object_color_buffer = {0};
	Color white = {1.0f, 1.0f, 1.0f, 1.0f};
	dx11_create_and_bind_constant_buffer(
		dx, &object_color_buffer, sizeof(Color), OBJECT_COLOR_BUFFER_REGISTER_INDEX, &white
	);

	D3D_constant_buffer object_texrect_buffer = {0};
	Rect default_texrect = {0.0f, 0.0f, 1.0f, 1.0f};
	dx11_create_and_bind_constant_buffer(
		dx, &object_texrect_buffer, sizeof(Rect), OBJECT_TEXRECT_BUFFER_REGISTER_INDEX, &default_texrect
	);

	// MESHES
	//TODO: make meshes_list an array of size = sizeof(Meshes)/sizeof(u32*)
	// but i am not sure if doing that i will not be able to create new meshes at runtime
	// we'll see
	LIST(Dx_mesh, meshes_list) = {0};

	// LOADING MESHES FROM FILES
	From_file_request* mff_request_node = init_data.mesh_from_file_requests[0];
	UNTIL(i, LIST_SIZE(init_data.mesh_from_file_requests))
	{
		From_file_request* request = mff_request_node;
		NEXT_ELEM(mff_request_node);
		File_data glb_file = win_read_file(request->filename, temp_arena);
		GLB glb = {0};
		glb_get_chunks(glb_file.data, 
			&glb);
	#if DEBUGMODE
		{ // THIS IS JUST FOR READABILITY OF THE JSON CHUNK
			void* formated_json = arena_push_size(temp_arena,MEGABYTES(4));
			u32 new_size = format_json_more_readable(glb.json_chunk, glb.json_size, formated_json);
			win_write_file(concat_strings(request->filename, string(".json"), temp_arena), formated_json, new_size);
			arena_pop_back_size(temp_arena, MEGABYTES(4));
		}
	#endif
		u32 meshes_count = 0;
		Gltf_mesh* meshes = gltf_get_meshes(&glb, temp_arena, &meshes_count);
		
		Mesh_primitive* primitives = ARENA_PUSH_STRUCTS(permanent_arena, Mesh_primitive, meshes_count);
		for(u32 m=0; m<meshes_count; m++)
		{
			u32 primitives_count = meshes[m].primitives_count;
			Gltf_primitive* Mesh_primitive = meshes[m].primitives;
			//TODO: here i am assuming this mesh has only one primitive
			primitives[m] = gltf_primitives_to_mesh_primitives(permanent_arena, &Mesh_primitive[0]);
			// for(u32 p=0; p<primitives_count; p++)
			// {	
			// }
		}
		
		*request->p_uid = LIST_SIZE(meshes_list);

		Dx_mesh* current_mesh; PUSH_BACK(meshes_list, permanent_arena, current_mesh);
		*current_mesh = dx11_init_mesh(dx, 
		&primitives[0],
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
	}
	// CREATING MESHES FROM MANUALLY DEFINED PRIMITIVES
	Mesh_from_primitives_request* mfp_request_node = init_data.mesh_from_primitives_requests[0];
	UNTIL(i, LIST_SIZE(init_data.mesh_from_primitives_requests))
	{
		Mesh_from_primitives_request* request = mfp_request_node;
		NEXT_ELEM(mfp_request_node);

		*request->p_mesh_uid = LIST_SIZE(meshes_list);
		Dx_mesh* current_mesh; PUSH_BACK(meshes_list, permanent_arena, current_mesh);
		*current_mesh = dx11_init_mesh(dx, 
		request->primitives, 
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	// CREATING  D3D PIPELINES
	dx11_create_sampler(dx, &dx->sampler);
	dx11_create_rasterizer_state(dx, &dx->rasterizer_state, D3D11_FILL_SOLID, D3D11_CULL_BACK);

	// TODO: CONVERT THIS TWO LISTS INTO STATIC ARRAYS IF BEING DYNAMIC SERVES NO PURPOSE AT ALL
	LIST(Dx11_blend_state*, blend_states_list) = {0};
	Create_blend_state_request* bs_request_node = init_data.create_blend_state_requests[0];
	UNTIL(i, LIST_SIZE(init_data.create_blend_state_requests)){
		Create_blend_state_request* request = bs_request_node;
		NEXT_ELEM(bs_request_node);

		*request->p_uid = LIST_SIZE(blend_states_list);
		Dx11_blend_state** blend_state; PUSH_BACK(blend_states_list, memory.permanent_arena, blend_state);
		dx11_create_blend_state(dx, blend_state, request->enable_alpha_blending);
	}

	LIST(Depth_stencil, depth_stencils_list) = {0};
	Create_depth_stencil_request* ds_request_node = init_data.create_depth_stencil_requests[0];
	UNTIL(i, LIST_SIZE(init_data.create_depth_stencil_requests)){
		Create_depth_stencil_request* request = ds_request_node;
		NEXT_ELEM(ds_request_node);

		*request->p_uid = LIST_SIZE(depth_stencils_list);
		Depth_stencil* depth_stencil; PUSH_BACK(depth_stencils_list, memory.permanent_arena, depth_stencil);
		dx11_create_depth_stencil_state(dx, &depth_stencil->state, request->enable_depth);
	}

	// FRAME CAPPING SETUP
	UINT desired_scheduler_ms = 1;
	b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);

	LARGE_INTEGER pcf_result;
	QueryPerformanceFrequency(&pcf_result);
	s64 performance_counter_frequency = pcf_result.QuadPart;

	//TODO: maybe in the future use GetDeviceCaps() to get the monitor hz
	int monitor_refresh_hz = 200;
	
	memory.update_hz = (r32)monitor_refresh_hz;
	r32 target_seconds_per_frame = 1.0f / memory.update_hz;

#if DEBUGMODE
	u64 last_cycles_count = __rdtsc();
#endif
	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);
	
	// TODO: input backbuffer
	User_input input = {0};
	memory.input = &input;
	User_input holding_inputs = {0};
	memory.holding_inputs = &holding_inputs;

	r32 fov = 1;
	memory.fov = 32;
	bool perspective_on = false;
	memory.lock_mouse = false;
	Color bg_color = {0.2f, 0.2f, 0.2f, 1};
	// MAIN LOOP ____________________________________________________________
	
	global_running = 1;
	while(global_running)
	{
		arena_pop_back_size(temp_arena, temp_arena->used);

		// HANDLE WINDOW RESIZING
		Int2 current_client_size = win_get_client_sizes(global_main_window);
		if(!dx->render_target_view || client_size.x != current_client_size.x || client_size.y != current_client_size.y)
		{
			client_size = current_client_size;
			if(dx->render_target_view)
			{
				dx->render_target_view->Release();
				Depth_stencil* ds_node = depth_stencils_list[0];
				FOR_EACH(depth_stencils_list, i){
					Depth_stencil* current_ds = ds_node;
					NEXT_ELEM(ds_node);
					current_ds->view->Release();
				}
				dx->render_target_view = 0;
			}
			
			//TODO: be careful with 8k monitors
			if(client_size.x > 0 && client_size.y > 0 &&
				client_size.x < 4000 && client_size.y < 4000
			){
				ASSERT(client_size.x < 4000 && client_size.y < 4000);
				memory.aspect_ratio = (r32)client_size.x / (r32) client_size.y;
				hr = dx->swap_chain->ResizeBuffers(0, client_size.x, client_size.y, DXGI_FORMAT_UNKNOWN, 0);
				ASSERTHR(hr);

				dx11_create_render_target_view(dx, &dx->render_target_view);

				Depth_stencil* ds_node = depth_stencils_list[0];
				FOR_EACH(depth_stencils_list, i){
					Depth_stencil* current_ds = ds_node;
					NEXT_ELEM(ds_node);
					dx11_create_depth_stencil_view(dx, &current_ds->view, client_size.x, client_size.y);
				}
				
				dx11_set_viewport(dx, 0, 0, client_size.x, client_size.y);
			}
		}

		// MOUSE POSITION
		HWND foreground_window = GetForegroundWindow();
		memory.is_window_in_focus = (foreground_window == global_main_window);
		if(memory.is_window_in_focus)
		{
			POINT mousep;
			GetCursorPos(&mousep);
			ScreenToClient(global_main_window, &mousep);

			// TODO: THIS IS TEMPORAL
			RECT client_rect;
			GetClientRect(global_main_window, &client_rect); 

			Int2 client_center_pos = {
				client_rect.left + ((client_rect.right - client_rect.left)/2),
				client_rect.top + ((client_rect.bottom - client_rect.top)/2),
			};
			
			POINT center_point = { client_center_pos.x, client_center_pos.y };

			ClientToScreen(global_main_window, &center_point);

			input.cursor_speed = {0};
			if(memory.lock_mouse)
			{
				SetCursorPos(center_point.x, center_point.y);
				r32 px = (r32)(mousep.x-client_center_pos.x)/client_size.x;
				r32 py = -(r32)(mousep.y-client_center_pos.y)/client_size.y;
				input.cursor_speed.x = px - input.cursor_pos.x;
				input.cursor_speed.y = py - input.cursor_pos.y;
				input.cursor_pos = {0,0};
			}else
				input.cursor_pos = {
					(r32)(mousep.x - client_center_pos.x)/client_size.x, 
					-(r32)(mousep.y - client_center_pos.y)/client_size.y};
				
		}
		// HANDLING MESSAGES
		MSG message;
		while(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
		{
			switch(message.message)
			{
				case WM_DESTROY:
				case WM_CLOSE:
				case WM_QUIT:
					// THIS MESSAGES GO DIRECTLY INTO THE WINDOW PROCEDURE
				break;
				case WM_ACTIVATE:
				{
					// Check if the window is being activated or deactivated
					// BUT THIS MESSAGES ONLY GOES THROUGH THE WINDOW PROCEDURE SO FUCK
					// memory.is_window_in_focus = (message.wParam != WA_INACTIVE);
				}break;
				case WM_LBUTTONDBLCLK:
				break;
				case WM_LBUTTONDOWN:// just when the buttom is pushed
				{
					holding_inputs.cursor_primary = 1;
				}
				break;
				case WM_LBUTTONUP:
					holding_inputs.cursor_primary = 0;
				break;
				case WM_RBUTTONDOWN:
					holding_inputs.cursor_secondary = 1;
				break;
				case WM_RBUTTONUP:
					holding_inputs.cursor_secondary = 0;
				break;

				case WM_MOUSEWHEEL:
				break;

				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYDOWN:
				case WM_KEYUP:
				{
					u32 vkcode = message.wParam;
					u16 repeat_count = (u16)message.lParam;
					b32 was_down = ((message.lParam & (1 <<  30)) != 0);
					b32 is_down = ((message.lParam & (1 << 31)) == 0 );
					ASSERT(is_down == 0 || is_down == 1);
					if(is_down != was_down)
					{
						if(vkcode == 'A')
							holding_inputs.d_left = is_down;
						else if(vkcode == 'D')
							holding_inputs.d_right = is_down;
						else if(vkcode == 'W')
							holding_inputs.d_up = is_down;
						else if(vkcode == 'S')
							holding_inputs.d_down = is_down;
						else if(vkcode == VK_SPACE)
							holding_inputs.move = is_down;
						// else if(vkcode == VK_SHIFT)
						// 	holding_inputs.backward = is_down;
						else if(vkcode == 'F')
							holding_inputs.cancel = is_down;
						// else if(vkcode == 'X')
						// 	holding_inputs.shoot = is_down;
						else if(vkcode == 'Q')
							holding_inputs.L = is_down;
						else if(vkcode == 'E')
							holding_inputs.R = is_down;
						
						
						if(is_down)
						{
							if(vkcode == 'J')
								memory.camera_rotation.z -= PI32/180;
							else if(vkcode == 'L')
								memory.camera_rotation.z += PI32/180;
							else if(vkcode == 'U')
								fov = fov/2;
							else if(vkcode == 'O')
								fov = fov*2;
							else if(vkcode == 'P')
								perspective_on = !perspective_on;
							// else if(message.wParam == 'I')
							// else if(message.wParam == 'K')
							else if(vkcode == 'I')
								memory.fov = memory.fov/2;
							else if(vkcode == 'K')
								memory.fov = memory.fov*2;
							// else if(message.wParam == 'T')
							// else if(message.wParam == 'F')
							else if(vkcode == 'M')
								memory.lock_mouse = !memory.lock_mouse;
#if DEBUGMODE
							else if(vkcode == VK_F5)
								global_running = false;
#endif
						}

						b32 AltKeyWasDown = ((message.lParam & (1 << 29)));
						if ((vkcode == VK_F4) && AltKeyWasDown)
						{
							global_running = false;
						}
					}

				}break;
				default:
					TranslateMessage(&message);
					DispatchMessageA(&message);
			}
		}
		//TODO: shortcuts system
		UNTIL(i, ARRAYCOUNT(input.buttons))
		{
			// input.buttons[i] = holding_inputs.buttons[i] + input.buttons[i]*holding_inputs.buttons[i];
			if(holding_inputs.buttons[i]) {
				input.buttons[i]++;
			}else{
				if(input.buttons[i] > 0)
					input.buttons[i] = -1; // just released button
				else
					input.buttons[i] = 0;
			}
		}
		// APP UPDATE
		app.update(&memory);

		// APP RENDER
		LIST(Renderer_request, render_list) = {0};
		app.render(&memory, render_list, client_size);

		if(dx->render_target_view)
		{
			// apparently always need to clear this buffers before rendering to them
			dx->context->ClearRenderTargetView(dx->render_target_view, (float*)&bg_color);
			Depth_stencil* ds_node = depth_stencils_list[0];
			FOR_EACH(depth_stencils_list, i){
				Depth_stencil* current_ds = ds_node;
				NEXT_ELEM(ds_node);

				dx->context->ClearDepthStencilView(
					current_ds->view, 
					D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
					1, 0);   
			}
			//TODO: this should be done by the render requests from the app layer
			dx11_bind_render_target_view(dx, &dx->render_target_view, depth_stencils_list[0]->view);
			dx11_bind_rasterizer_state(dx, dx->rasterizer_state);
			dx11_bind_sampler(dx, &dx->sampler);

			// RENDER HERE

			// 3D RENDER PIPELINE

			// WORLD VIEW
			// NEGATIVE VALUES CUZ MOVING THE WHOLE WORLD IN THE OPOSITE DIRECTION
			// GIVES THE ILLUSION OF YOU MOVING THE CAMERA
			view_matrix = 
				XMMatrixTranslation( -memory.camera_pos.x, -memory.camera_pos.y,-memory.camera_pos.z )*
				XMMatrixRotationZ( memory.camera_rotation.z )*
				XMMatrixRotationY(-memory.camera_rotation.y)*
				XMMatrixRotationX(-memory.camera_rotation.x) ;
			dx11_modify_resource(dx, view_buffer.buffer, &view_matrix, sizeof(view_matrix));	
			
			// WORLD PROJECTION
			if(perspective_on)
				projection_matrix = XMMatrixPerspectiveLH(memory.fov*memory.aspect_ratio/16, memory.fov/16, fov, 100.0f);
			else
				projection_matrix = XMMatrixOrthographicLH(memory.fov*memory.aspect_ratio, memory.fov, fov, 100.0f);
			dx11_modify_resource(dx, projection_buffer.buffer, &projection_matrix, sizeof(projection_matrix));			

			// OBJECT TRANSFORM
			//TODO: make a GENERAL RENDER REQUEST QUEUE 
			Renderer_request* request_node = render_list[0];
			FOR_EACH(render_list, i){
				Renderer_request* request = request_node;
				NEXT_ELEM(request_node);

				ASSERT(request->type_flags); //assert at least one flag is set

				if(request->type_flags & REQUEST_FLAG_RENDER_OBJECT){
					Object3d* object = &request->object3d;
					ASSERT(object->color.a); // FORGOR TO SET THE COLOR
					ASSERT(object->scale.x || object->scale.y || object->scale.z); // FORGOR TO SET THE SCALE
					XMMATRIX object_transform_matrix = 
						XMMatrixScaling(object->scale.x,object->scale.y,object->scale.z)*
						XMMatrixRotationX(object->rotation.x) *
						XMMatrixRotationY(object->rotation.y) *
						XMMatrixRotationZ(object->rotation.z) *
						XMMatrixTranslation(object->pos.x,object->pos.y, object->pos.z); 

					Dx_mesh* object_mesh; LIST_GET(meshes_list, object->mesh_uid, object_mesh);
					Dx11_texture* texture; LIST_GET(textures_list, object->tex_uid.uid, texture);

					if(object->tex_uid.is_atlas){
						// TODO: make sure this rect is normalized
						Rect texrect = texture->texrects[object->tex_uid.rect_uid];
						dx11_modify_resource(dx, object_texrect_buffer.buffer, &texrect, sizeof(Rect));
					}else{
						dx11_modify_resource(dx, object_texrect_buffer.buffer, &default_texrect, sizeof(Rect));
					}
					
					dx11_bind_texture_view(dx, &texture->texview);
					dx11_modify_resource(dx, object_color_buffer.buffer, &object->color, sizeof(Color));

					dx11_draw_mesh(dx, object_buffer.buffer, object_mesh, &object_transform_matrix);
				
				}if(request->type_flags & REQUEST_FLAG_RENDER_IMAGE){//TODO: yeah not ready yet to do instancing
					Object3d* object = &request->object3d;
					ASSERT(object->color.a); // FORGOR TO SET THE COLOR
					ASSERT(object->scale.x || object->scale.y || object->scale.z); // FORGOR TO SET THE SCALE
					
					Packed_tex_info tex_info = memory.font_charinfo[object->tex_uid.rect_uid];
					object->scale.x *= ((2.0f*tex_info.w) / client_size.x);
					object->scale.y *= (2.0f*tex_info.h) / client_size.y;
					
					r32 half_pixel_offset = 0.25f;
					r32 xoffset = (tex_info.xoffset+half_pixel_offset)/client_size.x;
					r32 yoffset = (2*(tex_info.yoffset)+half_pixel_offset)/client_size.y;
					request->object3d.pos.x += xoffset; 
					request->object3d.pos.y -= yoffset;

					XMMATRIX object_transform_matrix = 
						XMMatrixScaling(object->scale.x,object->scale.y,object->scale.z)*
						XMMatrixRotationX(object->rotation.x) *
						XMMatrixRotationY(object->rotation.y) *
						XMMatrixRotationZ(object->rotation.z) *
						XMMatrixTranslation(object->pos.x,object->pos.y, object->pos.z); 

					Dx_mesh* object_mesh; LIST_GET(meshes_list, object->mesh_uid, object_mesh);
					Dx11_texture* texture; LIST_GET(textures_list, object->tex_uid.uid, texture);

					if(object->tex_uid.is_atlas){
						// TODO: make sure this rect is normalized
						Rect texrect = texture->texrects[object->tex_uid.rect_uid];
						dx11_modify_resource(dx, object_texrect_buffer.buffer, &texrect, sizeof(Rect));
					}else{
						dx11_modify_resource(dx, object_texrect_buffer.buffer, &default_texrect, sizeof(Rect));
					}
					
					dx11_bind_texture_view(dx, &texture->texview);
					dx11_modify_resource(dx, object_color_buffer.buffer, &object->color, sizeof(Color));

					dx11_draw_mesh(dx, object_buffer.buffer, object_mesh, &object_transform_matrix);
					
				}if(request->type_flags & REQUEST_FLAG_SET_VS){
					Vertex_shader* vertex_shader; LIST_GET(vertex_shaders_list, request->vshader_uid, vertex_shader);
					dx11_bind_vs(dx, vertex_shader->shader);
					dx11_bind_input_layout(dx, vertex_shader->input_layout);
				}if(request->type_flags & REQUEST_FLAG_SET_PS){
					Dx11_pixel_shader** pixel_shader; LIST_GET(pixel_shaders_list, request->pshader_uid, pixel_shader);
					dx11_bind_ps(dx, *pixel_shader);
				}if(request->type_flags & REQUEST_FLAG_SET_BLEND_STATE){
					Dx11_blend_state** blend_state; LIST_GET(blend_states_list, request->blend_state_uid, blend_state);
					dx11_bind_blend_state(dx, *blend_state);
				}if(request->type_flags & REQUEST_FLAG_SET_DEPTH_STENCIL){
					Depth_stencil* depth_stencil; LIST_GET(depth_stencils_list, request->depth_stencil_uid, depth_stencil);
					dx11_bind_depth_stencil_state(dx, depth_stencil->state);
					dx11_bind_render_target_view(dx, &dx->render_target_view, depth_stencil->view);
				}
			}
			// PRESENT RENDERING
			hr = dx->swap_chain->Present(1,0);
			ASSERTHR(hr);
		}

		{//FRAME CAPPING
			LARGE_INTEGER current_wall_clock;
			QueryPerformanceCounter(&current_wall_clock);

			r32 frame_seconds_elapsed = (r32)(current_wall_clock.QuadPart - last_counter.QuadPart) / (r32)performance_counter_frequency;

			DWORD sleep_ms = 0;
			if(frame_seconds_elapsed < target_seconds_per_frame)
			{
				if(sleep_is_granular)
				{
					sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - frame_seconds_elapsed));
					if(sleep_ms > 0)
						SleepEx(sleep_ms, false);
				}
				while(frame_seconds_elapsed < target_seconds_per_frame)
				{
					QueryPerformanceCounter(&current_wall_clock); 
					frame_seconds_elapsed = (r32)(current_wall_clock.QuadPart - last_counter.QuadPart) / (r32)performance_counter_frequency;
				}
			}else{
				//TODO: Missed Framerate
			}

			memory.delta_time = frame_seconds_elapsed;
			u32 frame_ms_elapsed = (u32)(frame_seconds_elapsed*1000);
			memory.time_ms += frame_ms_elapsed;
			
			//PRINT TIME AND DELTA_TIME
			// char text_buffer[256];
			// wsprintfA(text_buffer, "%d, %d \n", frame_ms_elapsed, memory.time_ms);
			// OutputDebugStringA(text_buffer);   
		}
#if DEBUGMODE
		{// DEBUG FRAMERATE
			LARGE_INTEGER current_wall_clock;
			QueryPerformanceCounter(&current_wall_clock);

			r32 ms_per_frame = 1000.0f * (r32)(current_wall_clock.QuadPart - last_counter.QuadPart) / (r32)performance_counter_frequency;
			s64 end_cycle_count = __rdtsc(); // clock cycles count


			u64 cycles_elapsed = end_cycle_count - last_cycles_count;
			r32 FPS = (1.0f / (ms_per_frame/1000.0f));
			s32 MegaCyclesPF = (s32)((r64)cycles_elapsed / (r64)(1000*1000));

			char text_buffer[256];
			wsprintfA(text_buffer, "%dms/f| %d f/s|  %d Mhz/f \n", (s32)ms_per_frame, (s32)FPS, MegaCyclesPF);
			// OutputDebugStringA(text_buffer);   
			last_cycles_count = __rdtsc();
		}
#endif
		QueryPerformanceCounter(&last_counter);
	}
	//TODO: this is dumb but i don't want dumb messages each time i close
	dx->device->Release();
	dx->context->Release();
	dx->swap_chain->Release();
	dx->rasterizer_state->Release();
	dx->sampler->Release();
	dx->render_target_view->Release();

	object_buffer.buffer->Release();
	view_buffer.buffer->Release();
	projection_buffer.buffer->Release();
	object_color_buffer.buffer->Release();

	Dx_mesh* mesh_node = meshes_list[0];
	FOR_EACH(meshes_list, i)
	{
		Dx_mesh* current_mesh = mesh_node;
		NEXT_ELEM(mesh_node);
		current_mesh->vertex_buffer->Release();
		current_mesh->index_buffer->Release();
	}
	Dx11_texture* texture_node = textures_list[0];
	FOR_EACH(textures_list, i)
	{
		Dx11_texture* current_tex = texture_node;
		NEXT_ELEM(texture_node);
		(current_tex->texview)->Release();
	}

	Vertex_shader* vsnodes = vertex_shaders_list[0];
	FOR_EACH(vertex_shaders_list, i){
		Vertex_shader* current_vs = vsnodes;
		NEXT_ELEM(vsnodes);

		current_vs->shader->Release();
		current_vs->input_layout->Release();
	}

	Dx11_pixel_shader** psnodes = pixel_shaders_list[0];
	FOR_EACH(pixel_shaders_list, i){
		Dx11_pixel_shader** current_ps = psnodes;
		NEXT_ELEM(psnodes);

		(*current_ps)->Release();
	}

	Dx11_blend_state** blendnodes = blend_states_list[0];
	FOR_EACH(blend_states_list, i){
		Dx11_blend_state** current_blend = blendnodes;
		NEXT_ELEM(blendnodes);

		(*current_blend)->Release();
	}

	Depth_stencil* stencilnodes = depth_stencils_list[0];
	FOR_EACH(depth_stencils_list, i){
		Depth_stencil* current_stencil = stencilnodes;
		NEXT_ELEM(stencilnodes);

		current_stencil->view->Release();
		current_stencil->state->Release();
	}

#if DEBUGMODE
	// Create an instance of the DXGI debug interface
	IDXGIDebug1* p_debug = nullptr;

	hr = DXGIGetDebugInterface1(0, IID_PPV_ARGS(&p_debug));
	ASSERTHR(hr);
	// Enable DXGI object tracking
	p_debug->EnableLeakTrackingForThread();

	// Call the ReportLiveObjects() function to report live DXGI objects
	hr = p_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
	ASSERTHR(hr);
	// Release the DXGI debug interface
	p_debug->Release();

#endif
	return 0;
}