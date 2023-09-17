cbuffer Constant : register(b1)
{
   float time;
   // float outer_border;
   // float inner_border;
};

struct PS_IN
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


struct PS_OUT
{
	float4 color : SV_Target0;
	float4 depth : SV_Target1;
};

PS_OUT ps( PS_IN input, uint tid : SV_PrimitiveID)
{
   PS_OUT result;
   float2 vector_from_center = float2((input.texcoord.x*2)-1, (input.texcoord.y*2)-1);
   float distance_from_center = length(vector_from_center);

   //TODO: add a way to modify this along with the time
   float border_thickness= 0.6f;
   
   clip(1-distance_from_center);

   //TODO: add a time multiplier so that i can accelerate/slow or invert the wave
   float inner_border = time-floor(time);

   float temp = distance_from_center-inner_border;
   temp = temp-floor(temp);

   clip(border_thickness-temp);

   float half_border_thickness = border_thickness/2;
      

   float4 texcolor = texture0.Sample(sampler0, input.texcoord);
   result.color = float4(
      texcolor.r * input.color.r,
      texcolor.g * input.color.g,
      texcolor.b * input.color.b,
      1-abs(half_border_thickness-temp)/half_border_thickness);

      
	float pixel_value = 1-(length(input.vertex_world_pos-input.camera_world_pos.xyz)/100);
	result.depth = input.camera_world_pos.w *float4(pixel_value, pixel_value, pixel_value, 1);
   
   
   return result;
}