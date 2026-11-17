#pragma once

#include <cstdint>
#include <span>

namespace im3e {

/// SPIR-V code of a shader compiled at build time.
///
/// Refers to the code without copying it: the code must outlive the object, as the code embedded in the binaries by
/// im3e_shader_compiler does.
class CompiledShader
{
public:
    /// Throws std::invalid_argument if the code is shorter than a SPIR-V header or doesn't start with the SPIR-V magic
    /// number.
    explicit CompiledShader(std::span<const uint32_t> spirv);

    /// Words of the SPIR-V code. Its size_bytes() is the code size Vulkan expects.
    [[nodiscard]] auto getSpirv() const -> std::span<const uint32_t> { return m_spirv; }

private:
    std::span<const uint32_t> m_spirv;
};

}  // namespace im3e
