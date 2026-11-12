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

#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <fmt/std.h>

#include <cstdio>
#include <filesystem>
#include <regex>
#include <string>

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

}  // namespace

int main(int argc, char* argv[])
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

    const Arguments arguments{
        .input = path::fromUtf8(input),
        .header = path::fromUtf8(header),
        .source = path::fromUtf8(source),
        .depfile = path::fromUtf8(depfile),
        .function = function,
    };

    if (!std::filesystem::is_regular_file(arguments.input))
    {
        fmt::print(stderr, "{:g}: error: file not found\n", arguments.input);
        return 1;
    }

    return 0;
}
