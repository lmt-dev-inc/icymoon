#pragma once

#include <optional>
#include <ostream>
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

    auto operator==(const PushConstantRange&) const -> bool = default;
};

/// Function of a shader that a pipeline can run, as declared in its Slang file.
struct ShaderEntryPoint
{
    ShaderStage stage{};                             ///< Stage the entry point runs in.
    std::string name;                                ///< Name of the entry point.
    std::optional<PushConstantRange> pushConstants;  ///< Push constants of the entry point, if it has any.

    auto operator==(const ShaderEntryPoint&) const -> bool = default;
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

    auto operator==(const ShaderReflection&) const -> bool = default;
};

auto operator<<(std::ostream& stream, ShaderStage stage) -> std::ostream&;
auto operator<<(std::ostream& stream, const PushConstantRange& range) -> std::ostream&;
auto operator<<(std::ostream& stream, const ShaderEntryPoint& entryPoint) -> std::ostream&;
auto operator<<(std::ostream& stream, const ShaderReflection& reflection) -> std::ostream&;

}  // namespace im3e