struct VSInput {
  [[vk::location(0)]] float2 Position : POSITION0;
  [[vk::location(1)]] float2 TexCoord : TEXCOORD0;
};

struct VSOutput {
  float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

VSOutput main(VSInput input) {
  VSOutput output = (VSOutput)0;
  output.Position = float4(input.Position, 0, 1);
  output.TexCoord = input.TexCoord;
  return output;
}