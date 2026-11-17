#include "test_constants.slang.h"

#include <im3e/shader/compiled_shader.h>
#include <im3e/test_utils/test_utils.h>

#include <cstdint>

using namespace im3e;

namespace {

// First word of every SPIR-V module, from the SPIR-V specification ("Magic Number")
constexpr uint32_t SpirvMagicNumber = 0x07230203U;

}  // namespace

TEST(ConstantsTest, compilesShaderImportingConstants)
{
    const auto spirv = shaders::getTestConstantsShader().getSpirv();

    ASSERT_THAT(spirv.empty(), IsFalse());
    EXPECT_THAT(spirv[0], Eq(SpirvMagicNumber));
}
