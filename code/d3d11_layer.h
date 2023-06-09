#include <d3d11.h>
#include <dxgi1_2.h>
// using namespace DirectX;

#if DEBUGMODE
	#define DX11_CREATE_DEVICE_DEBUG_FLAG D3D11_CREATE_DEVICE_DEBUG
#else
	#define DX11_CREATE_DEVICE_DEBUG_FLAG 0
#endif

typedef ID3D11ShaderResourceView Dx11_texture_view;

struct D3D
{
	ID3D11Device* device;
	ID3D11DeviceContext* context;
	IDXGISwapChain1* swap_chain; // maybe change to IDXGISwapChain1 

	ID3D11RasterizerState* rasterizer_state;
	ID3D11SamplerState* sampler;
	ID3D11RenderTargetView* render_target;
};

