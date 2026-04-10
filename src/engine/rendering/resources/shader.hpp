#pragma once

#include <string>

namespace niji
{

enum class ShaderType
{
    NONE,
    VERTEX,
    FRAGMENT,
    FRAG_AND_VERT,
    COMPUTE
};

struct Shader
{
    Shader() = default;
    Shader(const std::string path, const ShaderType shaderType);

    std::string Source = {};
    std::vector<std::string> Spirv = {};
    ShaderType Type = {};

    void init();

    bool re_compile();
};

} // namespace niji