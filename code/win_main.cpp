#include "pch.h"

#include "app.h"

#include "win_functions.h"

#include "d3d11_layer.h"


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
	Int2 win_size = {800, 600};

	// WINDOW CREATION
	WNDCLASSA window_class = {0};
	window_class.style = CS_VREDRAW|CS_HREDRAW;
	window_class.lpfnWndProc = win_main_window_proc;
	window_class.hInstance = h_instance;
	window_class.lpszClassName = "classname";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	ASSERT(RegisterClassA(&window_class));

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

	// GETTING CLIENT SIZES
	Int2 client_size = win_get_client_sizes(global_main_window);
	r32 aspect_ratio =  1;
	if(client_size.y) aspect_ratio = (r32)client_size.x / (r32)client_size.y;
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
		hr = dxgi_adapter->GetParent(IID_IDXGIFactory, (void**)&factory);
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

		factory->Release();
		dxgi_adapter->Release();
		dxgi_device->Release();
	}
	//TODO: FIND A WAY TO MAKE THIS CALLS FROM THE APP LAYER
	Render_pipeline pipeline_3d = {0};
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
	
	dx11_create_input_layout(dx, shaders_3d_compiled_vs, ied, ARRAY_COUNT(ied), &pipeline_3d.input_layout);
	
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

	
	// CREATING  D3D PIPELINES
	dx11_create_sampler(dx, &dx->sampler);
	dx11_create_rasterizer_state(dx, &dx->rasterizer_state);
	dx11_create_blend_state(dx, &pipeline_3d.blend_state, false);
	dx11_create_depth_stencil_state(dx, &pipeline_3d.depth_stencil_state, true);

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
	Object3d* ogre = list_push_back_struct(&pipeline_3d.list, Object3d, permanent_arena);
	*ogre = object3d(&ogre_mesh);
	// Object3d* ogre2 =  list_push_back_struct(&pipeline_3d.list, Object3d, permanent_arena);
	// *ogre2 = object3d(&ogre_mesh);

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
		3,
		triangle_indices,
		3,
	};
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
		test_plane_vertices, ARRAY_COUNT(test_plane_vertices), sizeof(Vertex3d),
		test_plane_indices, ARRAY_COUNT(test_plane_indices), 
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	

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

	// FRAME CAPPING SETUP
	UINT desired_scheduler_ms = 1;
	b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);

	LARGE_INTEGER pcf_result;
	QueryPerformanceFrequency(&pcf_result);
	s64 performance_counter_frequency = pcf_result.QuadPart;

	//TODO: maybe in the future use GetDeviceCaps() to get the monitor hz
	int monitor_refresh_hz = 60;
	
	//TODO: this should be = monitor_refresh_hz;
	r32 update_hz = (r32)monitor_refresh_hz;
	r32 target_seconds_per_frame = 1.0f / update_hz;

#if DEBUGMODE
	u64 last_cycles_count = __rdtsc();
#endif
	LARGE_INTEGER last_counter;
	QueryPerformanceCounter(&last_counter);

	// APP INITIALIZE 
	app.init(&memory);
	
	// TODO: input backbuffer
	User_input input = {0};
	memory.input = &input;


	r32 fov = 1;
	r32 fov2 = 1;

	//TODO: delta_time
	r32 delta = 0.0f;
	// MAIN LOOP ____________________________________________________________
	global_running = 1;
	while(global_running)
	{
		arena_pop_size(temp_arena, temp_arena->used);
		// HANDLE WINDOW RESIZING
		Int2 current_client_size = win_get_client_sizes(global_main_window);
		if(!dx->render_target_view || client_size.x != current_client_size.x || client_size.y != current_client_size.y)
		{
			client_size = current_client_size;
			if(dx->render_target_view)
			{
				dx->render_target_view->Release();
				pipeline_3d.depth_stencil_view->Release();
				dx->render_target_view = 0;
			}
			
			if(client_size.x != 0 && client_size.y != 0)
			{
				ASSERT(client_size.x < 4000 && client_size.y < 4000);
				aspect_ratio = (r32)client_size.x / (r32) client_size.y;
				hr = dx->swap_chain->ResizeBuffers(0, client_size.x, client_size.y, DXGI_FORMAT_UNKNOWN, 0);
				ASSERTHR(hr);

				dx11_create_render_target_view(dx, &dx->render_target_view);
				dx11_create_depth_stencil_view(dx, &pipeline_3d.depth_stencil_view, client_size.x, client_size.y);
				
				dx11_set_viewport(dx, 0, 0, client_size.x, client_size.y);
			}
		}

		// MOUSE POSITION
		{
			POINT mousep;
			GetCursorPos(&mousep);
			ScreenToClient(global_main_window, &mousep);

			input.cursor_pos.x = mousep.x;
			input.cursor_pos.y = mousep.y;
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
				{
					ASSERT(false);
					global_running = 0;
				}break;

				case WM_LBUTTONDBLCLK:
				break;
				case WM_LBUTTONDOWN:// just when the buttom is pushed
				{
					input.A = true;
				}
				break;
				case WM_LBUTTONUP:
					input.A = false;
				break;
				case WM_RBUTTONDOWN:
				break;
				case WM_RBUTTONUP:
				break;

				case WM_MOUSEWHEEL:
				break;

				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYDOWN:
				case WM_KEYUP:
				break;
				
				break;
				default:
					TranslateMessage(&message);
					DispatchMessageA(&message);
			}
		}
		// APP UPDATE
		app.update(&memory);

		// APP RENDER
		app.render(&memory, client_size);

		Color bg_color = {0.1f, 0.4f, 0.5f, 1};
		if(dx->render_target_view)
		{
			// apparently always need to clear this buffers before rendering to them
			dx->context->ClearRenderTargetView(dx->render_target_view, (float*)&bg_color);
			dx->context->ClearDepthStencilView(
				pipeline_3d.depth_stencil_view, 
				D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 
				1, 0);   

			dx11_bind_render_target_view(dx, &dx->render_target_view, pipeline_3d.depth_stencil_view);
			dx11_bind_rasterizer_state(dx, dx->rasterizer_state);
			dx11_bind_sampler(dx, &dx->sampler);

			// RENDER HERE
			view_matrix = 
				XMMatrixTranslation( 0,0,0 )*
				XMMatrixRotationZ( 0 )*
				XMMatrixRotationY(-(r32)input.cursor_pos.x / client_size.x )*
				XMMatrixRotationX((r32)input.cursor_pos.y /client_size.y ) ;
			dx11_modify_resource(dx, view_buffer.buffer, &view_matrix, sizeof(view_matrix));	
			
			
			bool perspective_on = true;
			if(perspective_on)
				projection_matrix = XMMatrixPerspectiveLH(fov2, fov2*aspect_ratio, fov, 100.0f);
			else
				projection_matrix = XMMatrixOrthographicLH(fov2*4, fov2*4*aspect_ratio, fov, 100.0f);
			dx11_modify_resource(dx, projection_buffer.buffer, &projection_matrix, sizeof(projection_matrix));			

			XMMATRIX object_transform_matrix =   //IDENTITY_MATRIX;
				XMMatrixScaling(1,1,1)*
				XMMatrixRotationX(0) *
				XMMatrixRotationY(0) *
				XMMatrixRotationZ(0) *
				XMMatrixTranslation(0, 0, ogre->pos.z);
			dx11_draw_mesh(dx, &pipeline_3d, object_buffer.buffer, &ogre_mesh, &object_transform_matrix);
			ogre->pos.z += 0.01f;

			dx11_draw_mesh(dx, &pipeline_3d, object_buffer.buffer, &triangle_mesh, &object_transform_matrix);

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
			OutputDebugStringA(text_buffer);   
			last_cycles_count = __rdtsc();
		}
#endif
		QueryPerformanceCounter(&last_counter);
	}
	return 0;
}