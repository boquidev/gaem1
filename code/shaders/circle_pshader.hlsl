
struct PSINPUT
{
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 normal : NORMAL;
	float4 color : COLOR0;

	float3 vertex_world_pos : COLOR1;
	float4 camera_world_pos : COLOR2;
};


Texture2D<float4> texture0 : register(t0);
sampler sampler0 : register(s0);

float4 ps( PSINPUT input, uint tid : SV_PrimitiveID) : SV_TARGET
{
   float4 result;
   float2 vector_from_center = float2((input.texcoord.x*2)-1, (input.texcoord.y*2)-1);
   clip(1-length(vector_from_center));

   float4 texcolor = texture0.Sample(sampler0, input.texcoord);
   result = float4(
      texcolor.r * input.color.r,
      texcolor.g * input.color.g,
      texcolor.b * input.color.b,
      texcolor.a * input.color.a);
   
   return result;
}