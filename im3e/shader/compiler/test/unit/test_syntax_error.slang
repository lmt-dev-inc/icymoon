// Shader with a syntax error, compiled by the tests of im3e_shader_compiler to check that Slang's error is printed and
// that the tool fails.

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0)
}
