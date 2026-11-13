// Compiles a Slang file to SPIR-V, embedded in generated C++ code. Run by the build:
//
//   im3e_shader_compiler <input .slang>
//     --header <output .h>
//     --source <output .cpp>
//     --depfile <output dependency file>
//     --function <qualified name of the generated function, e.g. im3e::test::getTestShader>
//
// Arguments are read as UTF-8 on every OS. On Windows, argv uses the code page of the process, so CLI11 reads them
// from the UTF-16 command line instead.
#include <im3e/utils/core/path_utils.h>
#include <im3e/utils/core/throw_utils.h>

#include <fmt/format.h>
#include <fmt/std.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <CLI/CLI.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace im3e;
using namespace fmt::literals;

namespace {

struct Arguments
{
    std::filesystem::path input;
    std::filesystem::path header;
    std::filesystem::path source;
    std::filesystem::path depfile;
    std::string function;
};

int parseCliArguments(int argc, char* argv[], Arguments& arguments)
{
    CLI::App app("Compiles a Slang file to SPIR-V, embedded in generated C++ code", "im3e_shader_compiler");
    argv = app.ensure_utf8(argv);

    std::string input;
    std::string header;
    std::string source;
    std::string depfile;
    std::string function;
    app.add_option("input", input, "Slang file with the entry points")->required();
    app.add_option("--header", header, "Generated header")->required();
    app.add_option("--source", source, "Generated source file, which includes the header by its file name")->required();
    app.add_option("--depfile", depfile, "Dependency file listing the files read, for the build system")->required();
    app.add_option("--function", function, "Qualified name of the generated function, e.g. im3e::getShader")
        ->required()
        ->check(CLI::Validator(
            [](const std::string& name) {
                static const std::regex QualifiedName(R"([A-Za-z_]\w*(::[A-Za-z_]\w*)*)");
                return std::regex_match(name, QualifiedName) ? std::string() : "not a qualified C++ name";
            },
            "QUALIFIED_NAME"));

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e)
    {
        // Prints the help, or the error with the usage, and returns the matching exit code
        return app.exit(e);
    }

    arguments = Arguments{
        .input = path::fromUtf8(input),
        .header = path::fromUtf8(header),
        .source = path::fromUtf8(source),
        .depfile = path::fromUtf8(depfile),
        .function = function,
    };
    return 0;
}

void logIfExists(slang::IBlob* diagnostics)
{
    if (diagnostics && diagnostics->getBufferSize() > 0U)
    {
        const auto text = std::string_view(static_cast<const char*>(diagnostics->getBufferPointer()),
                                           diagnostics->getBufferSize());
        fmt::print(stderr, "{}{}", text, text.ends_with("\n") ? "" : "\n");
    }
}

void throwIfSlangFailed(SlangResult result, std::string_view stepName)
{
    if (SLANG_FAILED(result))
    {
        throw std::runtime_error(fmt::format("error: failed to {}", stepName));
    }
}

auto createSlangGlobalSession()
{
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    throwIfSlangFailed(slang::createGlobalSession(globalSession.writeRef()), "create the global session");
    return globalSession;
}

auto createSlangSession(Slang::ComPtr<slang::IGlobalSession> globalSession)
{
    slang::TargetDesc target{
        .format = SLANG_SPIRV,
        .profile = globalSession->findProfile("spirv_1_5"),
    };
    slang::SessionDesc sessionDesc{
        .targets = &target,
        .targetCount = 1,
    };
    Slang::ComPtr<slang::ISession> session;
    throwIfSlangFailed(globalSession->createSession(sessionDesc, session.writeRef()), "create the session");
    return session;
}

auto readFile(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    throwIfFalse<std::runtime_error>(!!file, fmt::format("{:g}: error: cannot open file", filePath));
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

auto loadModule(Slang::ComPtr<slang::ISession> session, const std::filesystem::path& filePath)
{
    const auto moduleName = path::toUtf8(filePath.stem());
    const auto sourceCode = readFile(filePath);

    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IModule> module(session->loadModuleFromSourceString(
        moduleName.c_str(), path::toUtf8(filePath).c_str(), sourceCode.c_str(), diagnostics.writeRef()));
    logIfExists(diagnostics);
    throwIfFalse<std::runtime_error>(module != nullptr, fmt::format("error: failed to load module {}", moduleName));
    return module;
}

struct FunctionName
{
    explicit FunctionName(std::string_view fullName)
    {
        const auto findPos = fullName.rfind("::");
        if (findPos != fullName.npos)
        {
            namespaces = fullName.substr(0U, findPos);
            name = fullName.substr(findPos + 2U);
        }
        else
        {
            name = fullName;
        }
    }

    std::string_view namespaces;  // Empty for global namespace
    std::string_view name;
};

void writeFileIfChanged(const std::filesystem::path& filePath, std::string_view content)
{
    // Keeps the timestamp of an unchanged file, so that the build system doesn't rebuild what depends on it. Comparing
    // the sizes first avoids reading files that changed size.
    std::error_code error;
    const auto existingSize = std::filesystem::file_size(filePath, error);
    if (!error && existingSize == content.size() && readFile(filePath) == content)
    {
        return;
    }

    // Make sure that all parent folders exist:
    std::filesystem::create_directories(filePath.parent_path());

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    throwIfFalse<std::runtime_error>(!!file, fmt::format("{:g}: error: cannot open file", filePath));

    file.write(content.data(), static_cast<std::streamsize>(content.size()));
    file.close();
    throwIfFalse<std::runtime_error>(!!file, fmt::format("{:g}: error: cannot write file", filePath));
}

void createHeaderFile(const std::filesystem::path& shaderPath, const std::filesystem::path& filePath,
                      const FunctionName& functionName)
{
    constexpr auto HeaderSource = R"(// Generated by im3e_shader_compiler
// Source file: "{shaderFile}"
// Do not Edit.
#pragma once

#include <im3e/shader/compiled_shader.h>

{namespaceBegin}[[nodiscard]] auto {functionName}() -> const im3e::CompiledShader&;{namespaceEnd}
)";

    const auto hasNamespace = !functionName.namespaces.empty();
    const auto namespaceBegin = hasNamespace ? fmt::format("namespace {} {{\n\n", functionName.namespaces) : "";
    const auto namespaceEnd = hasNamespace ? fmt::format("\n\n}}  // namespace {}", functionName.namespaces) : "";

    writeFileIfChanged(filePath, fmt::format(HeaderSource, "shaderFile"_a = path::toUtf8(shaderPath.filename()),
                                             "namespaceBegin"_a = namespaceBegin, "namespaceEnd"_a = namespaceEnd,
                                             "functionName"_a = functionName.name));
}

void createSourceFile(const Arguments& args)
{
    constexpr auto Source = R"(// Generated by im3e_shader_compiler
// Source file: "{shaderFile}"
// Do not Edit.
#include "{headerFile}"

#include <span>

namespace {{

constexpr auto SpirVCode = std::span<const uint32_t>{{}};

}}  // namespace

auto {functionName}() -> const im3e::CompiledShader&
{{
    static const im3e::CompiledShader shader{{SpirVCode}};
    return shader;
}}
)";

    writeFileIfChanged(args.source, fmt::format(Source, "shaderFile"_a = path::toUtf8(args.input.filename()),
                                                "headerFile"_a = path::toUtf8(args.header.filename()),
                                                "functionName"_a = args.function));
}

}  // namespace

int main(int argc, char* argv[])
{
    Arguments arguments;
    if (auto errorCode = parseCliArguments(argc, argv, arguments))
    {
        return errorCode;
    }

    if (!std::filesystem::is_regular_file(arguments.input))
    {
        fmt::print(stderr, "{:g}: error: file not found\n", arguments.input);
        return 1;
    }

    try
    {
        auto globalSession = createSlangGlobalSession();
        auto session = createSlangSession(globalSession);
        auto module = loadModule(session, arguments.input);

        FunctionName functionName{arguments.function};
        createHeaderFile(arguments.input, arguments.header, functionName);
        createSourceFile(arguments);
    }
    catch (const std::exception& exception)
    {
        fmt::print(stderr, "{}\n", exception.what());
        return 1;
    }

    return 0;
}
