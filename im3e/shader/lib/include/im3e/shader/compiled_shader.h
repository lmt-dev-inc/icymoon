#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace im3e {

/// Pipeline stage that an entry point runs in.
enum class ShaderStage
{
    Vertex,
    Fragment,
    Compute
};

/// Part of the push constant memory that an entry point reads.
struct PushConstantRange
{
    uint32_t offset{};  ///< Start of the range, in bytes from the start of the push constant memory.
    uint32_t size{};    ///< Size of the range, in bytes.
};

/// Function of a shader that a pipeline can run, as declared in its Slang file.
struct ShaderEntryPoint
{
    ShaderStage stage{};                             ///< Stage the entry point runs in.
    std::string name;                                ///< Name of the entry point.
    std::optional<PushConstantRange> pushConstants;  ///< Push constants of the entry point, if it has any.
};

/// What pipeline creation needs to know about a shader.
///
/// Extracted by im3e_shader_compiler with the Slang reflection API, and stored as plain data, with no dependency on
/// Slang or on a graphics API.
struct ShaderReflection
{
    /// Entry points of the shader, in the order of its Slang file. Several entry points can have the same stage, e.g.
    /// the compute kernels of one file: the code creating a pipeline chooses the ones it uses.
    std::vector<ShaderEntryPoint> entryPoints;
};

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
