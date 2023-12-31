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
// move this value to the pixel constant buffers
cbuffer camera_pos_buffer : register(b5){
	float4 camera_pos;
}

struct VSINPUT
{
	float3 vertex_pos : POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
};

struct PS_IN
{
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
	float4 color : COLOR0;

	float3 vertex_world_pos : COLOR1;
	
	// w value of camera is 1 or 0 depending on if perspective is on or off
	// it's kind of dumb and that calculation should be done just once in the cpu
	float4 camera_world_pos : COLOR2;
};

PS_IN vs( VSINPUT input )
{
	PS_IN result;

	float4 vertex_world_pos = mul(object_transform, float4( input.vertex_pos, 1.0f ) );
	float4 pos_from_camera = mul( world_view, vertex_world_pos);
	result.pixel_pos = mul( world_projection, pos_from_camera );
	// a little funny deformation
	// vertex_world_pos.y += 5*sin(vertex_world_pos.x*10)/10;

	result.texcoord.x = object_texrect.x + (input.texcoord.x * object_texrect.z);
	result.texcoord.y = object_texrect.y + (input.texcoord.y * object_texrect.w);
	result.color = object_color;	
	result.normal = mul( (float3x3)object_transform , input.normal);
	float4x4 world_rotation = world_view;

	// maybe get this from cpu
	// result.camera_world_pos.xyz = normalize(mul(float4(0,0,1,1), world_rotation).xyz);
	// result.camera_world_pos.xyz = lerp(result.camera_world_pos.xyz, camera_pos.xyz, camera_pos.w);
	// result.camera_world_pos.w = camera_pos.w;
	result.camera_world_pos = camera_pos;

	result.vertex_world_pos = vertex_world_pos.xyz;
	return result;
}

// PIXEL SHADER

Texture2D<float4> texture0 : register(t0);

sampler sampler0 : register(s0);

// Output color information to the first render target (e.g., RT0)
// float4 ColorOutput : SV_Target0;
// Output depth information to the second render target (e.g., RT1)
// float DepthOutput : SV_Target1;

struct PS_OUT
{
	float4 color : SV_Target0;
	float4 depth : SV_Target1;
};

PS_OUT ps( PS_IN input, uint tid : SV_PrimitiveID)
{
	PS_OUT result;
	float4 texcolor = texture0.Sample( sampler0, input.texcoord );
	float result_alpha = texcolor.a*input.color.a;
	clip(result_alpha-0.0001f);
	result.color = float4(
		texcolor.r * (0.5+input.color.r)/1.5, 
		texcolor.g * (0.5+input.color.g)/1.5,
		texcolor.b * (0.5+input.color.b)/1.5,
		result_alpha);

	float3 N = normalize(input.normal);
	result.color.rgb *=   ( (N.x+N.y+N.z+3)/3 );
	float3 camera_to_vertex = normalize(input.vertex_world_pos - input.camera_world_pos.xyz);
	// float3 camera_vector = lerp(input.camera_world_pos.xyz, camera_to_vertex, input.camera_world_pos.w);
	float3 camera_vector = camera_to_vertex;

	float fresnel = 1.5 + dot(camera_vector, N);

	result.color.rgb *= fresnel;
	

	float pixel_value = 1-(length(input.vertex_world_pos-input.camera_world_pos.xyz)/100);

	// I HAVE NO IDEA WHAT THIS MULTIPLICATION IS DOING
	result.depth = input.camera_world_pos.w * float4(pixel_value, pixel_value, pixel_value, 1);

	// result.depth = float4(1,1,1,1);

	// get this value from a constant buffer
	float FRESNEL_MULTIPLIER = 20;
	float clip_mask = saturate(FRESNEL_MULTIPLIER*(fresnel));
	result.color.rgb *= clip_mask;
	
	return result;
}