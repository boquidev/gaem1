// VERTEX SHADER

cbuffer object_buffer : register(b0)
{
	matrix object_transform; 
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
	float3 vertex_pos : POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
};

struct PSINPUT
{
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
	float4 color : COLOR0;
};

PSINPUT vs( VSINPUT input )
{
	PSINPUT result;
	float4 vertex_world_pos = mul(object_transform, float4( input.vertex_pos, 1.0f ) );
	// vertex_world_pos.y += sin(vertex_world_pos.x*100)/100;
	float4 pos_from_camera = mul( world_view, vertex_world_pos);
	result.pixel_pos = mul( world_projection, pos_from_camera );
	result.texcoord = input.texcoord;
	result.color = object_color;	
	result.normal = normalize( mul( (float3x3)object_transform , input.normal) );
	return result;
}

// PIXEL SHADER

Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

float4 ps( PSINPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = texture0.Sample( sampler0, input.texcoord );
	// float a = sin(input.color.x*500);
	// float4 result = float4(1,1,1,1);
	float4 result = float4(
		texcolor.r * input.color.r, 
		texcolor.g * input.color.g,
		texcolor.b * input.color.b,
		texcolor.a * input.color.a);
		 
	result.r = ((input.normal.y*1.5)+input.normal.z + input.normal.x)*0.6 * (0.5+result.r);
	result.g = (((input.normal.y*1.5)+input.normal.z + (input.normal.x))*0.4) * (0.5+result.g);
	result.b = (-input.normal.x/3);

	// result.r = (input.normal.z*input.normal.y*input.normal.y*2)+((input.normal.y)+input.normal.z + input.normal.x)*0.6 * (0.5+result.r);
	// result.g = (input.normal.z*input.normal.y*input.normal.y*2)+(((input.normal.y)+input.normal.z + (input.normal.x))*0.4) * (0.5+result.g);
	// result.b = (-input.normal.x/4);

	// result.r = (input.normal.y+input.normal.z + input.normal.x*0.5) * (0.5+result.r);
	// result.g = (input.normal.y+input.normal.z + input.normal.x*0.5) * (0.5+result.g);
	// result.b = (-input.normal.x)+(input.normal.y+input.normal.z + input.normal.x*0.5) * (0.5+result.b);; 

	// result.r = (input.normal.z*input.normal.y*input.normal.y)+((input.normal.y+input.normal.z)*2 + input.normal.x)*0.6 * (0.5+result.r);
	// result.g = (input.normal.z*input.normal.y*input.normal.y)+(((input.normal.y+input.normal.z)*2 + (input.normal.x))*0.4) * (0.5+result.g);
	// result.b = (-input.normal.x/2) + (input.normal.z*input.normal.z*input.normal.z*0.5+input.normal.y*input.normal.y*input.normal.y);

	// result.r = (input.normal.z*input.normal.y*input.normal.y)+((input.normal.y*2)+input.normal.z + input.normal.x)*0.6 * (0.5+result.r);
	// result.g = (input.normal.z*input.normal.y*input.normal.y)+(((input.normal.y*2)+input.normal.z + (input.normal.x))*0.4) * (0.5+result.g);
	// result.b = (-input.normal.x/2) + (input.normal.z*input.normal.z*input.normal.z*0.5+input.normal.y*input.normal.y*input.normal.y);
	
	// result.r = (input.normal.z*input.normal.y*input.normal.y*2)+((input.normal.y)+input.normal.z + input.normal.x)*0.6 * (0.5+result.r);
	// result.g = (input.normal.z*input.normal.y*input.normal.y*2)+(((input.normal.y)+input.normal.z + (input.normal.x))*0.4) * (0.5+result.g);
	// result.b = (-input.normal.x/2) +(input.normal.z*input.normal.z*input.normal.y*input.normal.y*2);

	// result.r = (input.normal.z*input.normal.y*input.normal.y)+((input.normal.y)+input.normal.z + input.normal.x)*0.6 * (0.5+result.r);
	// result.g = (input.normal.z*input.normal.y*input.normal.y)+(((input.normal.y)+input.normal.z + (input.normal.x))*0.4) * (0.5+result.g);
	// result.b = (-input.normal.x/2) +(input.normal.z*input.normal.z*input.normal.y*input.normal.y*2);
	
	
	return result;
}