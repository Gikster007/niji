#include "shader.hpp"

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace niji
{

Shader::Shader(const std::string path, const ShaderType shaderType)
{
    Source = path;
    Type = shaderType;

    init();
}

void Shader::init()
{
    if (Type == ShaderType::NONE)
        throw std::runtime_error("Invalid Shader Type");

    fs::path p(Source);
    p = p.replace_extension("");

    fs::path newParent = p.parent_path() / "spirv";

    fs::path newPath = newParent / p.filename();

    std::string outDir = newPath.string();

    if (Type == ShaderType::FRAG_AND_VERT)
    {
        {
            std::string vert = outDir + ".vert" + ".spv";
            Spirv.push_back(vert);
        }
        {
            std::string frag = outDir + ".frag" + ".spv";
            Spirv.push_back(frag);
        }
    }
    else if (Type == ShaderType::COMPUTE)
    {
        std::string compute = outDir + ".spv";
        Spirv.push_back(compute);
    }
}

bool Shader::re_compile()
{
    bool result = false;

    if (Type == ShaderType::NONE)
        return result;

    if (Type == ShaderType::FRAG_AND_VERT)
    {
        {
            std::string cmd = "slangc " + Source + " -entry " + "vertex" +
                              "_main -profile vs_6_0 -target spirv -o " + Spirv[0] + " -g";
            result = std::system(cmd.c_str()) == 0;
        }

        {
            std::string cmd = "slangc " + Source + " -entry " + "fragment" +
                              "_main -profile ps_6_0 -target spirv -o " + Spirv[1] + " -g";
            result &= std::system(cmd.c_str()) == 0;
        }
    }
    else if (Type == ShaderType::COMPUTE)
    {
        std::string cmd = "slangc " + Source + " -entry " + "compute" +
                          "_main -profile cs_6_0 -target spirv -o " + Spirv[0] + " -g";
        result = std::system(cmd.c_str()) == 0;
    }

    return result;
}

} // namespace niji