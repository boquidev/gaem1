
#include "pch.h"

#include "app.h"

#include "win_functions.h"

#include "d3d11_layer.h"

// MINIAUDIO LIBRARY
#define MINIAUDIO_IMPLEMENTATION
#include "libraries/miniaudio.h"

#include "miniaudio_layer.h"

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

b32 global_running = 0;
HWND global_main_window = 0;
Int2 global_client_size = {0};
u32 error;

struct App_dll
{
	b32 is_valid;

	update_type(update);
	render_type(render);
	init_type(init);
};

#define ASSERMSG(msg) case msg:ASSERT(false); break;
LRESULT CALLBACK
win_main_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch(message)
	{
		case WM_DESTROY:
		case WM_QUIT:
		case WM_CLOSE:
			global_running = false;
		break;
		case WM_SIZING:{
			RECT* result_rect = (RECT*)lParam;
			Int2 new_size = {result_rect->right - result_rect->left, result_rect->bottom - result_rect->top};

			RECT winrect = {0,0,global_client_size.x, global_client_size.y};
			AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW,0,0);
			Int2 win_size = {winrect.right-winrect.left, winrect.bottom-winrect.top};
			GetWindowRect(window, &winrect);
			Int2 winclient_diff = win_size - global_client_size;
			
			s32 new_height = MAX(90,new_size.y - winclient_diff.y);
			s32 new_width = MAX(160,new_size.x - winclient_diff.x);


			s32 new_bottom = 0;
			switch (wParam){
				case WMSZ_TOP:
					result_rect->top = MIN(result_rect->top,winrect.bottom - (winclient_diff.y+new_height));
				case WMSZ_BOTTOMRIGHT:
				case WMSZ_BOTTOM:
					new_width = (s32)(0.5f+(16*new_height/9));
					new_bottom =  result_rect->top + (new_height+winclient_diff.y);
				break;
				
				case WMSZ_LEFT:
				case WMSZ_TOPLEFT:
				case WMSZ_BOTTOMLEFT:
					result_rect->left = MIN(result_rect->left,winrect.right - (winclient_diff.x+new_width));
				case WMSZ_RIGHT:
					new_height = (s32)(0.5f+(9*new_width/16));
					new_bottom =  result_rect->top + (new_height+winclient_diff.y);
				break;
				case WMSZ_TOPRIGHT:
					
					new_bottom =  winrect.bottom;
					new_height = (s32)(0.5f+(9*new_width/16));
					result_rect->top = winrect.bottom - (winclient_diff.y+new_height);
				break;
				default:
					new_height = MAX(new_height,(s32)(0.5f+(9*new_width/16)));
					new_width = MAX(new_width, (s32)(0.5f+(16*new_height/9)));
					new_bottom =  result_rect->top + (new_height+winclient_diff.y);
				break;
			}
			result_rect->right = result_rect->left + (new_width+winclient_diff.x);
			result_rect->bottom = new_bottom;
			ASSERT(true);
		}break;
		// case WM_WINDOWPOSCHANGING:{
		// 	WINDOWPOS* new_win = (WINDOWPOS*)lParam;
		// 	if(new_win->flags & SWP_NOSIZE){
		// 		result = DefWindowProc(window, message, wParam, lParam);
		// 	}else if(new_win->cx && new_win->cy){

		// 		RECT winrect = {0,0 ,global_client_size.x, global_client_size.y};
		// 		AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW,0,0);
		// 		Int2 win_size = {winrect.right-winrect.left, winrect.bottom-winrect.top};
		// 		ASSERT(win_size.x < 2000);
		// 		ASSERT(win_size.y < 2000);
		// 		Int2 winclient_diff = win_size - global_client_size;
		// 		ASSERT(winclient_diff.x == 16 && winclient_diff.y == 39);
		// 		s32 new_width = MAX(0,new_win->cx-winclient_diff.x);
		// 		s32 new_height = MAX(0,new_win->cy-winclient_diff.y);
		// 		if(win_size.x != new_win->cx){
		// 			new_height = (s32)(0.5f+(9*new_width/16));
		// 		}
		// 		if(win_size.y != new_win->cy){	
		// 			new_width = (s32)(0.5f+(16*new_height/9));
		// 		}
		// 		ASSERT(ABS(new_win->cy-new_height) < 450);
		// 		ASSERT(ABS(new_win->cx-new_width) < 450);
		// 		new_win->cy = new_height + winclient_diff.y;
		// 		new_win->cx = new_width + winclient_diff.x;
		// 	}
		// }break;
		//TODO: could i get rid of these?
		ASSERMSG(WM_MOUSEWHEEL)
		ASSERMSG(WM_LBUTTONDBLCLK)
		ASSERMSG(WM_LBUTTONDOWN)
		ASSERMSG(WM_LBUTTONUP) 
		ASSERMSG(WM_RBUTTONDOWN) 
		ASSERMSG(WM_RBUTTONUP) 
		ASSERMSG(WM_SYSKEYDOWN) 
		ASSERMSG(WM_SYSKEYUP) 
		ASSERMSG(WM_KEYDOWN) 
		ASSERMSG(WM_KEYUP) 

		// UNPROCESSED MESSAGES
		case WM_NCMOUSELEAVE:
		case WM_NCMOUSEMOVE:

		
		case WM_IME_SETCONTEXT:
		case WM_IME_NOTIFY:

		case WM_WINDOWPOSCHANGED:

		// RESIZING WINDOW MESSAGES
		case WM_GETMINMAXINFO:
		case WM_ENTERSIZEMOVE:
		case WM_EXITSIZEMOVE:
		case WM_MOVING:
		// break;


		// case WM_ERASEBKGND:
		// case WM_PAINT:
		// case WM_NCLBUTTONDOWN:
		// case WM_NCPAINT:
		// case WM_NCACTIVATE:
		// case WM_NCHITTEST:
		// case WM_NCCALCSIZE:
		// case WM_SETCURSOR:
		// case WM_ACTIVATEAPP:
		// case WM_KILLFOCUS:
		// case WM_CAPTURECHANGED:
		// case WM_SYSCOMMAND:
		default:
			result = DefWindowProc(window, message, wParam, lParam);
	}

	return result;
}

int WINAPI 
wWinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, PWSTR cmd_line, int cmd_show)
{
	RECT winrect = {0,0,1600,900};
	AdjustWindowRectEx(&winrect, WS_OVERLAPPEDWINDOW,0,0);
	Int2 win_size = {winrect.right-winrect.left, winrect.bottom-winrect.top};

	// WINDOW CREATION
	WNDCLASSA window_class = {0};
	window_class.style = CS_VREDRAW|CS_HREDRAW;
	window_class.lpfnWndProc = win_main_window_proc;
	window_class.hInstance = h_instance;
	window_class.lpszClassName = "classname";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	RegisterClassA(&window_class);

	DWORD exstyle = WS_MAXIMIZE | WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	
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
	// Int2 global_client_size = win_get_client_sizes(global_main_window);
	// if(global_client_size.y) 
	// 	memory.aspect_ratio = (r32)global_client_size.x / (r32)global_client_size.y;

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
	init_data.meshes_serialization = win_read_file(string("data/meshes_init.txt"),temp_arena);
	init_data.textures_serialization = win_read_file(string("data/textures_init.txt"),temp_arena);
	app.init(&memory, &init_data);
	
	LIST(Dx11_texture_view*, textures_list) = {0};
	LIST(Vertex_shader, vertex_shaders_list) = {0};
	LIST(Dx11_pixel_shader*, pixel_shaders_list) = {0};
	LIST(Dx_mesh, meshes_list) = {0};
	LIST(Dx11_blend_state*, blend_states_list) = {0};
	LIST(Depth_stencil, depth_stencils_list) = {0};
	LIST(ma_decoder, sounds_list) = {0};
	FOREACH(Asset_request, request, init_data.asset_requests){
		switch(request->type){
			case TEX_FROM_FILE_REQUEST:{
				int comp;
				Surface tex_surface = {0};
				char temp_buffer [MAX_PATH] = {0}; 
				copy_mem(request->filename.text, temp_buffer, request->filename.length);
				tex_surface.data = stbi_load(temp_buffer, (int*)&tex_surface.width, (int*)&tex_surface.height, &comp, STBI_rgb_alpha);
				ASSERT(tex_surface.data);
				
				Tex_info* tex_info; PUSH_BACK(memory.tex_infos, permanent_arena, tex_info);
				tex_info->w = tex_surface.width;
				tex_info->h = tex_surface.height;
				tex_info->texrect.xf = 0.0f;
				tex_info->texrect.yf = 0.0f;
				tex_info->texrect.wf = 1.0f;
				tex_info->texrect.hf = 1.0f;

				tex_info->texview_uid = LIST_SIZE(textures_list);
				Dx11_texture_view** texture_view; PUSH_BACK(textures_list, permanent_arena, texture_view);
				dx11_create_texture_view(dx, &tex_surface, texture_view);
			}break;


			case VERTEX_SHADER_FROM_FILE_REQUEST:{
				// COMPILING VS
					// File_data compiled_vs = dx11_get_compiled_shader(request->filename, temp_arena, "vs", VS_PROFILE);
				// CREATING VS
				File_data compiled_vs = win_read_file(request->filename, temp_arena);

				*request->p_uid = LIST_SIZE(vertex_shaders_list);
				Vertex_shader* vs; PUSH_BACK(vertex_shaders_list, permanent_arena, vs);
				dx11_create_vs(dx, compiled_vs, &vs->shader);
				
				D3D11_INPUT_ELEMENT_DESC* ied = ARENA_PUSH_STRUCTS(temp_arena, D3D11_INPUT_ELEMENT_DESC, request->ied.count);
				u32 current_aligned_byte_offset = 0;
				UNTIL(j, request->ied.count){
					ied[j].SemanticName = request->ied.names[j].text;
					// ied[j].SemanticIndex // this is in case the element is bigger than a float4 (a matrix for example)
					ied[j].Format = input_element_format_from_size(request->ied.sizes[j]);
					// ied[j].InputSlot // this is for using secondary buffers (like an instance buffer)
					ied[j].AlignedByteOffset = current_aligned_byte_offset;
					current_aligned_byte_offset += request->ied.sizes[j]; // reset this value if using instance buffer
					// ied[j].InputSlotClass; // 0 PER_VERTEX_DATA vs 1 PER_INSTANCE_DATA
					// ied[j].InstanceDataStepRate; // the amount of instances to draw using the PER_INSTANCE data
				}
				dx11_create_input_layout(dx, compiled_vs, ied, request->ied.count, &vs->input_layout);
			}break;


			case PIXEL_SHADER_FROM_FILE_REQUEST:{
				*request->p_uid = LIST_SIZE(pixel_shaders_list);
				// COMPILING PS
					// File_data compiled_ps = dx11_get_compiled_shader(request->filename, temp_arena, "ps", PS_PROFILE);
				// CREATING PS
				File_data compiled_ps = win_read_file(request->filename, temp_arena);

				Dx11_pixel_shader** ps; PUSH_BACK(pixel_shaders_list, permanent_arena, ps);
				dx11_create_ps(dx, compiled_ps, ps);
			}break;


			case MESH_FROM_FILE_REQUEST:{
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
				Dx_mesh* current_mesh; PUSH_BACK(meshes_list, permanent_arena, current_mesh);
				*current_mesh = dx11_init_mesh(dx, 
				&primitives[0],
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
			}break;


			case CREATE_BLEND_STATE_REQUEST:{
				*request->p_uid = LIST_SIZE(blend_states_list);
				Dx11_blend_state** blend_state; PUSH_BACK(blend_states_list, memory.permanent_arena, blend_state);
				dx11_create_blend_state(dx, blend_state, request->enable_alpha_blending);
			}break;


			case CREATE_DEPTH_STENCIL_REQUEST:{
				*request->p_uid = LIST_SIZE(depth_stencils_list);
				Depth_stencil* depth_stencil; PUSH_BACK(depth_stencils_list, memory.permanent_arena, depth_stencil);
				dx11_create_depth_stencil_state(dx, &depth_stencil->state, request->enable_depth);
			}break;


			case FONT_FROM_FILE_REQUEST:{
				float lines_height = 18.0f;
				// FONTS 
				File_data font_file = win_read_file(request->filename, temp_arena);
				//currently just supporting ANSI characters
				//TODO: learn how to do UNICODE

				// GETTING BITMAPS AND INFO
				u32 atlas_texview_uid = LIST_SIZE(textures_list);

				stbtt_fontinfo font;
				Color32* charbitmaps [CHARS_COUNT];
				Tex_info temp_charinfos[CHARS_COUNT];
				stbtt_InitFont(&font, (u8*)font_file.data,stbtt_GetFontOffsetForIndex((u8*)font_file.data, 0));
				UNTIL(c, CHARS_COUNT){
					u32 codepoint = c+FIRST_CHAR;

					temp_charinfos[c].texview_uid = atlas_texview_uid;

					u8* monobitmap = stbtt_GetCodepointBitmap(
						&font, 0, stbtt_ScaleForPixelHeight(&font, lines_height), codepoint, 
						&temp_charinfos[c].w, &temp_charinfos[c].h, 
						&temp_charinfos[c].xoffset, &temp_charinfos[c].yoffset
					);
					u32 bitmap_size = temp_charinfos[c].w * temp_charinfos[c].h;
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
					rects[i].w = temp_charinfos[i].w;
					rects[i].h = temp_charinfos[i].h;
				}
				stbrp_pack_rects(&pack_context, rects, CHARS_COUNT);

				// CREATE TEXTURE ATLAS AND COPY EACH CHARACTER BITMAP INTO IT USING THE POSITIONS OBTAINED FROM PACK
				Color32* atlas_pixels = ARENA_PUSH_STRUCTS(temp_arena, Color32, atlas_side*atlas_side);

				Int2 atlas_size = {atlas_side, atlas_side};

				UNTIL(i, CHARS_COUNT){
					ASSERT(rects[i].was_packed);
					if(rects[i].was_packed){
						temp_charinfos[i].texrect.xf = (r32)rects[i].x / atlas_size.x;
						temp_charinfos[i].texrect.yf = (r32)rects[i].y / atlas_size.y;
						temp_charinfos[i].texrect.wf = (r32)temp_charinfos[i].w / atlas_size.x;
						temp_charinfos[i].texrect.hf = (r32)temp_charinfos[i].h / atlas_size.y;
						
						memory.font_tex_infos_uids[i] = LIST_SIZE(memory.tex_infos);
						Tex_info* charinfo; PUSH_BACK(memory.tex_infos, permanent_arena, charinfo);
						*charinfo = temp_charinfos[i]; 

						// PASTING EACH CHAR INTO THE ATLAS PIXELS
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
				Tex_info* atlas_tex_info; PUSH_BACK(memory.tex_infos, permanent_arena, atlas_tex_info);
				atlas_tex_info->texview_uid = atlas_texview_uid;
				atlas_tex_info->w = atlas_size.x;
				atlas_tex_info->h = atlas_size.y;
				atlas_tex_info->texrect.xf = 0.0f;
				atlas_tex_info->texrect.yf = 0.0f;
				atlas_tex_info->texrect.wf = 1.0f;
				atlas_tex_info->texrect.hf = 1.0f;

				Dx11_texture_view** atlas_texture; PUSH_BACK(textures_list, permanent_arena, atlas_texture);
				Surface atlas_surface = {(u32)atlas_size.x, (u32)atlas_size.y, atlas_pixels};
				dx11_create_texture_view(dx, &atlas_surface, atlas_texture);
			}break;


			case TEX_FROM_SURFACE_REQUEST:{
				*request->p_uid = LIST_SIZE(memory.tex_infos);
				Tex_info* tex_info; PUSH_BACK(memory.tex_infos, permanent_arena, tex_info);
				tex_info->w = request->tex_surface.width;
				tex_info->h = request->tex_surface.height;

				tex_info->texrect.xf = 0.0f;
				tex_info->texrect.yf = 0.0f;
				tex_info->texrect.wf = 1.0f;
				tex_info->texrect.hf = 1.0f;
				
				tex_info->texview_uid = LIST_SIZE(textures_list);
				Dx11_texture_view** texture_view; PUSH_BACK(textures_list, memory.permanent_arena, texture_view);
				dx11_create_texture_view(dx, &request->tex_surface, texture_view);
			}break;


			case SOUND_FROM_FILE_REQUEST:{
				ma_result ma_error;
				ma_decoder_config decoder_config;
				
				*request->p_uid = LIST_SIZE(sounds_list);
				ma_decoder* new_decoder; PUSH_BACK(sounds_list, permanent_arena, new_decoder);

				decoder_config = ma_decoder_config_init(ma_format_f32, 2, 48000);
				
				char filename [MAX_PATH]={0};
				copy_mem(request->filename.text, filename, request->filename.length);
				ma_error = ma_decoder_init_file(filename, &decoder_config, new_decoder);
				ASSERT(ma_error == MA_SUCCESS);
			}break;


			case MESH_FROM_PRIMITIVES_REQUEST:{
				*request->p_uid = LIST_SIZE(meshes_list);
				Dx_mesh* current_mesh; PUSH_BACK(meshes_list, permanent_arena, current_mesh);
				*current_mesh = dx11_init_mesh(dx, 
				request->mesh_primitives, 
				D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}break;


			case FORGOR_TO_SET_ASSET_TYPE:
			default:
				ASSERT(false)
			break;
		}
	}
	Audio_data audio_data = {0};
	audio_data.decoders = ARENA_PUSH_STRUCTS(permanent_arena, ma_decoder*, LIST_SIZE(sounds_list));
	audio_data.decoders_count = LIST_SIZE(sounds_list);
	ma_decoder* current_decoder = sounds_list[0];
	for(u32 i=0; i<audio_data.decoders_count; i++, SKIP_ELEM(current_decoder)){
		audio_data.decoders[i] = current_decoder;
	}
	// MINIAUDIO SETUP
	{
		//TODO: this
		ma_context context;
		ASSERT(ma_context_init(NULL, 0, NULL, &context) == MA_SUCCESS);

		ma_device_info* playback_infos;
		ma_uint32 playback_count;
		ma_device_info* capture_infos;
		ma_uint32 capture_count;
		ASSERT(ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count) == MA_SUCCESS);
		
		// Loop over each device info and do something with it. Here we just print the name with their index. You may want
		// to give the user the opportunity to choose which device they'd prefer.
		for (ma_uint32 iDevice = 0; iDevice < playback_count; iDevice += 1) {
			printf("%d - %s\n", iDevice, playback_infos[iDevice].name);
		}
		ASSERT(playback_count)
		u32 chosen_device = 0;

		/* Create only a single device. The decoders will be mixed together in the callback. In this example the data format needs to be the same as the decoders. */
		ma_device_config sound_device_config = ma_device_config_init(ma_device_type_playback);
		// sound_device_config.playback.pDeviceID = &pPlaybackInfos[chosen_device].id;
		sound_device_config.playback.format   = ma_format_f32;
		sound_device_config.playback.channels = 2;
		sound_device_config.sampleRate        = 48000;
		sound_device_config.dataCallback      = miniaudio_callback;
		sound_device_config.pUserData         = &audio_data;

		ma_device sound_device;
		ASSERT(ma_device_init(NULL, &sound_device_config, &sound_device) == MA_SUCCESS);

		// ma_event_init(&g_stopEvent);
		// ma_event_wait(&g_stopEvent);
		// ma_event_signal(&g_stopEvent);
		/* 
			what i understood is that this thread will wait in the ma_event_wait function 
			until the audio thread(the callback function) sends the signal g_stop_Event
			with the function ma_event_signal;
			if the g_stopEvent is not initialized with ma_event_init
			then ma_event_wait will be ignored
		*/
		ASSERT(ma_device_start(&sound_device) == MA_SUCCESS);

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

	D3D_constant_buffer camera_pos_buffer = {0};
	dx11_create_and_bind_constant_buffer(
		dx, &camera_pos_buffer, sizeof(V4), CAMERA_POS_BUFFER_REGISTER_INDEX, 0
	);

	// CREATING  D3D PIPELINES
	dx11_create_sampler(dx, &dx->sampler);
	dx11_create_rasterizer_state(dx, &dx->rasterizer_state, D3D11_FILL_SOLID, D3D11_CULL_BACK);


	// FRAME CAPPING SETUP
	UINT desired_scheduler_ms = 1;
	b32 sleep_is_granular = (timeBeginPeriod(desired_scheduler_ms) == TIMERR_NOERROR);

	LARGE_INTEGER pcf_result;
	QueryPerformanceFrequency(&pcf_result);
	s64 performance_counter_frequency = pcf_result.QuadPart;

	//TODO: maybe in the future use GetDeviceCaps() to get the monitor hz
	int monitor_refresh_hz = 60;
	memory.delta_time = 1.0f/monitor_refresh_hz;
	
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
	b32 perspective_on = 0;
	memory.lock_mouse = false;
	Color bg_color = {0.2f, 0.2f, 0.2f, 1};
	// MAIN LOOP ____________________________________________________________
	
	global_running = 1;
	while(global_running)
	{
		arena_pop_back_size(temp_arena, temp_arena->used);


		// HANDLE WINDOW RESIZING
		Int2 current_client_size = win_get_client_sizes(global_main_window);
		if(!dx->render_target_view || global_client_size.x != current_client_size.x || global_client_size.y != current_client_size.y)
		{
			s32 new_width = current_client_size.x;
			s32 new_height = current_client_size.y;
			// if(current_client_size.x != global_client_size.x){
			// 	new_height = (s32)(0.5f+(9*current_client_size.x/16));
			// }
			// if(current_client_size.y != global_client_size.y){
			// 	new_width = (s32)(0.5f+(16*new_height/9));
			// }
			RECT new_client_size = {0,0,new_width,new_height};
			global_client_size = {new_width, new_height};

			// bool success = AdjustWindowRectEx(&new_client_size, WS_OVERLAPPEDWINDOW,0,0);
			// u32 new_window_width = new_client_size.right - new_client_size.left;
			// u32 new_window_height = new_client_size.bottom - new_client_size.top;
			// success = SetWindowPos(global_main_window, 0, 0, 0, new_window_width, new_window_height,SWP_NOMOVE);
			if(dx->render_target_view)
			{
				dx->render_target_view->Release();
				FOREACH(Depth_stencil, current_ds, depth_stencils_list){
					current_ds->view->Release();
					current_ds->view = 0;
				}
				dx->render_target_view = 0;
			}
			
			//TODO: be careful with 8k monitors
			if(global_client_size.x > 0 && global_client_size.y > 0 &&
				global_client_size.x < 4000 && global_client_size.y < 4000
			){
				ASSERT(global_client_size.x < 4000 && global_client_size.y < 4000);
				// memory.aspect_ratio = (r32)global_client_size.x / (r32) global_client_size.y;
				memory.aspect_ratio = 16.0f/9;
				hr = dx->swap_chain->ResizeBuffers(0, global_client_size.x, global_client_size.y, DXGI_FORMAT_UNKNOWN, 0);
				ASSERTHR(hr);

				dx11_create_render_target_view(dx, &dx->render_target_view);

				FOREACH(Depth_stencil, current_ds, depth_stencils_list){
					dx11_create_depth_stencil_view(dx, &current_ds->view, global_client_size.x, global_client_size.y);
				}
				
				dx11_set_viewport(dx, 0, 0, global_client_size.x, global_client_size.y);
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
				r32 px = (r32)(mousep.x-client_center_pos.x)/global_client_size.x;
				r32 py = -(r32)(mousep.y-client_center_pos.y)/global_client_size.y;
				input.cursor_speed.x = px - input.cursor_pos.x;
				input.cursor_speed.y = py - input.cursor_pos.y;
				input.cursor_pos = {0,0};
			}else
				input.cursor_pos = {
					(r32)(mousep.x - client_center_pos.x)/global_client_size.x, 
					-(r32)(mousep.y - client_center_pos.y)/global_client_size.y};
				
		}else{
			holding_inputs = {0};
		}
		// HANDLING MESSAGES
		MSG msg;
		while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
		{
			switch(msg.message)
			{
				case WM_MOUSEMOVE: // WM_MOUSEFIRST
				break;
				case WM_ACTIVATE:
					ASSERT(false);
				break;
				case WM_LBUTTONDBLCLK:
				break;
				case WM_LBUTTONDOWN:// just when the buttom is pushed
					holding_inputs.cursor_primary = 1;
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
					u32 vkcode = msg.wParam;
					u16 repeat_count = (u16)msg.lParam;
					b32 was_down = ((msg.lParam & (1 <<  30)) != 0);
					b32 is_down = ((msg.lParam & (1 << 31)) == 0 );
					ASSERT(is_down == 0 || is_down == 1);
					if(is_down != was_down)
					{
						switch(vkcode){
							case 'A':
								holding_inputs.d_left = is_down;
							break;
							case 'D':
								holding_inputs.d_right = is_down;
							break; 
							case 'W':
								holding_inputs.d_up = is_down;
							break;
							case 'S':
								holding_inputs.d_down = is_down;
							break;
							case VK_SPACE:
								holding_inputs.move = is_down;
								break;
							case 'F':
								holding_inputs.cancel = is_down;
								break;
							case 'T':
								holding_inputs.reset = is_down;
								break;
							case 'Q':
								holding_inputs.L = is_down;
								break;
							case 'E':
								holding_inputs.R = is_down;
							break;
							case '1':
								holding_inputs.k1 = is_down;
							break;
							case '2':
								holding_inputs.k2 = is_down;
							break;
							case '3':
								holding_inputs.k3 = is_down;
							break;
							case '4':
								holding_inputs.k4 = is_down;
							break;
							case '5':
								holding_inputs.k5 = is_down;
							break;
							case '6':
								holding_inputs.k6 = is_down;
							break;
							default:
							break;

						}
						if(vkcode == 'H'){
							audio_data.playback_requests[audio_data.requests_count] = audio_data.requests_count % 2;
							audio_data.requests_count ++;
						}
						// else if(vkcode == 'D')
						// 	holding_inputs.d_right = is_down;
						// else if(vkcode == 'W')
						// 	holding_inputs.d_up = is_down;
						// else if(vkcode == 'S')
						// 	holding_inputs.d_down = is_down;
						// else if(vkcode == VK_SPACE)
						// 	holding_inputs.move = is_down;
						// // else if(vkcode == VK_SHIFT)
						// // 	holding_inputs.backward = is_down;
						// else if(vkcode == 'F')
						// 	holding_inputs.cancel = is_down;
						// // else if(vkcode == 'X')
						// // 	holding_inputs.shoot = is_down;
						// else if(vkcode == 'Q')
						// 	holding_inputs.L = is_down;
						// else if(vkcode == 'E')
						// 	holding_inputs.R = is_down;
						// else if(vk)
						
						
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
								perspective_on = perspective_on^1;
							// else if(msg.wParam == 'I')
							// else if(msg.wParam == 'K')
							else if(vkcode == 'I')
								memory.fov = memory.fov/2;
							else if(vkcode == 'K')
								memory.fov = memory.fov*2;
							// else if(msg.wParam == 'T')
							// else if(msg.wParam == 'F')
							else if(vkcode == 'M')
								memory.lock_mouse = !memory.lock_mouse;
#if DEBUGMODE
							else if(vkcode == VK_F5)
								global_running = false;
#endif
						}

						b32 AltKeyWasDown = ((msg.lParam & (1 << 29)));
						if ((vkcode == VK_F4) && AltKeyWasDown)
							global_running = false;
					}

				}break;
				default:
					//TODO: this function is for when i want to handle text input with WM_CHAR messages
					TranslateMessage(&msg);
					DispatchMessageA(&msg);
					// DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
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
		app.render(&memory, render_list, global_client_size);

		if(dx->render_target_view)
		{
			// apparently always need to clear this buffers before rendering to them
			dx->context->ClearRenderTargetView(dx->render_target_view, (float*)&bg_color);
			FOREACH(Depth_stencil, current_ds, depth_stencils_list){
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
				XMMatrixRotationZ(-memory.camera_rotation.z )*
				XMMatrixRotationY(-memory.camera_rotation.y)*
				XMMatrixRotationX(-memory.camera_rotation.x) ;
			dx11_modify_resource(dx, view_buffer.buffer, &view_matrix, sizeof(view_matrix));
			V4 camera_pos = {memory.camera_pos.x, memory.camera_pos.y, memory.camera_pos.z, (r32)perspective_on};	
			dx11_modify_resource(dx, camera_pos_buffer.buffer, &camera_pos, sizeof(V4));
			
			// WORLD PROJECTION
			if(perspective_on)
				projection_matrix = XMMatrixPerspectiveLH(memory.fov*memory.aspect_ratio/32, memory.fov/32, fov, 100.0f);
			else
				projection_matrix = XMMatrixOrthographicLH(memory.fov*memory.aspect_ratio, memory.fov, fov, 100.0f);
			dx11_modify_resource(dx, projection_buffer.buffer, &projection_matrix, sizeof(projection_matrix));			

			// OBJECT TRANSFORM
			//TODO: make a GENERAL RENDER REQUEST QUEUE 
			FOREACH(Renderer_request, request, render_list){
				ASSERT(request->type_flags); //assert at least one flag is set

				if((request->type_flags & REQUEST_FLAG_RENDER_OBJECT)){
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
					Tex_info* texinfo; LIST_GET(memory.tex_infos, object->texinfo_uid, texinfo);
					Dx11_texture_view** texture_view; LIST_GET(textures_list, texinfo->texview_uid, texture_view);

					dx11_modify_resource(dx, object_texrect_buffer.buffer, &texinfo->texrect, sizeof(Rect));
					
					dx11_bind_texture_view(dx, texture_view);
					dx11_modify_resource(dx, object_color_buffer.buffer, &object->color, sizeof(Color));

					dx11_draw_mesh(dx, object_buffer.buffer, object_mesh, &object_transform_matrix);
				
				}else if((request->type_flags & REQUEST_FLAG_RENDER_IMAGE_TO_SCREEN)){
					//TODO: yeah not ready yet to do instancing
					Object3d* object = &request->object3d;
					ASSERT(object->color.a); // FORGOR TO SET THE COLOR
					ASSERT(object->scale.x && object->scale.y && object->scale.z); // FORGOR TO SET THE SCALE
			
					// Packed_tex_info tex_info = memory.font_charinfo[object->tex_uid.rect_uid];
					// object->scale.x *= ((2.0f*tex_info.w) / global_client_size.x);
					// object->scale.y *= (2.0f*tex_info.h) / global_client_size.y;

					r32 half_pixel_offset = 0.5f;
					// r32 xoffset = (tex_info.xoffset+half_pixel_offset)/global_client_size.x;
					// r32 yoffset = (2*(tex_info.yoffset)+half_pixel_offset)/global_client_size.y;
					request->object3d.pos.x -= half_pixel_offset/global_client_size.x;
					request->object3d.pos.y += half_pixel_offset/global_client_size.y;		

					XMMATRIX object_transform_matrix = 
						XMMatrixScaling(object->scale.x, object->scale.y, object->scale.z)*
						XMMatrixRotationX(object->rotation.x) *
						XMMatrixRotationY(object->rotation.y) *
						XMMatrixRotationZ(object->rotation.z) *
						XMMatrixTranslation(object->pos.x,object->pos.y, object->pos.z); 

					Dx_mesh* object_mesh; LIST_GET(meshes_list, object->mesh_uid, object_mesh);
					Tex_info* texinfo; LIST_GET(memory.tex_infos, object->texinfo_uid, texinfo);
					Dx11_texture_view** texture_view; LIST_GET(textures_list, texinfo->texview_uid, texture_view);

					dx11_modify_resource(dx, object_texrect_buffer.buffer, &texinfo->texrect, sizeof(Rect));
					
					dx11_bind_texture_view(dx, texture_view);

					dx11_modify_resource(dx, object_color_buffer.buffer, &object->color, sizeof(Color));

					dx11_draw_mesh(dx, object_buffer.buffer, object_mesh, &object_transform_matrix);
				
				}else if ((request->type_flags & REQUEST_FLAG_RENDER_IMAGE_TO_WORLD)){
					//TODO: yeah not ready yet to do instancing
					Object3d* object = &request->object3d;
					ASSERT(object->color.a); // FORGOR TO SET THE COLOR
					ASSERT(object->scale.x && object->scale.y && object->scale.z); // FORGOR TO SET THE SCALE
					
					// Tex_info tex_info = memory.font_charinfo[object->tex_uid.rect_uid];

					// r32 half_pixel_offset = 0.25f;
					// r32 xoffset = object->scale.x*(tex_info.xoffset+half_pixel_offset)/16;
					// r32 yoffset = object->scale.y*(2*(tex_info.yoffset)+half_pixel_offset)/9;
					
					// object->scale.x *= ((2.0f*tex_info.w) / 16);
					// object->scale.y *= (2.0f*tex_info.h) / 9;

					// request->object3d.pos.x += xoffset; 
					// request->object3d.pos.y -= yoffset;

					XMMATRIX object_transform_matrix = (
						(
							XMMatrixScaling(object->scale.x,object->scale.y,object->scale.z)*
							XMMatrixRotationX(object->rotation.x) *
							XMMatrixRotationY(object->rotation.y) *
							XMMatrixRotationZ(object->rotation.z) *
							XMMatrixTranslation(object->pos.x,object->pos.y, object->pos.z)
						) * view_matrix * projection_matrix
					);

					Dx_mesh* object_mesh; LIST_GET(meshes_list, object->mesh_uid, object_mesh);
					Tex_info* texinfo; LIST_GET(memory.tex_infos, object->texinfo_uid, texinfo);
					Dx11_texture_view** texture_view; LIST_GET(textures_list, texinfo->texview_uid, texture_view);

					dx11_modify_resource(dx, object_texrect_buffer.buffer, &texinfo->texrect, sizeof(Rect));
					
					dx11_bind_texture_view(dx, texture_view);
					dx11_modify_resource(dx, object_color_buffer.buffer, &object->color, sizeof(Color));

					dx11_draw_mesh(dx, object_buffer.buffer, object_mesh, &object_transform_matrix);
				
				}else{
					if(request->type_flags & REQUEST_FLAG_SET_VS){
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

			// memory.delta_time = frame_seconds_elapsed;
			// TODO: this is slower than delta time probably cuz it does not take into account sub ms times
			// OR maybe delta time is the one that's off and is faster for some reason
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
	if(dx->render_target_view)
		dx->render_target_view->Release();

	object_buffer.buffer->Release();
	view_buffer.buffer->Release();
	projection_buffer.buffer->Release();
	object_color_buffer.buffer->Release();

	// for( struct : Dx_mesh{u32 i}iterator = {0,list[0]}; 
	// iterator.i<LIST_SIZE(list); 
	// iterator.i++, SKIP_ELEM(iterator))

	FOREACH(Dx_mesh, current_mesh, meshes_list)
	{
		current_mesh->vertex_buffer->Release();
		current_mesh->index_buffer->Release();
	}


	FOREACH(Dx11_texture_view*, current_tex, textures_list)
	{
		(*current_tex)->Release();
	}

	FOREACH(Vertex_shader, current_vs, vertex_shaders_list){
		current_vs->shader->Release();
		current_vs->input_layout->Release();
	}

	FOREACH(Dx11_pixel_shader*, current_ps, pixel_shaders_list){
		(*current_ps)->Release();
	}

	FOREACH(Dx11_blend_state*, current_blend, blend_states_list){
		(*current_blend)->Release();
	}

	FOREACH(Depth_stencil, current_stencil, depth_stencils_list){
		if(current_stencil->view)
			current_stencil->view->Release();
		if(current_stencil->state)
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