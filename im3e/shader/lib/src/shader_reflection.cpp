#include <im3e/shader/shader_reflection.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <sstream>

using namespace im3e;

auto im3e::operator<<(std::ostream& stream, ShaderStage stage) -> std::ostream&
{
    switch (stage)
    {
        case ShaderStage::Vertex: stream << "ShaderStage::Vertex"; break;
        case ShaderStage::Fragment: stream << "ShaderStage::Fragment"; break;
        case ShaderStage::Compute: stream << "ShaderStage::Compute"; break;
    }
    return stream;
}

auto im3e::operator<<(std::ostream& stream, const PushConstantRange& range) -> std::ostream&
{
    stream << fmt::format("PushConstantRange{{ .offset = {}, .size = {} }}", range.offset, range.size);
    return stream;
}

auto im3e::operator<<(std::ostream& stream, const ShaderEntryPoint& entryPoint) -> std::ostream&
{
    std::stringstream pushConstantsStr;
    if (entryPoint.pushConstants.has_value())
    {
        pushConstantsStr << entryPoint.pushConstants.value();
    }
    else
    {
        pushConstantsStr << "none";
    }

    stream << fmt::format("ShaderEntryPoint{{ .stage = {}, .name = {}, .pushConstants = {} }}",
                          fmt::streamed(entryPoint.stage), fmt::streamed(entryPoint.name), pushConstantsStr.str());
    return stream;
}

auto im3e::operator<<(std::ostream& stream, const ShaderReflection& reflection) -> std::ostream&
{
    stream << "ShaderReflection{ .entryPoints = { ";
    auto separator = "";
    for (const auto& entryPoint : reflection.entryPoints)
    {
        stream << separator << entryPoint;
        separator = ", ";
    }
    stream << " } }";
    return stream;
}