struct VSOutput {
  float4 Position : SV_POSITION;
  [[vk::location(0)]] float2 TexCoord : TEXCOORD0;
};

VSOutput main(uint vertexID : SV_VertexID) {
  float2 positions[4] = {float2(-1, -1), float2(-1, 1), float2(1, -1),
                         float2(1, 1)};

  float2 texCoords[4] = {float2(0, 0), float2(0, 1), float2(1, 0),
                         float2(1, 1)};

  VSOutput output = (VSOutput)0;
  output.Position = float4(positions[vertexID], 0, 1);
  output.TexCoord = texCoords[vertexID];
  return output;
}