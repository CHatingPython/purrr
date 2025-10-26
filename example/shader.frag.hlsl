struct PSInput {
  float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

struct PSOutput {
  float4 Color : SV_TARGET0;
};

Texture2D uTexture : register(t0);
SamplerState uSampler : register(s0);

PSOutput main(PSInput input) {
  PSOutput output = (PSOutput)0;
  output.Color = uTexture.Sample(uSampler, input.TexCoord);
  return output;
}