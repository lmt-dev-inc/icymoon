#include <im3e/utils/loggers.h>

#include <fmt/format.h>

using namespace im3e;

int main(int argc, char* argv[])
{
    auto logger = createTerminalLogger();
    logger->info(fmt::format("im3e_shader_compiler called with {} arguments", argc - 1));
    for (int i = 1; i < argc; i++)
    {
        logger->info(fmt::format("arg[{}] = {}", i, argv[i]));
    }

    return 0;
}