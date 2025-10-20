struct VSInput {
  [[vk::location(0)]] float2 Position : POSITION0;
};

struct VSOutput {
	float4 Pos : SV_POSITION;
};

VSOutput main(VSInput input) {
	VSOutput output = (VSOutput)0;
	output.Pos = float4(input.Position, 0, 1);
	return output;
}