// Compiles a compute shader to SPIR-V, which checks that the library links, loads, and finds its standard modules.
#include <slang-com-ptr.h>
#include <slang.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr char Source[] = R"(
[shader("compute")]
[numthreads(1, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
}
)";

void check(SlangResult result, slang::IBlob* diagnostics, const char* step)
{
    if (SLANG_FAILED(result))
    {
        std::string message = std::string("Failed to ") + step;
        if (diagnostics)
        {
            message += ":\n" + std::string(static_cast<const char*>(diagnostics->getBufferPointer()),
                                           diagnostics->getBufferSize());
        }
        throw std::runtime_error(message);
    }
}

}  // namespace

int main()
{
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    check(slang::createGlobalSession(globalSession.writeRef()), nullptr, "create the global session");

    slang::TargetDesc target{};
    target.format = SLANG_SPIRV;
    target.profile = globalSession->findProfile("spirv_1_5");

    slang::SessionDesc sessionDesc{};
    sessionDesc.targets = &target;
    sessionDesc.targetCount = 1;

    Slang::ComPtr<slang::ISession> session;
    check(globalSession->createSession(sessionDesc, session.writeRef()), nullptr, "create the session");

    Slang::ComPtr<slang::IBlob> diagnostics;
    slang::IModule* module = session->loadModuleFromSourceString("example", "example.slang", Source,
                                                                 diagnostics.writeRef());
    check(module ? SLANG_OK : SLANG_FAIL, diagnostics, "load the module");

    Slang::ComPtr<slang::IEntryPoint> entryPoint;
    check(module->findEntryPointByName("main", entryPoint.writeRef()), nullptr, "find the entry point");

    slang::IComponentType* components[] = {module, entryPoint};
    Slang::ComPtr<slang::IComponentType> program;
    check(session->createCompositeComponentType(components, 2, program.writeRef(), diagnostics.writeRef()), diagnostics,
          "compose the program");

    Slang::ComPtr<slang::IComponentType> linked;
    check(program->link(linked.writeRef(), diagnostics.writeRef()), diagnostics, "link the program");

    Slang::ComPtr<slang::IBlob> code;
    check(linked->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef()), diagnostics, "generate SPIR-V");

    uint32_t magic = 0;
    if (code->getBufferSize() < sizeof(magic))
    {
        throw std::runtime_error("Generated SPIR-V is empty");
    }
    std::memcpy(&magic, code->getBufferPointer(), sizeof(magic));
    if (magic != 0x07230203)
    {
        throw std::runtime_error("Generated code doesn't start with the SPIR-V magic number");
    }
    std::cout << "Compiled " << code->getBufferSize() << " bytes of SPIR-V with slang" << std::endl;

    return 0;
}
