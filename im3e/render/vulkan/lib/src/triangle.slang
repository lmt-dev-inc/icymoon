// Triangle drawn by the frame pipeline, with its vertices hard-coded in the vertex shader: it is drawn with
// vkCmdDraw(3, 1, 0, 0), without vertex input. Positions are in Vulkan clip space, where +Y points down, and are listed
// counter-clockwise as seen on screen, so that the triangle is a front face with VK_FRONT_FACE_COUNTER_CLOCKWISE.

static const float2 Positions[3] = {
    float2(0.0, -0.5),
    float2(-0.5, 0.5),
    float2(0.5, 0.5),
};

static const float4 Color = float4(1.0, 0.5, 0.0, 1.0);

[shader("vertex")]
float4 vertexMain(uint vertexId: SV_VulkanVertexID) : SV_Position
{
    return float4(Positions[vertexId], 0.0, 1.0);
}

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return Color;
}
