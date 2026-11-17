// Shader of the incremental build test that imports nothing: it must not compile again when the module changes.

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return float4(1.0, 0.0, 0.0, 1.0);
}
