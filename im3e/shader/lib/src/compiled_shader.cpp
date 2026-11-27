#include <im3e/shader/compiled_shader.h>

#include <im3e/utils/core/throw_utils.h>

#include <stdexcept>
#include <utility>

using namespace im3e;

namespace {

void throwIfSpirvInvalid(const std::span<const uint32_t>& spirv)
{
    // Magic number, version, generator, bound and schema (SPIR-V specification, "Physical Layout of a SPIR-V Module")
    constexpr std::size_t SpirvHeaderWordCount = 5U;
    constexpr uint32_t SpirvMagicNumber = 0x07230203U;

    throwIfFalse<std::invalid_argument>(spirv.size() >= SpirvHeaderWordCount,
                                        "Shader code is shorter than a SPIR-V header");
    throwIfFalse<std::invalid_argument>(spirv.front() == SpirvMagicNumber,
                                        "Shader code doesn't start with the SPIR-V magic number");
}

}  // namespace

CompiledShader::CompiledShader(std::span<const uint32_t> spirv, ShaderReflection reflection)
  : m_spirv(spirv)
  , m_reflection(std::move(reflection))
{
    throwIfSpirvInvalid(m_spirv);
}
