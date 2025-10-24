// Translated from: https://www.shadertoy.com/view/WljBDG

struct PSInput {
  float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

struct PSOutput {
  float4 Color : SV_TARGET0;
};

cbuffer ubo { float time; }

PSOutput main(PSInput input) {
  float3 o = -3.1416 * float3(0., .5, 1.);

  float g = input.TexCoord.y + time;
  float3 col = .5 + .5 * -sin(g) * cos(g + o);

  col.g += .25;
  col = .5 + (col * 2. - 1.);
  col.gb *= float2(.75, .9);
  col = .125 + .75 * col;

  PSOutput output = (PSOutput)0;
  output.Color = float4(col, 1);
  return output;
}