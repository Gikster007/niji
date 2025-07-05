#pragma once

#include <memory>

#include "rendering/model/model.hpp"

namespace niji
{

struct MeshComponent
{
    Model* Model = nullptr;
    uint16_t MeshID = -1;
    uint16_t MaterialID = -1;
};

} // namespace niji