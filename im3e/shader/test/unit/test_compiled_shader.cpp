#include <im3e/shader/compiled_shader.h>

#include <im3e/test_utils/test_utils.h>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>

using namespace im3e;

namespace {

// Header of a SPIR-V module: magic number, version 1.5, generator, bound and schema
constexpr std::array<uint32_t, 5U> SpirvHeader{0x07230203U, 0x00010500U, 0U, 1U, 0U};

auto createFakeReflection() -> ShaderReflection
{
    return ShaderReflection{
        .entryPoints{
            ShaderEntryPoint{
                .stage = ShaderStage::Vertex,
                .name = "vertexMain",
            },
            ShaderEntryPoint{
                .stage = ShaderStage::Fragment,
                .name = "fragmentMain",
            },
        },
    };
};

}  // namespace

TEST(CompiledShaderTest, constructor)
{
    const auto reflection = createFakeReflection();
    const CompiledShader shader(SpirvHeader, reflection);

    EXPECT_THAT(shader.getSpirv().data(), Eq(SpirvHeader.data()));
    EXPECT_THAT(shader.getSpirv().size(), Eq(SpirvHeader.size()));
    EXPECT_THAT(shader.getReflection(), Eq(reflection));
}

TEST(CompiledShaderTest, constructor_withEmptyCode)
{
    EXPECT_THROW(CompiledShader(std::span<const uint32_t>(), ShaderReflection{}), std::invalid_argument);
}

TEST(CompiledShaderTest, constructor_withTooShortCode)
{
    EXPECT_THROW(CompiledShader(std::span(SpirvHeader).first(4U), ShaderReflection{}), std::invalid_argument);
}

TEST(CompiledShaderTest, constructor_withWrongMagicNumber)
{
    auto code = SpirvHeader;
    code[0] = 0x03022307U;  // Magic number with the wrong byte order

    EXPECT_THROW(CompiledShader(code, ShaderReflection{}), std::invalid_argument);
}
