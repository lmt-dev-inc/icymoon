// Shader compiled by the tests of im3e_shader: it imports the shared module im3e.shader.constants, found through the
// import paths of the libraries test_im3e_shader links. Pi is used in the output, so the import can't be dropped.

import im3e.shader.constants;

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return float4(Pi / 4.0, 0.0, 0.0, 1.0);
}
