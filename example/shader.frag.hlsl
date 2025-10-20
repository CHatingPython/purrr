struct PSInput {
  float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

struct PSOutput {
  float4 Color : SV_TARGET0;
};

[[vk::binding(0, 0)]] Texture2D uTexture : register(t0);
[[vk::binding(0, 0)]] SamplerState uSampler : register(s0);

PSOutput main(PSInput input) {
  float4 sampled = uTexture.Sample(uSampler, input.TexCoord);

  PSOutput output = (PSOutput)0;
  output.Color = float4(lerp(float3(0.09, 0.09, 0.09), sampled.rgb, sampled.a), 1);
  return output;
}