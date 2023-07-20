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
cbuffer object_texrect_buffer : register(b4){
	float4 object_texrect;
}
cbuffer camera_pos_buffer : register(b5){
	float4 camera_pos;
}

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

	float3 vertex_world_pos : COLOR1;
	float3 camera_world_pos : COLOR2;
};

PSINPUT vs( VSINPUT input )
{
	PSINPUT result;

	float4 vertex_world_pos = mul(object_transform, float4( input.vertex_pos, 1.0f ) );
	float4 pos_from_camera = mul( world_view, vertex_world_pos);
	result.pixel_pos = mul( world_projection, pos_from_camera );
	// a little funny deformation
	// vertex_world_pos.y += 5*sin(vertex_world_pos.x*10)/10;

	result.texcoord.x = object_texrect.x + (input.texcoord.x * object_texrect.z);
	result.texcoord.y = object_texrect.y + (input.texcoord.y * object_texrect.w);
	result.color = object_color;	
	result.normal = normalize( mul( (float3x3)object_transform , input.normal) );
	result.camera_world_pos = camera_pos.xyz;
	result.vertex_world_pos = vertex_world_pos.xyz;
	return result;
}

// PIXEL SHADER

Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

float4 ps( PSINPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
	float4 texcolor = texture0.Sample( sampler0, input.texcoord );
	float result_alpha = texcolor.a*input.color.a;
	clip(result_alpha-0.0001f);
	// float a = sin(input.color.x*500);
	// float4 result = float4(1,1,1,1);
	float4 result = float4(
		texcolor.r * (0.5+input.color.r)/1.5, 
		texcolor.g * (0.5+input.color.g)/1.5,
		texcolor.b * (0.5+input.color.b)/1.5,
		result_alpha);

	// clip(0.5-abs(input.normal.x));
	result.rgb =   ( (input.normal.x+input.normal.y+input.normal.z+3)/3 ) * result.rgb;
	float3 camera_vector = normalize(input.vertex_world_pos - input.camera_world_pos);

	float fresnel = 1.5+ dot(camera_vector, input.normal);

	result.rgb = result.rgb * fresnel;

	// weird outline (just works in perspective view and with smooth meshes)
	float clip_mask = floor(2.3-fresnel);
	result.rgb = result.rgb * clip_mask;
	
	return result;
}