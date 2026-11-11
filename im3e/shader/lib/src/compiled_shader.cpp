#include <im3e/shader/compiled_shader.h>

#include <im3e/utils/core/throw_utils.h>

#include <stdexcept>

using namespace im3e;

namespace {

// Magic number, version, generator, bound and schema (SPIR-V specification, "Physical Layout of a SPIR-V Module")
constexpr std::size_t SpirvHeaderWordCount = 5U;
constexpr std::uint32_t SpirvMagicNumber = 0x07230203U;

}  // namespace

CompiledShader::CompiledShader(std::span<const std::uint32_t> spirv)
  : m_spirv(spirv)
{
    throwIfFalse<std::invalid_argument>(m_spirv.size() >= SpirvHeaderWordCount,
                                        "Shader code is shorter than a SPIR-V header");
    throwIfFalse<std::invalid_argument>(m_spirv.front() == SpirvMagicNumber,
                                        "Shader code doesn't start with the SPIR-V magic number");
}
