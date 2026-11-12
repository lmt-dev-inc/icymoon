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
    const auto moduleName = path::toUtf8(filePath.filename().stem());
    const auto sourceCode = readFile(filePath);

    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IModule> module(session->loadModuleFromSourceString(
        moduleName.c_str(), path::toUtf8(filePath).c_str(), sourceCode.c_str(), diagnostics.writeRef()));
    logIfExists(diagnostics);
    throwIfFalse<std::runtime_error>(module != nullptr, fmt::format("error: failed to load module {}", moduleName));
    return module;
}

void createHeaderFile(const std::filesystem::path& shaderPath, const std::filesystem::path& filePath,
                      std::string_view functionName)
{
    std::ofstream file(filePath, std::ios::trunc);
    throwIfFalse<std::runtime_error>(!!file, fmt::format("{:g}: error: cannot open file", filePath));

    file << fmt::format(R"(/// Generated by im3e_shader_compiler
/// Source file: "{}"
/// Do not Edit.
#pragma once

#include <im3e/shader/compiled_shader.h>

)",
                        shaderPath);

    const auto findPos = functionName.rfind("::");
    std::string_view functionNamespaces;
    if (findPos != functionName.npos)
    {
        functionNamespaces = functionName.substr(0U, findPos);
        functionName = functionName.substr(findPos + 2U);
        file << fmt::format("namespace {} {{", functionNamespaces) << std::endl << std::endl;
    }

    file << fmt::format("[[nodiscard]] auto {}() -> const im3e::CompiledShader&;", functionName) << std::endl
         << std::endl;

    if (!functionNamespaces.empty())
    {
        file << fmt::format("}} // namespace {}", functionNamespaces) << std::endl << std::endl;
    }
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
        createHeaderFile(arguments.input, arguments.header, arguments.function);
    }
    catch (const std::exception& exception)
    {
        fmt::print(stderr, "{}\n", exception.what());
    }

    return 0;
}
