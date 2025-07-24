#pragma once

#include "common.hpp"

namespace niji
{
class Envmap
{
  public:
    Envmap() = default;
    Envmap(const std::string& path);
    ~Envmap();

    void cleanup();

  private:
    void LoadSpecular(const std::string& path, const int mips);
    void LoadDiffuse(const std::string& path);
    void LoadLUT(const std::string& path);

  private:
    friend class SkyboxPass;
    friend class ForwardPass;

    Texture m_specularCubemap = {};
    Texture m_diffuseCubemap = {};
    Texture m_brdfTexture = {};
};
} // namespace niji