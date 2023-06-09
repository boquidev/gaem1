#include "pch.h"
#include "definitions.h"
#include "app.h"

#include "d3d11_layer.h"


HWND global_main_window = 0;
b32 global_running = 0;

struct File_data
{
	void* data;
	u32 size;
};

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

internal File_data 
win_read_file(String filename, Memory_arena* arena)
{
	File_data result = {0};

	HANDLE file_handle = CreateFileA(filename.text, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(file_handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		ASSERT(false);
	}
	LARGE_INTEGER file_size;
	if( GetFileSizeEx(file_handle, &file_size) )
	{
		u32 file_size_32 = (u32)file_size.QuadPart;
		result.data = arena_push_size(arena, file_size_32);
		if(result.data)
		{
			DWORD bytes_read;
			if(ReadFile(file_handle, result.data, file_size_32, &bytes_read, 0) && (file_size_32 == bytes_read))
				result.size = file_size_32;
			else
				arena_pop_size(arena, file_size_32);
		}
	}
	ASSERT(result.size);
	CloseHandle(file_handle);

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
	Int2 client_size = {0};
	RECT rect;
	GetClientRect(global_main_window, &rect);
	client_size.x = rect.right - rect.left;
	client_size.y = rect.bottom - rect.top;

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

	// DIRECTX11
	// INITIALIZE DIRECT3D

	D3D dx = {0};
	HRESULT hr;
	// CREATE DEVICE AND SWAPCHAIN
	{
		D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};

		D3D_FEATURE_LEVEL result_feature_level;
		u32 create_device_flags = DX11_CREATE_DEVICE_DEBUG_FLAG;

		hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, create_device_flags, feature_levels, 
			ARRAYSIZE(feature_levels), D3D11_SDK_VERSION, &dx.device, &result_feature_level, &dx.context);
		ASSERTHR(hr);

	#if DEBUGMODE
		//for debug builds enable debug break on API errors
		ID3D11InfoQueue* info;
		dx.device->QueryInterface(IID_ID3D11InfoQueue, (void**)&info);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		info->Release();

		// hresult return values should not be checked anymore
		// cuz debugger will break on errors but i will check them anyway
	#endif

		IDXGIDevice* dxgi_device;
		hr = dx.device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
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
		hr = factory->CreateSwapChainForHwnd(dx.device, global_main_window, &scd, 0, 0, &dx.swap_chain);
		ASSERTHR(hr);

		// disable Alt+Enter changing monitor resolution to match window size
		factory->MakeWindowAssociation(global_main_window, DXGI_MWA_NO_ALT_ENTER);

		factory->Release();
		dxgi_adapter->Release();
		dxgi_device->Release();
	}

	// GETTING COMPILED SHADERS
	String source_shaders_filename = string("x:/source/code/shaders/simple_shaders.hlsl");

	// SHADERS
	{
		File_data source_shaders_file = win_read_file(source_shaders_filename, temp_arena);
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
	
	User_input input = {0};
	memory.input = &input;


	// MAIN LOOP
	global_running = 1;
	while(global_running)
	{
		{
			POINT mousep;
			GetCursorPos(&mousep);
			ScreenToClient(global_main_window, &mousep);

			input.cursor_pos.x = mousep.x;
			input.cursor_pos.y = mousep.y;
		}
		
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