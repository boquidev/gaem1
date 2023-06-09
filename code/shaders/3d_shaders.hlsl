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

struct VSINPUT
{
	float3 pos : POSITION;
	float2 uv : TEXCOORD;
};

struct VSOUTPUT
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VSOUTPUT vs( VSINPUT input )
{
	VSOUTPUT result;
	result.pos =  mul(transform, float4( input.pos, 1.0f ) );
	// result.pos = mul( world_projection, mul( world_view, mul(transform, float4( input.pos, 1.0f ) ) ) );
	result.uv = input.uv;
	return result;
}

// PIXEL SHADER

Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

float4 ps( VSOUTPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = texture0.Sample( sampler0, input.uv );
	
	float4 result = float4(
		texcolor.r, 
		texcolor.g,
		texcolor.b,
		texcolor.a);
	return result;
}