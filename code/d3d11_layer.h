//WHY DOES THIS GET PUT INSIDE THE PRECOMPILED HEADER
#include <d3d11.h>
#include <dxgi1_2.h>
#include <D3DCompiler.h>
#include <DirectXMath.h>

using namespace DirectX; //TODO: THIS IS ONLY USED IN directxmath.h

#define PS_PROFILE "ps_5_0"
#define VS_PROFILE "vs_5_0"

#if DEBUGMODE
	#define DX11_CREATE_DEVICE_DEBUG_FLAG D3D11_CREATE_DEVICE_DEBUG
#else
	#define DX11_CREATE_DEVICE_DEBUG_FLAG 0
#endif

typedef ID3D11ShaderResourceView 		Dx11_texture_view;
typedef ID3D11InputLayout 					Dx11_input_layout;
typedef ID3D11Device 						Dx11_device;
typedef ID3D11DeviceContext				Dx11_device_context;
typedef IDXGISwapChain1						Dx11_swap_chain;
typedef ID3D11RasterizerState				Dx11_rasterizer_state;
typedef ID3D11SamplerState					Dx11_sampler_state;
typedef ID3D11RenderTargetView			Dx11_render_target_view;

typedef ID3D11Buffer 						Dx11_buffer;
typedef D3D11_BUFFER_DESC					Dx11_buffer_desc;
typedef D3D11_SUBRESOURCE_DATA			Dx11_subresource_data;

typedef ID3D11VertexShader					Dx11_vertex_shader;
typedef D3D11_INPUT_ELEMENT_DESC			Dx11_input_layout_desc;
typedef ID3D11PixelShader 					Dx11_pixel_shader;

typedef ID3DBlob								Dx11_blob;
typedef ID3D11Resource 						Dx11_resource;

typedef D3D11_SAMPLER_DESC					Dx11_sampler_desc;
typedef D3D11_RASTERIZER_DESC				Dx11_rasterizer_desc;
typedef ID3D11BlendState					Dx11_blend_state;
typedef ID3D11DepthStencilState			Dx11_depth_stencil_state;
typedef D3D11_DEPTH_STENCIL_DESC			Dx11_depth_stencil_desc;
typedef ID3D11ShaderResourceView			Dx11_texture_view;
typedef ID3D11DepthStencilView 			Dx11_depth_stencil_view;
typedef D3D11_VIEWPORT						Dx11_viewport;

struct D3D
{
	Dx11_device* device;
	Dx11_device_context* context;
	Dx11_swap_chain* swap_chain; // maybe change to IDXGISwapChain1 
	Dx11_viewport viewport;

	Dx11_rasterizer_state* rasterizer_state;
	Dx11_sampler_state* sampler;
	Dx11_render_target_view* render_target_view;
};

enum CONSTANT_BUFFER_REGISTER_INDEX
{
	OBJECT_BUFFER_REGISTER_INDEX = 0,
	WORLD_VIEW_BUFFER_REGISTER_INDEX = 1,
	WORLD_PROJECTION_BUFFER_REGISTER_INDEX = 2,
	OBJECT_COLOR_BUFFER_REGISTER_INDEX = 3,
};

struct D3D_constant_buffer
{
	Dx11_buffer* buffer;
	u32 size;
	CONSTANT_BUFFER_REGISTER_INDEX register_index;
};

struct Dx11_render_pipeline
{
	Dx11_vertex_shader* vs;
	Dx11_pixel_shader* ps;
	Dx11_input_layout* input_layout;
	Dx11_blend_state* blend_state;
	Dx11_depth_stencil_state* depth_stencil_state;
	Dx11_depth_stencil_view* depth_stencil_view;

	Dx11_texture_view* default_texture_view; 

	List list;
};

struct Dx_mesh
{
	u32 vertices_count;
	u32 vertex_size;
	u32 indices_count;

	Dx11_buffer* vertex_buffer;
	Dx11_buffer* index_buffer;

	D3D11_PRIMITIVE_TOPOLOGY topology;
};

internal Dx_mesh
OLD_dx11_init_mesh(D3D* dx, Mesh_primitive* data, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	Dx_mesh result = {0};

	// VERTEX BUFFER
	D3D11_BUFFER_DESC bd = {0};
	bd.ByteWidth        = data->vertices_count * data->vertex_size;
	bd.Usage            = D3D11_USAGE_DEFAULT;
	bd.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
	
	D3D11_SUBRESOURCE_DATA init_data = {0};
	init_data.pSysMem            = data->vertices;

	ASSERTHR(dx->device->CreateBuffer( &bd, &init_data, &result.vertex_buffer));

	// INDEX BUFFER
	bd = {0};
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.ByteWidth = data->indices_count*sizeof(u16);
	bd.StructureByteStride = sizeof(u16);

	init_data = {0};
	init_data.pSysMem = data->indices;
	ASSERTHR(dx->device->CreateBuffer(&bd, &init_data, &result.index_buffer));

	result.topology = topology;
	result.vertices_count = data->vertices_count;
	result.vertex_size = data->vertex_size;
	result.indices_count = data->indices_count;
	return result;
}
internal Dx_mesh
dx11_init_mesh(D3D* dx, void* vertices, u32 v_count, int v_size, u16* indices, u32 i_count, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	Dx_mesh result = {0};
	// VERTEX BUFFER
	D3D11_BUFFER_DESC bd = {0};
	bd.ByteWidth        = v_count * v_size;
	bd.Usage            = D3D11_USAGE_DEFAULT;
	bd.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
	
	D3D11_SUBRESOURCE_DATA init_data = {0};
	init_data.pSysMem            = vertices;

	ASSERTHR(dx->device->CreateBuffer( &bd, &init_data, &result.vertex_buffer));

	// INDEX BUFFER
	bd = {0};
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.ByteWidth = i_count*sizeof(u16);
	bd.StructureByteStride = sizeof(u16);
	init_data = {0};
	init_data.pSysMem = indices;
	ASSERTHR(dx->device->CreateBuffer(&bd, &init_data, &result.index_buffer));
	result.topology = topology;
	result.vertices_count = v_count;
	result.vertex_size = v_size;
	result.indices_count = i_count;
	return result;
}
internal Dx_mesh
dx11_init_mesh(D3D* dx, Mesh_primitive* primitives, D3D11_PRIMITIVE_TOPOLOGY topology)
{
	return dx11_init_mesh(dx, 
		primitives->vertices, primitives->vertices_count, primitives->vertex_size, 
		primitives->indices, primitives->indices_count,
		topology
	);
}

internal File_data
dx11_get_compiled_shader(String filename, Memory_arena* arena, char* entrypoint_name, char* target_profile)
{
	File_data source_shaders_file = win_read_file(filename, arena);

	D3D_SHADER_MACRO defines[] = {
		NULL, NULL
	};
	// D3D_FEATURE_LEVEL feature_level = device->GetFeatureLevel();
	u32 shader_compile_flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_ENABLE_STRICTNESS;
	
#if DEBUGMODE
	shader_compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	shader_compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL_3;
#endif
	Dx11_blob* shader_blob = 0;
	Dx11_blob* error_blob = 0;
	HRESULT hr = D3DCompile(
		source_shaders_file.data,
		source_shaders_file.size,
		0, defines,0,
		entrypoint_name, target_profile,
		shader_compile_flags, 0,
		&shader_blob, &error_blob
	);
	if(hr != S_OK){
		OutputDebugStringA((char*)error_blob->GetBufferPointer());
	}
	ASSERTHR(hr);

	File_data result =  {0};
	result.size = shader_blob->GetBufferSize();
	result.data = arena_push_data(arena, shader_blob->GetBufferPointer(), result.size);
	
	if(shader_blob) shader_blob->Release();
	if(error_blob) error_blob->Release();
	return result;
}

internal void
dx11_create_vs(D3D* dx, File_data vs, Dx11_vertex_shader** result)
{
	HRESULT hr = dx->device->CreateVertexShader(
		vs.data,
		vs.size,
		0,
		result
	);
	ASSERTHR(hr);
}

internal void
dx11_create_input_layout(D3D* dx, File_data vs, Dx11_input_layout_desc ied[], s32 ied_count, Dx11_input_layout** result)
{
	HRESULT hr = dx->device->CreateInputLayout(
		ied, ied_count, 
		vs.data, vs.size, 
		result
	); 
	ASSERTHR(hr);
}

internal void
dx11_create_ps(D3D* dx, File_data ps, Dx11_pixel_shader** result)
{
	dx->device->CreatePixelShader(
		ps.data,
  		ps.size,
  		0,
		result
	);
}
// CONSTANT BUFFER
internal void
dx11_create_constant_buffer(
	D3D* dx, D3D_constant_buffer* result, 
	u32 buffer_size, CONSTANT_BUFFER_REGISTER_INDEX register_index, 
	void* initialize)
{
	HRESULT hr = 0;
	D3D11_BUFFER_DESC desc = {0};
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.ByteWidth = buffer_size;
	if(initialize)
	{
		D3D11_SUBRESOURCE_DATA init_data = {0};
		init_data.pSysMem = initialize;
		hr = dx->device->CreateBuffer(&desc, &init_data, &result->buffer);
	}else{
		hr = dx->device->CreateBuffer(&desc, 0, &result->buffer);
	}
	ASSERTHR(hr);
	result->size = buffer_size;
	result->register_index = register_index;
}

// THIS FUNCTION BINDS INTO THE VERTEX SHADER SO I CAN'T ACCESS IT THROUGH THE PS
internal void
dx11_bind_constant_buffer(D3D* dx, D3D_constant_buffer* c)
{
	dx->context->VSSetConstantBuffers(c->register_index, 1, &c->buffer);
}
// THIS FUNCTION BINDS INTO THE VERTEX SHADER SO I CAN'T ACCESS IT THROUGH THE PS
internal void
dx11_create_and_bind_constant_buffer(
	D3D* dx, D3D_constant_buffer* c, 
	u32 buffer_size, CONSTANT_BUFFER_REGISTER_INDEX register_index,
	void* initialize)
{
	dx11_create_constant_buffer(dx, c, buffer_size, register_index, initialize);
	dx11_bind_constant_buffer(dx, c);
}

internal void
dx11_create_sampler(D3D* dx, Dx11_sampler_state** result)
{
	Dx11_sampler_desc desc = {0};
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	// D3D11_TEXTURE_ADDRESS_WRAP
	// D3D11_TEXTURE_ADDRESS_BORDER
	// D3D11_TEXTURE_ADDRESS_CLAMP
	D3D11_TEXTURE_ADDRESS_MODE texture_address_mode =  D3D11_TEXTURE_ADDRESS_BORDER;
	desc.AddressU = texture_address_mode;
	desc.AddressV = texture_address_mode;
	desc.AddressW = texture_address_mode;
	desc.BorderColor[0] = 1.0f;
	desc.BorderColor[1] = 0.0f;
	desc.BorderColor[2] = 1.0f;
	desc.BorderColor[3] = 1.0f;
	HRESULT hr = dx->device->CreateSamplerState(&desc, result);
	ASSERTHR(hr);
}
// fill modes: D3D11_FILL_WIREFRAME / D3D11_FILL_SOLID ;  cull modes: D3D11_CULL_BACK | D3D11_CULL_FRONT | D3D11_CULL_NONE
internal void
dx11_create_rasterizer_state(D3D* dx, Dx11_rasterizer_state** result, D3D11_FILL_MODE fill_mode, D3D11_CULL_MODE cull_mode)
{
	HRESULT hr = {0};
	Dx11_rasterizer_desc desc = {0};
	desc.FillMode = fill_mode;
	desc.CullMode = cull_mode;
	hr = dx->device->CreateRasterizerState(&desc, result);
	ASSERTHR(hr);
}

internal void
dx11_create_blend_state(D3D* dx, Dx11_blend_state** result, bool enable_alpha_blending)
{
	HRESULT hr;
	D3D11_RENDER_TARGET_BLEND_DESC d  = {0};
	d.BlendEnable = enable_alpha_blending;
	d.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	d.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	d.BlendOp = D3D11_BLEND_OP_ADD;
	d.SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	d.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	d.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	d.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;    
	D3D11_BLEND_DESC desc = {0};
	desc.RenderTarget[0] = d;
	hr = dx->device->CreateBlendState(&desc, result); //TODO: AAAAAAAAAA
	ASSERTHR(hr);
}

//TODO: why can i enable it ???
internal void 
dx11_create_depth_stencil_state(D3D* dx, Dx11_depth_stencil_state** result, bool enable)
{
	HRESULT hr = {0};
	D3D11_DEPTH_STENCIL_DESC desc = {0};
	desc.DepthEnable = enable; //TODO: TEST THIS
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	// D3D11_COMPARISON_GREATER/D3D11_COMPARISON_LESS/D3D11_COMPARISON_EQUAL
	desc.DepthFunc = D3D11_COMPARISON_LESS; 
	desc.StencilEnable = false; // define FrontFace and BackFace if i want to enable
	desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
	desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
	// to enable must set this 2
	// desc.FrontFace = ... 
	// desc.BackFace = ...
	hr = dx->device->CreateDepthStencilState(&desc, result);
	ASSERTHR(hr);
}
internal void
dx11_create_depth_stencil_view(D3D* dx, Dx11_depth_stencil_view** result, u32 width, u32 height)
{
    
	// CREATE DEPTH STENCIL TEXTURE
	ID3D11Texture2D* depth_stencil_texture = 0;
	D3D11_TEXTURE2D_DESC dd = {0};
	dd.Width = width;
	dd.Height = height;
	dd.MipLevels = 1;
	dd.ArraySize = 1;
	dd.Format = DXGI_FORMAT_D32_FLOAT;
	dd.SampleDesc = { 1, 0 };
	dd.Usage = D3D11_USAGE_DEFAULT;
	dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dx->device->CreateTexture2D(&dd, 0, &depth_stencil_texture);

	D3D11_DEPTH_STENCIL_VIEW_DESC ddsv = {0};
	ddsv.Format = DXGI_FORMAT_D32_FLOAT;
	ddsv.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	ddsv.Texture2D.MipSlice = 0;
	dx->device->CreateDepthStencilView(depth_stencil_texture, 0, result); //TODO: instead of 0 ddsv
	depth_stencil_texture->Release();
}

internal void
dx11_create_texture_view(D3D* dx, Surface* texture, ID3D11ShaderResourceView** texture_view)
{
	HRESULT hr;
	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = texture->width;
	desc.Height = texture->height;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc = {1, 0};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Dx11_subresource_data init_data = {0};
	init_data.pSysMem = texture->data;
	init_data.SysMemPitch = texture->width * sizeof(u32);
	ID3D11Texture2D* texture2D;
	hr = dx->device->CreateTexture2D(&desc, &init_data, &texture2D);
	ASSERTHR(hr);

	// D3D11_SHADER_RESOURCE_VIEW_DESC srd = {0};
	// srd.Format = desc.Format;
	// srd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	// srd.Texture2D.MostDetailedMip = 0;
	// srd.Texture2D.MipLevels = 1;

	hr = dx->device->CreateShaderResourceView(texture2D, 0, texture_view);//TODO: this had the srd
	ASSERTHR(hr);
	// pDesc to 0 to create a view that accesses the entire resource (using the format the resource was created with)
	texture2D->Release();
}

internal void
dx11_create_render_target_view(D3D* dx, Dx11_render_target_view** result)
{
	HRESULT hr = {0};
	ID3D11Texture2D* back_buffer = {0};
	// hr = dx->swap_chain->GetBuffer(0, __uuidof(ID3D11Resource), (void**)&back_buffer);
	hr = dx->swap_chain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&back_buffer);
	ASSERTHR(hr);

	hr = dx->device->CreateRenderTargetView(back_buffer, 0, result);
	ASSERTHR(hr);
	back_buffer->Release();
}
internal void
dx11_set_viewport(D3D* dx, s32 posx, s32 posy, u32 width, u32 height)
{
	Dx11_viewport vp = {0};
	vp.TopLeftX = (r32)posx;
	vp.TopLeftY = (r32)posy;
	vp.Width = (r32)width;
	vp.Height = (r32)height;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	dx->viewport = vp;
	dx->context->RSSetViewports(1, &vp);
}
// BINDING FUNCTIONS
internal void
dx11_bind_vs(D3D* dx, Dx11_vertex_shader* vertex_shader)
{
	// BINDING SHADERS AND LAYOUT
	dx->context->VSSetShader(vertex_shader, 0, 0);
}
internal void
dx11_bind_ps(D3D* dx, Dx11_pixel_shader* pixel_shader)
{
	dx->context->PSSetShader(pixel_shader, 0, 0);
}
internal void
dx11_bind_input_layout(D3D* dx, Dx11_input_layout* input_layout)
{
	dx->context->IASetInputLayout(input_layout);
}
internal void
dx11_bind_vertex_buffer(D3D* dx, Dx11_buffer* vertex_buffer, u32 sizeof_vertex)
{
	u32 strides = sizeof_vertex; 
	u32 offsets = 0;
	dx->context->IASetVertexBuffers(
		0,
		1,
		&vertex_buffer,
		&strides,
		&offsets
	);
}
// internal void
// dx11_bind_constant_buffer(D3D* dx, D3D_constant_buffer* c)
// {
// 	dx->context->VSSetConstantBuffers(c->register_index, 1, &c->buffer);
// }
internal void
dx11_bind_texture_view(D3D* dx, Dx11_texture_view** texture_view)
{
	dx->context->PSSetShaderResources(0,1, texture_view);
}
internal void
dx11_bind_sampler(D3D* dx, Dx11_sampler_state** sampler)
{
	dx->context->PSSetSamplers(0, 1, sampler);
}
internal void
dx11_bind_blend_state(D3D* dx, ID3D11BlendState* blend_state)
{
	dx->context->OMSetBlendState(blend_state, 0, ~0U);   
}
internal void
dx11_bind_rasterizer_state(D3D* dx, Dx11_rasterizer_state* rasterizer_state)
{
dx->context->RSSetState(rasterizer_state);
}
internal void
dx11_bind_depth_stencil_state(D3D* dx, Dx11_depth_stencil_state* depth_stencil_state)
{
	dx->context->OMSetDepthStencilState(depth_stencil_state, 0);
}
internal void // this needs the depth_stencil_view already created
dx11_bind_render_target_view(D3D* dx, Dx11_render_target_view** render_target_view, Dx11_depth_stencil_view* depth_stencil_view)
{
	//TODO: i think this fixed it
	dx->context->OMSetRenderTargets(1, render_target_view, depth_stencil_view); 
}

internal void
dx11_modify_resource(D3D* dx, Dx11_resource* resource, void* data, u32 size)
{
	D3D11_MAPPED_SUBRESOURCE mapped_resource = {0};
	// disable gpu access 
	dx->context->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
	// update here
	copy_mem(data, mapped_resource.pData, size);
	// re-enable gpu access

	dx->context->Unmap(resource, 0);
}

internal void
dx11_draw_mesh(
	D3D* dx, Dx11_render_pipeline* pipeline, Dx11_buffer* object_buffer, 
	Dx_mesh* mesh, Dx11_texture_view** texture, XMMATRIX* matrix)
{
	dx11_modify_resource(dx, object_buffer, matrix, sizeof(*matrix));
		
	// Input Assembler STAGE
	dx11_bind_input_layout(dx, pipeline->input_layout);
	dx->context->IASetPrimitiveTopology( mesh->topology );
	dx11_bind_vertex_buffer(dx, mesh->vertex_buffer, mesh->vertex_size);
	// VERTEX SHADER STAGE
	dx11_bind_vs(dx, pipeline->vs);
	// RASTERIZER STAGE
	// PIXEL SHADER STAGE
	dx11_bind_texture_view(dx, texture);
	dx11_bind_ps(dx, pipeline->ps);
	// OUTPUT MERGER
	dx11_bind_blend_state(dx, pipeline->blend_state);
	dx11_bind_depth_stencil_state(dx, pipeline->depth_stencil_state);
	dx11_bind_render_target_view(dx, &dx->render_target_view, pipeline->depth_stencil_view);

	dx->context->IASetIndexBuffer(mesh->index_buffer, DXGI_FORMAT_R16_UINT, 0);
	// FINALLY DRAW
	dx->context->DrawIndexed(mesh->indices_count, 0, 0);
}

