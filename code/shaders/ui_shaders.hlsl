
cbuffer object_buffer : register(b0)
{
	matrix object_transform;
}

cbuffer object_color : register(b3)
{
	float4 object_color;
}

cbuffer Constants
{
	float2 ScreenDimensions : RENDER_TARGET_DIMENSIONS;
}


struct VSINPUT
{
	float3 vertex_pos : POSITION;
	float2 texcoord : TEXCOORD;
};

struct PSINPUT
{
	float4 vertex_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float4 color : COLOR0;
};

PSINPUT vs(VSINPUT input){
	PSINPUT result;

	result.vertex_pos = mul(object_transform , float4(input.vertex_pos, 1));
	result.color = object_color;
	result.texcoord = input.texcoord;

	return result;
}


Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

float4 ps(PSINPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = texture0.Sample( sampler0, input.texcoord );

	float4 result = float4(
		texcolor.r * input.color.r, 
		texcolor.g * input.color.g,
		texcolor.b * input.color.b,
		texcolor.a * input.color.a);
	return result;
}