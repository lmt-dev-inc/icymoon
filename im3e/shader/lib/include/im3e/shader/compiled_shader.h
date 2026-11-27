#pragma once

#include <im3e/shader/shader_reflection.h>

#include <cstdint>
#include <span>

namespace im3e {

/// SPIR-V code of a shader compiled at build time, with its reflection data.
///
/// Refers to the code without copying it: the code must outlive the object, as the code embedded in the binaries by
/// im3e_shader_compiler does. The reflection data is copied.
class CompiledShader
{
public:
    /// Throws std::invalid_argument if the code is shorter than a SPIR-V header or doesn't start with the SPIR-V magic
    /// number.
    CompiledShader(std::span<const uint32_t> spirv, ShaderReflection reflection);

    /// Words of the SPIR-V code. Its size_bytes() is the code size Vulkan expects.
    [[nodiscard]] auto getSpirv() const -> std::span<const uint32_t> { return m_spirv; }

    /// Entry points and push constants of the shader, from which pipelines are created.
    [[nodiscard]] auto getReflection() const -> const ShaderReflection& { return m_reflection; }

private:
    std::span<const uint32_t> m_spirv;
    ShaderReflection m_reflection;
};

}  // namespace im3e
