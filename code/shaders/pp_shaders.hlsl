
cbuffer Constants : register(b0)
{
	float4 ScreenDimensions;
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
};


PSINPUT vs( VSINPUT input )
{
   PSINPUT result;
   result.pixel_pos = float4(input.vertex_pos, 1.0f);
   result.texcoord = input.texcoord;
   return result;
}



// Sobel filter coefficients for horizontal and vertical directions
static const float3x3 SobelHorizontal =
{
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

static const float3x3 SobelVertical =
{
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};


// PIXEL SHADER

Texture2D<float4> color_texture : register(t0);
Texture2D<float4> depth_texture : register(t1);

sampler sampler0 : register(s0);

float4 ps( PSINPUT input, uint tid : SV_PrimitiveID ) : SV_TARGET
{
   float4 result;

   // Initialize gradients
   float3 gradientX = 0;
   float3 gradientY = 0;

   // Sample the texture multiple times using the Sobel filter
   for (int i = -1; i <= 1; ++i)
   {
      for (int j = -1; j <= 1; ++j)
      {
         float2 sample_coord = input.texcoord + float2(i, j) / ScreenDimensions.xy;

         // Sample the input texture
         float4 tex_color = depth_texture.Sample(sampler0, sample_coord);

         // Apply the Sobel filter to calculate gradients
         gradientX += tex_color.rgb * SobelHorizontal[i + 1][j + 1];
         gradientY += tex_color.rgb * SobelVertical[i + 1][j + 1];
      }
   }

   // Calculate the magnitude of the gradient
   float magnitude = length(gradientX.rgb) + length(gradientY.rgb);

   // You can adjust the threshold to control the edge detection sensitivity
   float threshold = 0.5f; // Adjust as needed
   

   float min_value = 0.9f;
   float dif = 0.1f;
   float multiplier = 1.0f/dif; 
   

   float interpolator = saturate(multiplier*(magnitude-min_value));

   // clip(magnitude-threshold);


   // Use the magnitude and threshold to create a binary edge map
   float4 original_color = color_texture.Sample(sampler0, input.texcoord);

   result = lerp(original_color, float4(0,0,0,1), interpolator);
   // result = float4(interpolator, 0,0,1);



   // result = color_texture.Sample( sampler0, input.texcoord);
   // result = depth_texture.Sample( sampler0, input.texcoord );

   return result;
}

/*
Texture2D<float4> InputTexture : register(t0); // Input texture
float2 TextureSize; // Size of the input texture

SamplerState PointSampler : register(s0); // Sampler state

// Sobel filter coefficients for horizontal and vertical directions
static const float3x3 SobelHorizontal =
{
    -1, 0, 1,
    -2, 0, 2,
    -1, 0, 1
};

static const float3x3 SobelVertical =
{
    -1, -2, -1,
     0,  0,  0,
     1,  2,  1
};

struct PS_OUTPUT
{
   float4 Color : SV_Target;
};

PS_OUTPUT main(float4 position : SV_POSITION, float2 texCoord : TEXCOORD0) : PS_OUTPUT
{
   PS_OUTPUT output;

   // Initialize gradients
   float3 gradientX = 0;
   float3 gradientY = 0;

   // Sample the texture multiple times using the Sobel filter
   for (int i = -1; i <= 1; ++i)
   {
      for (int j = -1; j <= 1; ++j)
      {
         float2 sampleCoord = texCoord + float2(i, j) / TextureSize;

         // Sample the input texture
         float4 texColor = InputTexture.SampleLevel(PointSampler, sampleCoord, 0);

         // Apply the Sobel filter to calculate gradients
         gradientX += texColor.rgb * SobelHorizontal[i + 1][j + 1];
         gradientY += texColor.rgb * SobelVertical[i + 1][j + 1];
      }
   }

   // Calculate the magnitude of the gradient
   float magnitude = length(gradientX.rgb) + length(gradientY.rgb);

   // You can adjust the threshold to control the edge detection sensitivity
   float threshold = 0.5; // Adjust as needed

   // Use the magnitude and threshold to create a binary edge map
   output.Color = (magnitude > threshold) ? float4(1, 1, 1, 1) : float4(0, 0, 0, 1);

   return output;
}

*/