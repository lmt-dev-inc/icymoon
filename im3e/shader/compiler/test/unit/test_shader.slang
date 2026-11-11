// Shader compiled by the tests of im3e_shader_compiler: a triangle with its vertices in the vertex shader, and two
// entry points in the same file.

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color : COLOR;
};

static const float2 Positions[3] = { float2(0.0, -0.5), float2(0.5, 0.5), float2(-0.5, 0.5) };
static const float3 Colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };

[shader("vertex")]
VertexOutput vertexMain(uint vertexId: SV_VertexID)
{
    VertexOutput output;
    output.position = float4(Positions[vertexId], 0.0, 1.0);
    output.color = Colors[vertexId];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return float4(input.color, 1.0);
}
