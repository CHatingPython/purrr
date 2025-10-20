struct PSInput {
  [[vk::location(0)]] float4 Position : SV_POSITION;
};

struct PSOutput {
  float4 Color : SV_TARGET0;
};

PSOutput main(PSInput input) {
  PSOutput output = (PSOutput)0;
  output.Color = float4(1, 0, 0, 1);
  return output;
}