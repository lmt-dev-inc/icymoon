// Module imported by test_shader.slang, so that the tests of im3e_shader_compiler cover imports and the dependency file.
// It has no entry points: the tool also uses it to test that compiling a module without entry points fails.

public float3 getVertexColor(uint vertexId)
{
    static const float3 Colors[3] = { float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0) };
    return Colors[vertexId];
}
