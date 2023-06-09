// VERTEX SHADER

cbuffer object_buffer : register(b0)
{
	matrix transform; 
};
cbuffer world_view_buffer : register(b1)
{
	matrix world_view;
};
cbuffer world_projection_buffer : register(b2)
{
	matrix world_projection;
};
cbuffer object_color_buffer : register(b3)
{
	float4 object_color;
};

cbuffer Constants
{
	float2 ScreenDimensions : RENDER_TARGET_DIMENSIONS;
}

struct VSINPUT
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct PSINPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float4 color : COLOR0;
};

PSINPUT vs( VSINPUT input )
{
	PSINPUT result;
	float4 vertex_world_pos = mul(transform, float4( input.pos, 1.0f ) );
	// vertex_world_pos.y += sin(vertex_world_pos.x*100)/100;
	float4 pos_from_camera = mul( world_view, vertex_world_pos);
	result.pos = mul( world_projection, pos_from_camera );
	result.uv = input.uv;
	result.color = object_color;
	// result.color = float4(
	// 	vertex_world_pos.xyz, 1
	// );
	// result.color = object_color;
	return result;
}

// PIXEL SHADER

Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

float4 ps( PSINPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = texture0.Sample( sampler0, input.uv );
	float a = sin(input.color.r*100);
	// float4 result = float4(1,1,1,1);
	float4 result = float4(
		texcolor.r * input.color.r, 
		texcolor.g * input.color.g,
		texcolor.b * input.color.b,
		texcolor.a * input.color.a);
	return result;
}