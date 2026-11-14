#include "test_shader.h"

#include <im3e/shader/compiled_shader.h>
#include <im3e/test_utils/test_utils.h>
#include <im3e/utils/core/path_utils.h>

#include <fmt/format.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <regex>
#include <span>
#include <string>
#include <string_view>

using namespace im3e;

namespace {

// Execution models and opcode from the SPIR-V specification ("Binary Form")
constexpr uint32_t ExecutionModelVertex = 0U;
constexpr uint32_t ExecutionModelFragment = 4U;
constexpr uint32_t OpEntryPoint = 15U;
constexpr std::size_t SpirvHeaderWordCount = 5U;

/// Reads the OpEntryPoint instructions of a SPIR-V module, as their names and execution models.
///
/// Each instruction starts with a word holding its word count in the high 16 bits and its opcode in the low 16 bits.
/// OpEntryPoint is followed by the execution model, the id of the function, then the name as a null-terminated string
/// packed in words.
auto getEntryPoints(std::span<const uint32_t> spirv) -> std::map<std::string, uint32_t>
{
    std::map<std::string, uint32_t> entryPoints;
    auto offset = SpirvHeaderWordCount;
    while (offset < spirv.size())
    {
        const auto wordCount = spirv[offset] >> 16U;
        const auto opcode = spirv[offset] & 0xFFFFU;
        if (wordCount == 0U || offset + wordCount > spirv.size())
        {
            ADD_FAILURE() << "Invalid SPIR-V instruction at word " << offset;
            break;
        }
        if (opcode == OpEntryPoint)
        {
            const auto nameWords = spirv.subspan(offset + 3U, wordCount - 3U);
            const std::string_view nameBytes(reinterpret_cast<const char*>(nameWords.data()), nameWords.size_bytes());
            entryPoints[std::string(nameBytes.substr(0U, nameBytes.find('\0')))] = spirv[offset + 1U];
        }
        offset += wordCount;
    }
    return entryPoints;
}

/// Escapes spaces as the tool does in the dependency file. The tool also escapes "#" and "$", which aren't expected in
/// the folders of the tests.
auto escapeSpaces(const std::string& path) -> std::string
{
    return std::regex_replace(path, std::regex(" "), R"(\ )");
}

}  // namespace

TEST(ShaderCompilerTest, generatesValidShader)
{
    // CompiledShader checks the size and the magic number of the code
    EXPECT_NO_THROW([[maybe_unused]] auto shader = test::getTestShader());
}

TEST(ShaderCompilerTest, generatesSameShaderOnEachCall)
{
    EXPECT_THAT(&test::getTestShader(), Eq(&test::getTestShader()));
}

TEST(ShaderCompilerTest, generatesAllEntryPointsInOneModule)
{
    const auto entryPoints = getEntryPoints(test::getTestShader().getSpirv());

    EXPECT_THAT(entryPoints, ContainerEq(std::map<std::string, uint32_t>{
                                 {"fragmentMain", ExecutionModelFragment},
                                 {"vertexMain", ExecutionModelVertex},
                             }));
}

TEST(ShaderCompilerTest, writesGeneratedFilesDependingOnShaderAndImportedModules)
{
    // Paths come from CMake, which uses "/" on every OS, as the tool does. The dependencies are sorted.
    const auto sourceDir = escapeSpaces(TEST_SHADER_SOURCE_DIR);
    const auto generatedDir = escapeSpaces(TEST_SHADER_GENERATED_DIR);
    const auto expected = fmt::format(
        "{0}/test_shader.h \\\n"
        "{0}/test_shader.cpp: \\\n"
        "  {1}/test_module.slang \\\n"
        "  {1}/test_shader.slang\n",
        generatedDir, sourceDir);

    std::ifstream file(path::fromUtf8(TEST_SHADER_GENERATED_DIR) / "test_shader.d", std::ios::binary);
    const std::string depfile{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    EXPECT_THAT(depfile, Eq(expected));
}
