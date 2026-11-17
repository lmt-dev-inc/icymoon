// Shader of the incremental build test that imports a module: it must compile again when the module changes.

import imported_module;

[shader("fragment")]
float4 fragmentMain() : SV_Target
{
    return getColor();
}
